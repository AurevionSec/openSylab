#include "utils/Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/sink.h>
#include <vector>

namespace opensylab::utils {

std::shared_ptr<spdlog::logger> Logger::instance_;

void Logger::init(const std::string& logLevel, const std::string& logFile) {
    std::vector<spdlog::sink_ptr> sinks;

    // Always add a colour console sink
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sinks.push_back(consoleSink);

    // Optional rotating file sink (max 10 MB, 3 files)
    if (!logFile.empty()) {
        constexpr std::size_t kMaxBytes = 10 * 1024 * 1024;
        constexpr std::size_t kMaxFiles = 3;
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logFile, kMaxBytes, kMaxFiles);
        sinks.push_back(fileSink);
    }

    instance_ = std::make_shared<spdlog::logger>("opensylab", sinks.begin(), sinks.end());
    instance_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");

    // Map log level string to spdlog level
    spdlog::level::level_enum level = spdlog::level::info;
    if (logLevel == "trace")    level = spdlog::level::trace;
    else if (logLevel == "debug")    level = spdlog::level::debug;
    else if (logLevel == "info")     level = spdlog::level::info;
    else if (logLevel == "warn")     level = spdlog::level::warn;
    else if (logLevel == "error")    level = spdlog::level::err;
    else if (logLevel == "critical") level = spdlog::level::critical;

    instance_->set_level(level);
    spdlog::register_logger(instance_);
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!instance_) {
        // Lazy-init with default settings when init() was not called explicitly
        init("info", "");
    }
    return instance_;
}

} // namespace opensylab::utils
