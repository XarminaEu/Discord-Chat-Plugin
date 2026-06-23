#pragma once

#include <string>
#include <functional>
#include <thread>
#include <memory>
#include <atomic>
#include <mutex>

// File-based IPC bridge between the UE4SS Lua mod (in the game process) and the
// plugin core.
//
//   game  -> Discord : Lua appends "author\tmessage\n" to the outgoing file;
//                      FileBridge tails it and invokes the line callback.
//   Discord -> game  : FileBridge appends "author\tmessage\n" to the incoming
//                      file; the Lua mod tails it and injects the chat.
//
// Both sides are append-only with independent byte-offset tracking, so there
// are no truncation races.
class FileBridge {
public:
    FileBridge(const std::string& incoming_file, const std::string& outgoing_file);
    ~FileBridge();

    bool Start();
    void Stop();
    bool IsRunning() const;

    // Invoked for each new line found in the outgoing file (game -> Discord).
    using LineCallback = std::function<void(const std::string& author, const std::string& message)>;
    void SetLineCallback(LineCallback callback);

    // Appends a message to the incoming file (Discord -> game).
    bool WriteIncoming(const std::string& author, const std::string& message);

private:
    std::string incoming_file_;
    std::string outgoing_file_;
    std::atomic<bool> running_;
    unsigned long long read_offset_;
    std::string partial_line_;
    std::unique_ptr<std::thread> watcher_thread_;
    std::mutex incoming_mutex_;
    LineCallback line_callback_;

    void WatchLoop();
    void ProcessNewData();
    void HandleLine(const std::string& line);
};
