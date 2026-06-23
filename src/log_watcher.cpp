#include "log_watcher.h"
#include "logger.h"

#include <fstream>
#include <chrono>
#include <filesystem>
#include <regex>

LogWatcher::LogWatcher(const std::string& log_path)
    : log_path_(log_path),
      running_(false),
      read_offset_(0) {
}

LogWatcher::~LogWatcher() {
    if (running_) {
        Stop();
    }
}

bool LogWatcher::Start(ChatCallback callback) {
    if (running_) {
        return true;
    }

    callback_ = callback;

    // Find the latest log file if directory is given
    if (std::filesystem::is_directory(log_path_)) {
        std::string latest = FindLatestLogFile();
        if (!latest.empty()) {
            log_path_ = latest;
        }
    }

    g_logger.Info("LogWatcher starting, monitoring: " + log_path_);

    // Start from current end of file
    {
        std::ifstream existing(log_path_, std::ios::binary | std::ios::ate);
        if (existing.is_open()) {
            read_offset_ = static_cast<unsigned long long>(existing.tellg());
            g_logger.Info("LogWatcher starting at offset: " + std::to_string(read_offset_));
        } else {
            read_offset_ = 0;
            g_logger.Warning("LogWatcher: file not yet created, will wait: " + log_path_);
        }
    }

    running_ = true;
    watcher_thread_ = std::make_unique<std::thread>(&LogWatcher::WatchLoop, this);
    return true;
}

void LogWatcher::Stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (watcher_thread_ && watcher_thread_->joinable()) {
        watcher_thread_->join();
    }
    watcher_thread_.reset();

    g_logger.Info("LogWatcher stopped");
}

bool LogWatcher::IsRunning() const {
    return running_;
}


std::string LogWatcher::FindLatestLogFile() {
    std::string latest_file;
    auto latest_time = std::filesystem::file_time_type::min();

    try {
        for (const auto& entry : std::filesystem::directory_iterator(log_path_)) {
            if (entry.is_regular_file()) {
                auto name = entry.path().filename().string();
                // Look for .log files first
                if (name.find(".log") != std::string::npos) {
                    auto ftime = std::filesystem::last_write_time(entry);
                    if (ftime > latest_time) {
                        latest_time = ftime;
                        latest_file = entry.path().string();
                    }
                }
            }
        }
        // If no .log files found, look for any file
        if (latest_file.empty()) {
            for (const auto& entry : std::filesystem::directory_iterator(log_path_)) {
                if (entry.is_regular_file()) {
                    auto ftime = std::filesystem::last_write_time(entry);
                    if (ftime > latest_time) {
                        latest_time = ftime;
                        latest_file = entry.path().string();
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        g_logger.Error("LogWatcher: error scanning directory: " + std::string(e.what()));
    }

    return latest_file;
}

void LogWatcher::WatchLoop() {
    int scan_counter = 0;
    while (running_) {
        // If path is a directory or doesn't exist, try to find the latest file
        if (std::filesystem::is_directory(log_path_) || !std::filesystem::exists(log_path_)) {
            if (++scan_counter >= 10) { // every 5 seconds try to find a new file
                scan_counter = 0;
                std::string latest = FindLatestLogFile();
                if (!latest.empty()) {
                    log_path_ = latest;
                    read_offset_ = 0;
                    g_logger.Info("LogWatcher switched to: " + log_path_);
                }
            }
        }
        ProcessNewData();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void LogWatcher::ProcessNewData() {
    std::ifstream in(log_path_, std::ios::binary);
    if (!in.is_open()) {
        return; // file not created yet
    }

    in.seekg(0, std::ios::end);
    unsigned long long file_size = static_cast<unsigned long long>(in.tellg());

    if (file_size < read_offset_) {
        // File was truncated/rotated; restart from beginning
        read_offset_ = 0;
        partial_line_.clear();
    }

    if (file_size == read_offset_) {
        return; // nothing new
    }

    in.seekg(static_cast<std::streamoff>(read_offset_), std::ios::beg);

    std::string chunk;
    chunk.resize(static_cast<size_t>(file_size - read_offset_));
    in.read(&chunk[0], static_cast<std::streamsize>(chunk.size()));
    std::streamsize got = in.gcount();
    chunk.resize(static_cast<size_t>(got));
    read_offset_ += static_cast<unsigned long long>(got);

    partial_line_ += chunk;

    size_t pos;
    while ((pos = partial_line_.find('\n')) != std::string::npos) {
        std::string line = partial_line_.substr(0, pos);
        partial_line_.erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            HandleLine(line);
        }
    }
}

void LogWatcher::HandleLine(const std::string& line) {
    if (!callback_) return;

    // === CHAT ===
    size_t chat_pos = line.find("[Chat::Global]");
    if (chat_pos == std::string::npos) {
        chat_pos = line.find("[Chat::");
    }
    if (chat_pos != std::string::npos) {
        size_t name_start = line.find("['", chat_pos);
        if (name_start != std::string::npos) {
            name_start += 2;
            size_t name_end = line.find("']", name_start);
            if (name_end != std::string::npos) {
                std::string player = line.substr(name_start, name_end - name_start);
                if (player.empty() || player == "''") {
                    size_t uid_start = line.find("UserId=", name_end);
                    if (uid_start != std::string::npos) {
                        uid_start += 7;
                        size_t uid_end = line.find(",", uid_start);
                        if (uid_end == std::string::npos) uid_end = line.find(")", uid_start);
                        if (uid_end != std::string::npos) player = line.substr(uid_start, uid_end - uid_start);
                    }
                }
                size_t msg_start = line.find(":", name_end);
                if (msg_start != std::string::npos) {
                    std::string message = line.substr(msg_start + 2);
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
        size_t name_start = line.find("['");
        if (name_start != std::string::npos) {
            name_start += 2;
            size_t name_end = line.find("'", name_start);
            if (name_end != std::string::npos) player = line.substr(name_start, name_end - name_start);
        }
        if (player.empty() || player == "''") {
            size_t uid = line.find("UserId=");
            if (uid != std::string::npos) {
                uid += 7;
                size_t uid_end = line.find(",", uid);
                if (uid_end == std::string::npos) uid_end = line.find(")", uid);
                if (uid_end != std::string::npos) player = line.substr(uid, uid_end - uid);
            }
        }
        if (!player.empty()) {
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
            size_t uid_end = line.find(",", uid);
            if (uid_end == std::string::npos) uid_end = line.find(")", uid);
            if (uid_end != std::string::npos) player = line.substr(uid, uid_end - uid);
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
            callback_("leave", player, "");
        }
        return;
    }

    // === DEATH ===
    if (line.find("died") != std::string::npos ||
        line.find("was killed") != std::string::npos) {
        std::string player;
        size_t name_start = line.find("['");
        if (name_start != std::string::npos) {
            name_start += 2;
            size_t name_end = line.find("'", name_start);
            if (name_end != std::string::npos) player = line.substr(name_start, name_end - name_start);
        }
        if (player.empty()) {
            size_t uid = line.find("UserId=");
            if (uid != std::string::npos) {
                uid += 7;
                size_t uid_end = line.find(",", uid);
                if (uid_end == std::string::npos) uid_end = line.find(")", uid);
                if (uid_end != std::string::npos) player = line.substr(uid, uid_end - uid);
            }
        }
        if (!player.empty()) {
            callback_("death", player, "");
        }
        return;
    }
}
