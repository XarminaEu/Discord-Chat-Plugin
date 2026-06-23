#include "console_pipe.h"
#include "logger.h"

#include <cstring>

ConsolePipe::ConsolePipe()
    : hOriginalStdout_(INVALID_HANDLE_VALUE),
      hReadPipe_(INVALID_HANDLE_VALUE),
      hWritePipe_(INVALID_HANDLE_VALUE),
      running_(false) {
}

ConsolePipe::~ConsolePipe() {
    if (running_) Stop();
}

bool ConsolePipe::Start(ChatCallback callback) {
    if (running_) return true;

    callback_ = callback;

    // Save original stdout
    hOriginalStdout_ = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOriginalStdout_ == INVALID_HANDLE_VALUE || hOriginalStdout_ == nullptr) {
        g_logger.Warning("ConsolePipe: no original stdout");
        return false;
    }

    // Create pipe
    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    if (!CreatePipe(&hReadPipe_, &hWritePipe_, &sa, 0)) {
        g_logger.Error("ConsolePipe: CreatePipe failed");
        return false;
    }

    // Set stdout to write end of pipe
    if (!SetStdHandle(STD_OUTPUT_HANDLE, hWritePipe_)) {
        g_logger.Error("ConsolePipe: SetStdHandle failed");
        CloseHandle(hReadPipe_);
        CloseHandle(hWritePipe_);
        return false;
    }

    // Also redirect stderr
    SetStdHandle(STD_ERROR_HANDLE, hWritePipe_);

    g_logger.Info("ConsolePipe: stdout redirected to pipe");

    running_ = true;
    thread_ = std::make_unique<std::thread>(&ConsolePipe::ReadLoop, this);
    return true;
}

void ConsolePipe::Stop() {
    if (!running_) return;

    running_ = false;

    // Close write end to unblock reader
    if (hWritePipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle(hWritePipe_);
        hWritePipe_ = INVALID_HANDLE_VALUE;
    }

    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    thread_.reset();

    // Restore original stdout
    if (hOriginalStdout_ != INVALID_HANDLE_VALUE) {
        SetStdHandle(STD_OUTPUT_HANDLE, hOriginalStdout_);
    }

    if (hReadPipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle(hReadPipe_);
        hReadPipe_ = INVALID_HANDLE_VALUE;
    }

    g_logger.Info("ConsolePipe stopped");
}

bool ConsolePipe::IsRunning() const {
    return running_;
}

void ConsolePipe::ForwardToOriginal(const std::string& data) {
    if (hOriginalStdout_ != INVALID_HANDLE_VALUE && hOriginalStdout_ != nullptr) {
        DWORD written = 0;
        WriteFile(hOriginalStdout_, data.c_str(), static_cast<DWORD>(data.size()), &written, nullptr);
    }
}

void ConsolePipe::ReadLoop() {
    char buffer[4096];
    while (running_) {
        DWORD bytesRead = 0;
        BOOL ok = ReadFile(hReadPipe_, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);
        if (!ok || bytesRead == 0) {
            if (GetLastError() == ERROR_BROKEN_PIPE) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        buffer[bytesRead] = '\0';

        // Forward to original stdout so server output is not lost
        ForwardToOriginal(std::string(buffer, bytesRead));

        // Parse for chat messages
        partial_line_.append(buffer, bytesRead);

        size_t pos;
        while ((pos = partial_line_.find('\n')) != std::string::npos) {
            std::string line = partial_line_.substr(0, pos);
            partial_line_.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            HandleLine(line);
        }
    }
}

void ConsolePipe::HandleLine(const std::string& line) {
    if (!callback_) return;

    // === CHAT ===
    // Parse: [Chat::Global]['Name' (UserId=...)]: msg
    size_t chat = line.find("[Chat::Global]");
    if (chat == std::string::npos) chat = line.find("[Chat::");
    if (chat != std::string::npos) {
        size_t nameStart = line.find("['", chat);
        if (nameStart != std::string::npos) {
            nameStart += 2;
            size_t nameEnd = line.find("']", nameStart);
            if (nameEnd != std::string::npos) {
                std::string player = line.substr(nameStart, nameEnd - nameStart);
                // Fallback to UserId if name empty
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
                    g_logger.Debug("ConsolePipe chat: " + player + ": " + message);
                    callback_("chat", player, message);
                    return;
                }
            }
        }
    }

    // === JOIN ===
    // Patterns:
    // steam_xxx ('IP') connected to the server.
    // '' (UserId=steam_xxx, IP=xxx) has logged in.
    if (line.find("connected to the server") != std::string::npos ||
        line.find("has logged in") != std::string::npos) {
        // Extract name or steam ID
        std::string player;
        size_t nameStart = line.find("['");
        if (nameStart != std::string::npos) {
            nameStart += 2;
            size_t nameEnd = line.find("'", nameStart);
            if (nameEnd != std::string::npos) {
                player = line.substr(nameStart, nameEnd - nameStart);
            }
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
            g_logger.Debug("ConsolePipe join: " + player);
            callback_("join", player, "");
        }
        return;
    }

    // === LEAVE ===
    // Pattern: player disconnected / left / logged out
    if (line.find("disconnected") != std::string::npos ||
        line.find("logged out") != std::string::npos ||
        line.find("has left") != std::string::npos) {
        // Try to extract player name/ID from the line
        std::string player;
        size_t uid = line.find("UserId=");
        if (uid != std::string::npos) {
            uid += 7;
            size_t uidEnd = line.find(",", uid);
            if (uidEnd == std::string::npos) uidEnd = line.find(")", uid);
            if (uidEnd != std::string::npos) player = line.substr(uid, uidEnd - uid);
        }
        if (player.empty()) {
            // Try steam_ prefix
            size_t steam = line.find("steam_");
            if (steam != std::string::npos) {
                size_t end = line.find_first_of(" )'\"\t", steam);
                if (end == std::string::npos) end = line.length();
                player = line.substr(steam, end - steam);
            }
        }
        if (!player.empty()) {
            g_logger.Debug("ConsolePipe leave: " + player);
            callback_("leave", player, "");
        }
        return;
    }
}
