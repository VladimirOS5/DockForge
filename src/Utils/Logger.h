#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <filesystem>

enum class LogLevel { Debug, Info, Warning, Error, Fatal };

class Logger {
public:
    static Logger& Instance();
    static void Initialize(); // Compatibility wrapper
    static void Shutdown();   // Compatibility wrapper
    void Init(const std::filesystem::path& logDir, const std::string& appName);
    void Log(LogLevel level, const std::string& message, const char* file, int line);
    void Log(LogLevel level, const std::string& message, int value); // Overload for compatibility
    void SetLevel(LogLevel level) { m_minLevel = level; }
    bool IsInitialized() const { return m_initialized; }
    void Shutdown();

    // Chat 11: Crash dump path
    std::filesystem::path GetLogDirectory() const { return m_logPath.parent_path(); }
    void WriteCrashDump(const std::string& context);
private:
    Logger() = default;
    ~Logger();
    void RotateIfNeeded();
    std::string LevelToString(LogLevel level);
    std::string CurrentTimestamp();
    std::string ExtractFileName(const char* path);
    std::mutex m_mutex;
    std::ofstream m_file;
    std::filesystem::path m_logPath;
    LogLevel m_minLevel = LogLevel::Debug;
    size_t m_maxSize = 10 * 1024 * 1024;
    int m_maxFiles = 5;
    bool m_initialized = false;
};

#define LOG_DEBUG(msg) Logger::Instance().Log(LogLevel::Debug, msg, __FILE__, __LINE__)
#define LOG_INFO(msg)  Logger::Instance().Log(LogLevel::Info,  msg, __FILE__, __LINE__)
#define LOG_WARN(msg)  Logger::Instance().Log(LogLevel::Warning, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::Instance().Log(LogLevel::Error, msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) Logger::Instance().Log(LogLevel::Fatal, msg, __FILE__, __LINE__)