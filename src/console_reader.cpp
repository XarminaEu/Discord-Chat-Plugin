#include "console_reader.h"
#include "logger.h"
#include <windows.h>
#include <vector>
#include <chrono>
#include <thread>

ConsoleReader::ConsoleReader() : running_(false), last_buffer_y_(0) {
}

ConsoleReader::~ConsoleReader() {
    Stop();
}

bool ConsoleReader::Start(ChatCallback callback) {
    if (running_) return true;

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE || hOut == nullptr) {
        g_logger.Warning("ConsoleReader: no stdout handle available");
        return false;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) {
        g_logger.Warning("ConsoleReader: stdout is not a console (probably redirected)");
        return false;
    }

    callback_ = callback;
    running_ = true;
    reader_thread_ = std::make_unique<std::thread>(&ConsoleReader::ReadLoop, this);
    g_logger.Info("ConsoleReader started (reading console buffer every 500ms)");
    return true;
}

void ConsoleReader::Stop() {
    if (!running_) return;
    running_ = false;
    if (reader_thread_ && reader_thread_->joinable()) {
        reader_thread_->join();
    }
    reader_thread_.reset();
    g_logger.Info("ConsoleReader stopped");
}

bool ConsoleReader::IsRunning() const {
    return running_;
}

void ConsoleReader::ReadLoop() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    // Get buffer dimensions once
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;

    SHORT width = csbi.dwSize.X;
    SHORT height = csbi.dwSize.Y;
    if (width <= 0 || height <= 0) return;

    // Store previous lines to detect new ones
    std::vector<std::string> prev_lines(height);

    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Update dimensions (in case console was resized)
        if (!GetConsoleScreenBufferInfo(hOut, &csbi)) continue;
        SHORT curWidth = csbi.dwSize.X;
        SHORT curHeight = csbi.dwSize.Y;
        if (curWidth <= 0 || curHeight <= 0) continue;

        // Read current buffer
        std::vector<std::string> current_lines;
        current_lines.reserve(curHeight);

        for (SHORT y = 0; y < curHeight; ++y) {
            COORD coord = {0, y};
            DWORD read = 0;
            std::vector<char> buf(curWidth + 1);
            if (ReadConsoleOutputCharacterA(hOut, buf.data(), curWidth, coord, &read)) {
                buf[read] = '\0';
                std::string line(buf.data());
                // Trim trailing spaces
                size_t end = line.find_last_not_of(" \t\r\n");
                if (end != std::string::npos) {
                    line = line.substr(0, end + 1);
                } else {
                    line.clear();
                }
                current_lines.push_back(line);
            }
        }

        // Find new/changed lines
        for (size_t i = 0; i < current_lines.size(); ++i) {
            if (i < prev_lines.size() && prev_lines[i] == current_lines[i]) {
                continue; // unchanged
            }
            if (!current_lines[i].empty()) {
                HandleLine(current_lines[i]);
            }
        }

        prev_lines = std::move(current_lines);
    }
}

void ConsoleReader::HandleLine(const std::string& line) {
    if (!callback_) return;

    // === CHAT ===
    size_t chat = line.find("[Chat::Global]");
    if (chat == std::string::npos) chat = line.find("[Chat::");
    if (chat != std::string::npos) {
        size_t nameStart = line.find("['", chat);
        if (nameStart != std::string::npos) {
            nameStart += 2;
            size_t nameEnd = line.find("']", nameStart);
            if (nameEnd != std::string::npos) {
                std::string player = line.substr(nameStart, nameEnd - nameStart);
                if (player.empty() || player == "''") {
                    size_t uid = line.find("UserId=", nameEnd);
                    if (uid != std::string::npos) {
                        uid += 7;
                        size_t uidEnd = line.find(",", uid);
                        if (uidEnd == std::string::npos) uidEnd = line.find(")", uid);
                        if (uidEnd != std::string::npos) player = line.substr(uid, uidEnd - uid);
                    }
                }
                size_t msgStart = line.find(":", nameEnd);
                if (msgStart != std::string::npos) {
                    std::string message = line.substr(msgStart + 2);
                    g_logger.Debug("ConsoleReader chat: [" + player + "]: " + message);
                    callback_("chat", player, message);
                    return;
                }
            }
        }
    }

    // === JOIN ===
    if (line.find("connected to the server") != std::string::npos ||
        line.find("has logged in") != std::string::npos) {
        std::string player;
        size_t nameStart = line.find("['");
        if (nameStart != std::string::npos) {
            nameStart += 2;
            size_t nameEnd = line.find("'", nameStart);
            if (nameEnd != std::string::npos) player = line.substr(nameStart, nameEnd - nameStart);
        }
        if (player.empty() || player == "''") {
            size_t uid = line.find("UserId=");
            if (uid != std::string::npos) {
                uid += 7;
                size_t uidEnd = line.find(",", uid);
                if (uidEnd == std::string::npos) uidEnd = line.find(")", uid);
                if (uidEnd != std::string::npos) player = line.substr(uid, uidEnd - uid);
            }
        }
        if (!player.empty()) {
            g_logger.Debug("ConsoleReader join: " + player);
            callback_("join", player, "");
        }
        return;
    }

    // === LEAVE ===
    if (line.find("disconnected") != std::string::npos ||
        line.find("logged out") != std::string::npos ||
        line.find("has left") != std::string::npos) {
        std::string player;
        size_t uid = line.find("UserId=");
        if (uid != std::string::npos) {
            uid += 7;
            size_t uidEnd = line.find(",", uid);
            if (uidEnd == std::string::npos) uidEnd = line.find(")", uid);
            if (uidEnd != std::string::npos) player = line.substr(uid, uidEnd - uid);
        }
        if (player.empty()) {
            size_t steam = line.find("steam_");
            if (steam != std::string::npos) {
                size_t end = line.find_first_of(" )'\"\t", steam);
                if (end == std::string::npos) end = line.length();
                player = line.substr(steam, end - steam);
            }
        }
        if (!player.empty()) {
            g_logger.Debug("ConsoleReader leave: " + player);
            callback_("leave", player, "");
        }
        return;
    }

    // === DEATH ===
    if (line.find("died") != std::string::npos ||
        line.find("was killed") != std::string::npos) {
        std::string player;
        size_t nameStart = line.find("['");
        if (nameStart != std::string::npos) {
            nameStart += 2;
            size_t nameEnd = line.find("'", nameStart);
            if (nameEnd != std::string::npos) player = line.substr(nameStart, nameEnd - nameStart);
        }
        if (player.empty()) {
            size_t uid = line.find("UserId=");
            if (uid != std::string::npos) {
                uid += 7;
                size_t uidEnd = line.find(",", uid);
                if (uidEnd == std::string::npos) uidEnd = line.find(")", uid);
                if (uidEnd != std::string::npos) player = line.substr(uid, uidEnd - uid);
            }
        }
        if (!player.empty()) {
            g_logger.Debug("ConsoleReader death: " + player);
            callback_("death", player, "");
        }
        return;
    }
}
