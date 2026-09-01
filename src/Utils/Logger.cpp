#include "Logger.h"
#include <windows.h>
#include <sstream>
#include <iomanip>

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (m_file.is_open()) m_file.close();
}

void Logger::Initialize() {
    // Compatibility wrapper - initialize with default paths
    Instance().Init(std::filesystem::current_path(), "DockForge");
}

void Logger::Shutdown() {
    Instance().m_file.close();
    Instance().m_initialized = false;
}

void Logger::Log(LogLevel level, const std::string& message, int value) {
    // Overload for compatibility with 3-argument calls
    Log(level, message + " (value: " + std::to_string(value) + ")", "", 0);
}

void Logger::Init(const std::filesystem::path& logDir, const std::string& appName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return;
    std::filesystem::create_directories(logDir);
    m_logPath = logDir / (appName + ".log");
    RotateIfNeeded();
    m_file.open(m_logPath, std::ios::app);
    m_initialized = true;
    Log(LogLevel::Info, "Logger initialized. Path: " + m_logPath.string(), __FILE__, __LINE__);
}

void Logger::Log(LogLevel level, const std::string& message, const char* file, int line) {
    if (level < m_minLevel) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_file.is_open()) return;
    RotateIfNeeded();
    std::stringstream ss;
    ss << "[" << CurrentTimestamp() << "] "
       << "[" << LevelToString(level) << "] "
       << "[" << ExtractFileName(file) << ":" << line << "] "
       << message << "\n";
    m_file << ss.str();
    m_file.flush();
}

void Logger::WriteCrashDump(const std::string& context) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return;

    auto dumpPath = m_logPath;
    dumpPath += ".crash";
    std::ofstream dump(dumpPath, std::ios::app);
    if (!dump) return;

    dump << "\n=== CRASH DUMP ===\n";
    dump << "Time: " << CurrentTimestamp() << "\n";
    dump << "Context: " << context << "\n";
    dump << "Version: " << "1.0.0-alpha" << "\n";

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    dump << "CPU Cores: " << si.dwNumberOfProcessors << "\n";
    dump << "Page Size: " << si.dwPageSize << "\n";

    MEMORYSTATUSEX mem = { sizeof(mem) };
    if (GlobalMemoryStatusEx(&mem)) {
        dump << "Memory: " << (mem.ullTotalPhys / (1024*1024)) << " MB total, "
             << (mem.ullAvailPhys / (1024*1024)) << " MB free\n";
    }

    dump << "==================\n\n";
    dump.close();
}

void Logger::RotateIfNeeded() {
    if (!std::filesystem::exists(m_logPath)) return;
    auto size = std::filesystem::file_size(m_logPath);
    if (size < m_maxSize) return;
    m_file.close();
    std::filesystem::path oldest = m_logPath;
    oldest += "." + std::to_string(m_maxFiles);
    if (std::filesystem::exists(oldest)) std::filesystem::remove(oldest);
    for (int i = m_maxFiles - 1; i >= 1; --i) {
        std::filesystem::path oldPath = m_logPath;
        oldPath += "." + std::to_string(i);
        std::filesystem::path newPath = m_logPath;
        newPath += "." + std::to_string(i + 1);
        if (std::filesystem::exists(oldPath)) std::filesystem::rename(oldPath, newPath);
    }
    std::filesystem::path first = m_logPath;
    first += ".1";
    std::filesystem::rename(m_logPath, first);
    m_file.open(m_logPath, std::ios::app);
}

std::string Logger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
    }
    return "UNK  ";
}

std::string Logger::CurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
    std::tm tm{};
    localtime_s(&tm, &time);
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::ExtractFileName(const char* path) {
    std::string p(path);
    size_t pos = p.find_last_of("\\/");
    if (pos != std::string::npos) return p.substr(pos + 1);
    return p;
}