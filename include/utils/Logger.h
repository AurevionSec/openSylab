#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>

namespace opensylab::utils {

class Logger {
public:
    static void init(const std::string& logLevel = "info",
                     const std::string& logFile = "");
    static std::shared_ptr<spdlog::logger> get();

private:
    static std::shared_ptr<spdlog::logger> instance_;
};

} // namespace opensylab::utils

// Convenience macros
#define LOG_TRACE(...)    opensylab::utils::Logger::get()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)    opensylab::utils::Logger::get()->debug(__VA_ARGS__)
#define LOG_INFO(...)     opensylab::utils::Logger::get()->info(__VA_ARGS__)
#define LOG_WARN(...)     opensylab::utils::Logger::get()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    opensylab::utils::Logger::get()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) opensylab::utils::Logger::get()->critical(__VA_ARGS__)
