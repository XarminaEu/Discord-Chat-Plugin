#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <windows.h>

class ConsolePipe {
public:
    // Callback: type = "chat" | "join" | "leave", player = name, msg = extra info
    using ChatCallback = std::function<void(const std::string& type, const std::string& player, const std::string& msg)>;

    ConsolePipe();
    ~ConsolePipe();

    bool Start(ChatCallback callback);
    void Stop();
    bool IsRunning() const;

private:
    void ReadLoop();
    void HandleLine(const std::string& line);
    void ForwardToOriginal(const std::string& data);

    ChatCallback callback_;
    HANDLE hOriginalStdout_;
    HANDLE hReadPipe_;
    HANDLE hWritePipe_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> thread_;
    std::string partial_line_;
};
