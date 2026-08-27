#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

namespace server {

enum class LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void set_level(LogLevel level) { level_ = level; }

    void log(LogLevel level, const std::string& message) {
        if (level < level_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        std::ostringstream oss;
        oss << timestamp() << " [" << level_string(level) << "] "
            << "[Thread " << std::this_thread::get_id() << "] "
            << message << "\n";

        std::string formatted = oss.str();
        std::fwrite(formatted.c_str(), 1, formatted.size(), stdout);
        std::fflush(stdout);

        if (file_stream_.is_open()) {
            file_stream_ << formatted;
            file_stream_.flush();
        }
    }

    void debug(const std::string& msg) { log(LogLevel::LOG_DEBUG, msg); }
    void info(const std::string& msg)  { log(LogLevel::LOG_INFO, msg); }
    void warn(const std::string& msg)  { log(LogLevel::LOG_WARN, msg); }
    void error(const std::string& msg) { log(LogLevel::LOG_ERROR, msg); }

    void enable_file_output(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        file_stream_.open(path, std::ios::app);
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() : level_(LogLevel::LOG_INFO) {}

    LogLevel level_;
    std::mutex mutex_;
    std::ofstream file_stream_;

    static std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time);
#else
        localtime_r(&time, &tm_buf);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
        return std::string(buf);
    }

    static const char* level_string(LogLevel level) {
        switch (level) {
            case LogLevel::LOG_DEBUG: return "DEBUG";
            case LogLevel::LOG_INFO:  return "INFO";
            case LogLevel::LOG_WARN:  return "WARN";
            case LogLevel::LOG_ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }
};

#define LOG_DEBUG(msg) server::Logger::instance().debug(msg)
#define LOG_INFO(msg)  server::Logger::instance().info(msg)
#define LOG_WARN(msg)  server::Logger::instance().warn(msg)
#define LOG_ERROR(msg) server::Logger::instance().error(msg)

} // namespace server
