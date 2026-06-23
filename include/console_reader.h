#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <thread>

class ConsoleReader {
public:
    using ChatCallback = std::function<void(const std::string& type, const std::string& player, const std::string& msg)>;

    ConsoleReader();
    ~ConsoleReader();

    bool Start(ChatCallback callback);
    void Stop();
    bool IsRunning() const;

private:
    void ReadLoop();
    void HandleLine(const std::string& line);

    std::atomic<bool> running_;
    std::unique_ptr<std::thread> reader_thread_;
    ChatCallback callback_;
    int last_buffer_y_;
};
