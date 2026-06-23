#include "file_bridge.h"
#include "logger.h"

#include <fstream>
#include <chrono>

FileBridge::FileBridge(const std::string& incoming_file, const std::string& outgoing_file)
    : incoming_file_(incoming_file),
      outgoing_file_(outgoing_file),
      running_(false),
      read_offset_(0) {
}

FileBridge::~FileBridge() {
    if (running_) {
        Stop();
    }
}

bool FileBridge::Start() {
    if (running_) {
        return true;
    }

    // Start reading the outgoing file from its current end so we only process
    // messages produced after startup.
    {
        std::ifstream existing(outgoing_file_, std::ios::binary | std::ios::ate);
        if (existing.is_open()) {
            read_offset_ = static_cast<unsigned long long>(existing.tellg());
        } else {
            read_offset_ = 0;
        }
    }

    running_ = true;
    watcher_thread_ = std::make_unique<std::thread>(&FileBridge::WatchLoop, this);

    g_logger.Info("FileBridge started (in='" + incoming_file_ + "', out='" + outgoing_file_ + "')");
    return true;
}

void FileBridge::Stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (watcher_thread_ && watcher_thread_->joinable()) {
        watcher_thread_->join();
    }
    watcher_thread_.reset();

    g_logger.Info("FileBridge stopped");
}

bool FileBridge::IsRunning() const {
    return running_;
}

void FileBridge::SetLineCallback(LineCallback callback) {
    line_callback_ = callback;
}

bool FileBridge::WriteIncoming(const std::string& author, const std::string& message) {
    std::lock_guard<std::mutex> lock(incoming_mutex_);

    std::ofstream out(incoming_file_, std::ios::binary | std::ios::app);
    if (!out.is_open()) {
        g_logger.Error("FileBridge: cannot open incoming file for write: " + incoming_file_);
        return false;
    }

    // Sanitize: strip newlines/tabs from fields so the line format stays intact.
    auto sanitize = [](std::string s) {
        for (char& c : s) {
            if (c == '\n' || c == '\r' || c == '\t') c = ' ';
        }
        return s;
    };

    out << sanitize(author) << '\t' << sanitize(message) << '\n';
    out.flush();
    return true;
}

void FileBridge::WatchLoop() {
    while (running_) {
        ProcessNewData();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

void FileBridge::ProcessNewData() {
    std::ifstream in(outgoing_file_, std::ios::binary);
    if (!in.is_open()) {
        return; // file not created yet
    }

    in.seekg(0, std::ios::end);
    unsigned long long file_size = static_cast<unsigned long long>(in.tellg());

    if (file_size < read_offset_) {
        // File was truncated/rotated; restart from the beginning.
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

void FileBridge::HandleLine(const std::string& line) {
    // Format: author \t message
    size_t tab = line.find('\t');
    std::string author;
    std::string message;
    if (tab == std::string::npos) {
        author = "Player";
        message = line;
    } else {
        author = line.substr(0, tab);
        message = line.substr(tab + 1);
    }

    if (line_callback_) {
        line_callback_(author, message);
    }
}
