#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>

class LogWatcher {
public:
    // Callback: type("chat"/"join"/"leave"/"death"), player, msg
    using ChatCallback = std::function<void(const std::string& type, const std::string& player, const std::string& msg)>;

    LogWatcher(const std::string& log_path);
    ~LogWatcher();

    bool Start(ChatCallback callback);
    void Stop();
    bool IsRunning() const;

private:
    void WatchLoop();
    void ProcessNewData();
    void HandleLine(const std::string& line);
    std::string FindLatestLogFile();

    std::string log_path_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> watcher_thread_;
    ChatCallback callback_;
    unsigned long long read_offset_;
    std::string partial_line_;
};
