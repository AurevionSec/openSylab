#pragma once
#include <string>

namespace opensylab::utils {

/**
 * @brief Application configuration loaded from opensylab.conf or defaults.
 *
 * All fields have sensible defaults. CLI flags override any value set here.
 */
struct AppConfig {
    std::string dbPath;
    int         apiPort{8080};
    std::string tlsCert;
    std::string tlsKey;
    bool        forceHttps{false};
    std::string jwtSecretFile;
    // Match the API server's env-path default; a wildcard must be opt-in, not
    // the fallback, for a medical system.
    std::string corsOrigin{"http://localhost:5173"};
    std::string logLevel{"info"};
    std::string logFile;
    int         loginRateLimitPerMinute{10};
    int         sessionTimeoutMinutes{60};

    // Database backend selection: "sqlite" (default) or "postgresql" (stub, v1.1)
    std::string dbBackend{"sqlite"};
    std::string dbConnectionString;
};

/**
 * @brief Loads application configuration from an INI-style config file.
 *
 * Format rules:
 *   - Lines starting with '#' are comments and are ignored.
 *   - Blank lines are ignored.
 *   - Key/value pairs are separated by '='.
 *   - Leading/trailing whitespace is trimmed from both key and value.
 *   - Unknown keys are silently ignored.
 *   - Missing keys keep their default values.
 */
class Config {
public:
    /**
     * @brief Load configuration from the given file path.
     *
     * If the file does not exist or cannot be opened, defaults() is returned
     * without printing an error or calling std::exit().
     *
     * @param path Absolute or relative path to the config file.
     * @return Populated AppConfig (merged with defaults for missing keys).
     */
    static AppConfig loadFromFile(const std::string& path);

    /**
     * @brief Return a default-initialised AppConfig.
     */
    static AppConfig defaults();

    /**
     * @brief Return true if the given file exists and is readable.
     */
    static bool fileExists(const std::string& path);

private:
    static std::string trim(const std::string& s);
    static bool        parseBool(const std::string& s);
};

} // namespace opensylab::utils
