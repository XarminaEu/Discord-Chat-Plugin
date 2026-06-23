#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

Logger g_logger;

Logger::Logger() : debug_mode_(false), initialized_(false) {
}

Logger::~Logger() {
    if (initialized_) {
        Shutdown();
    }
}

bool Logger::Initialize(const std::string& log_file) {
    if (initialized_) {
        return true;
    }

    log_file_ = std::make_unique<std::ofstream>(log_file, std::ios::app);
    if (!log_file_ || !log_file_->is_open()) {
        std::cerr << "Failed to open log file: " << log_file << std::endl;
        return false;
    }

    initialized_ = true;
    Info("Logger initialized");
    return true;
}

void Logger::Shutdown() {
    if (!initialized_) {
        return;
    }

    Info("Logger shutting down");
    if (log_file_ && log_file_->is_open()) {
        log_file_->close();
    }
    log_file_.reset();
    initialized_ = false;
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (!initialized_) {
        return;
    }

    std::string level_str = GetLevelString(level);
    std::string timestamp = GetTimestamp();
    std::string log_message = "[" + timestamp + "] [" + level_str + "] " + message;

    if (log_file_ && log_file_->is_open()) {
        *log_file_ << log_message << std::endl;
        log_file_->flush();
    }

    if (debug_mode_ || level == LogLevel::Critical || level == LogLevel::Error) {
        std::cout << log_message << std::endl;
    }
}

void Logger::Debug(const std::string& message) {
    if (debug_mode_) {
        Log(LogLevel::Debug, message);
    }
}

void Logger::Info(const std::string& message) {
    Log(LogLevel::Info, message);
}

void Logger::Warning(const std::string& message) {
    Log(LogLevel::Warning, message);
}

void Logger::Error(const std::string& message) {
    Log(LogLevel::Error, message);
}

void Logger::Critical(const std::string& message) {
    Log(LogLevel::Critical, message);
}

void Logger::SetDebugMode(bool debug) {
    debug_mode_ = debug;
}

std::string Logger::GetLevelString(LogLevel level) const {
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARNING";
    case LogLevel::Error:
        return "ERROR";
    case LogLevel::Critical:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}

std::string Logger::GetTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}
