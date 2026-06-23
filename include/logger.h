#pragma once

#include <string>
#include <fstream>
#include <memory>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class Logger {
public:
    Logger();
    ~Logger();

    bool Initialize(const std::string& log_file);
    void Shutdown();

    void Log(LogLevel level, const std::string& message);
    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warning(const std::string& message);
    void Error(const std::string& message);
    void Critical(const std::string& message);

    void SetDebugMode(bool debug);

private:
    std::unique_ptr<std::ofstream> log_file_;
    bool debug_mode_;
    bool initialized_;

    std::string GetLevelString(LogLevel level) const;
    std::string GetTimestamp() const;
};

extern Logger g_logger;
