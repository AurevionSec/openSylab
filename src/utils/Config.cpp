/**
 * @file Config.cpp
 * @brief INI-style configuration file loader for OpenSylab.
 */

#include "utils/Config.h"
#include <cctype>
#include <fstream>

namespace opensylab::utils {

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string Config::trim(const std::string& s) {
    const auto* ws = " \t\r\n";
    const auto  first = s.find_first_not_of(ws);
    if (first == std::string::npos) {
        return {};
    }
    const auto last = s.find_last_not_of(ws);
    return s.substr(first, last - first + 1);
}

bool Config::parseBool(const std::string& s) {
    // Accept: true/false, 1/0, yes/no (case-insensitive)
    std::string lower;
    lower.reserve(s.size());
    for (const char c : s) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return (lower == "true" || lower == "1" || lower == "yes");
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

AppConfig Config::defaults() {
    return AppConfig{};
}

bool Config::fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

AppConfig Config::loadFromFile(const std::string& path) {
    AppConfig cfg = defaults();

    std::ifstream file(path);
    if (!file.is_open()) {
        // File does not exist or is not readable — return defaults silently.
        return cfg;
    }

    std::string line;
    while (std::getline(file, line)) {
        const std::string trimmedLine = trim(line);

        // Skip blank lines and comment lines
        if (trimmedLine.empty() || trimmedLine[0] == '#') {
            continue;
        }

        // Split on the first '=' character
        const auto eqPos = trimmedLine.find('=');
        if (eqPos == std::string::npos) {
            // Not a key=value line — ignore
            continue;
        }

        const std::string key   = trim(trimmedLine.substr(0, eqPos));
        const std::string value = trim(trimmedLine.substr(eqPos + 1));

        if (key.empty()) {
            continue;
        }

        // Map known keys to AppConfig fields; silently ignore unknown keys.
        if (key == "db_path") {
            cfg.dbPath = value;
        } else if (key == "api_port") {
            try {
                const int p = std::stoi(value);
                if (p >= 1 && p <= 65535) {  // valid TCP port range
                    cfg.apiPort = p;
                }
            } catch (const std::exception&) {
                // Keep default on parse error
            }
        } else if (key == "tls_cert") {
            cfg.tlsCert = value;
        } else if (key == "tls_key") {
            cfg.tlsKey = value;
        } else if (key == "force_https") {
            cfg.forceHttps = parseBool(value);
        } else if (key == "jwt_secret_file") {
            cfg.jwtSecretFile = value;
        } else if (key == "cors_origin") {
            cfg.corsOrigin = value;
        } else if (key == "log_level") {
            cfg.logLevel = value;
        } else if (key == "log_file") {
            cfg.logFile = value;
        } else if (key == "login_rate_limit_per_minute") {
            try {
                const int n = std::stoi(value);
                if (n > 0) {
                    cfg.loginRateLimitPerMinute = n;
                }
            } catch (const std::exception&) {
                // Keep default on parse error
            }
        } else if (key == "session_timeout_minutes") {
            try {
                const int n = std::stoi(value);
                if (n > 0) {
                    cfg.sessionTimeoutMinutes = n;
                }
            } catch (const std::exception&) {
                // Keep default on parse error
            }
        } else if (key == "db_backend") {
            cfg.dbBackend = value;
        } else if (key == "db_connection_string") {
            cfg.dbConnectionString = value;
        }
        // Unknown keys are intentionally ignored.
    }

    return cfg;
}

} // namespace opensylab::utils
