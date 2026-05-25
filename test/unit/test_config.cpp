/**
 * @file test_config.cpp
 * @brief Unit-Tests für Config (INI-style config file loader)
 */

#include "test_macros.h"
#include "utils/Config.h"
#include <cstdio>
#include <fstream>
#include <string>

using opensylab::utils::AppConfig;
using opensylab::utils::Config;

namespace {

// ---------------------------------------------------------------------------
// Helper: write a temporary file and return its path.
// The caller is responsible for removing it with std::remove().
// ---------------------------------------------------------------------------
std::string writeTempFile(const std::string& content) {
    // Use a fixed predictable path under /tmp to avoid dynamic allocation
    static int counter = 0;
    const std::string path =
        "/tmp/opensylab_test_config_" + std::to_string(counter++) + ".conf";
    std::ofstream f(path);
    f << content;
    return path;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

bool test_config_Defaults() {
    const AppConfig cfg = Config::defaults();
    ASSERT_EQ(cfg.apiPort, 8080);
    ASSERT_EQ(cfg.corsOrigin, std::string("*"));
    ASSERT_EQ(cfg.logLevel, std::string("info"));
    ASSERT_FALSE(cfg.forceHttps);
    ASSERT_EQ(cfg.loginRateLimitPerMinute, 10);
    ASSERT_EQ(cfg.sessionTimeoutMinutes, 60);
    ASSERT_TRUE(cfg.dbPath.empty());
    ASSERT_TRUE(cfg.tlsCert.empty());
    ASSERT_TRUE(cfg.tlsKey.empty());
    ASSERT_TRUE(cfg.jwtSecretFile.empty());
    ASSERT_TRUE(cfg.logFile.empty());
    return true;
}

bool test_config_NonExistentFileReturnsDefaults() {
    const AppConfig cfg =
        Config::loadFromFile("/tmp/opensylab_DOES_NOT_EXIST_99999.conf");
    ASSERT_EQ(cfg.apiPort, 8080);
    ASSERT_FALSE(cfg.forceHttps);
    return true;
}

bool test_config_EmptyFileReturnsDefaults() {
    const std::string path = writeTempFile("");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_EQ(cfg.apiPort, 8080);
    ASSERT_EQ(cfg.corsOrigin, std::string("*"));
    return true;
}

bool test_config_DbPathRead() {
    const std::string path =
        writeTempFile("db_path = /var/lib/opensylab/test.db\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_EQ(cfg.dbPath, std::string("/var/lib/opensylab/test.db"));
    return true;
}

bool test_config_ApiPortParsed() {
    const std::string path = writeTempFile("api_port = 9090\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_EQ(cfg.apiPort, 9090);
    return true;
}

bool test_config_ForceHttpsTrue() {
    const std::string path = writeTempFile("force_https = true\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_TRUE(cfg.forceHttps);
    return true;
}

bool test_config_ForceHttpsFalse() {
    const std::string path = writeTempFile("force_https = false\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_FALSE(cfg.forceHttps);
    return true;
}

bool test_config_ForceHttpsOne() {
    const std::string path = writeTempFile("force_https = 1\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_TRUE(cfg.forceHttps);
    return true;
}

bool test_config_ForceHttpsZero() {
    const std::string path = writeTempFile("force_https = 0\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_FALSE(cfg.forceHttps);
    return true;
}

bool test_config_ForceHttpsYes() {
    const std::string path = writeTempFile("force_https = yes\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_TRUE(cfg.forceHttps);
    return true;
}

bool test_config_ForceHttpsNo() {
    const std::string path = writeTempFile("force_https = no\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_FALSE(cfg.forceHttps);
    return true;
}

bool test_config_CommentsIgnored() {
    const std::string path = writeTempFile(
        "# This is a comment\n"
        "api_port = 7070\n"
        "# Another comment\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_EQ(cfg.apiPort, 7070);
    return true;
}

bool test_config_BlankLinesIgnored() {
    const std::string path = writeTempFile(
        "\n"
        "\n"
        "api_port = 7071\n"
        "\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_EQ(cfg.apiPort, 7071);
    return true;
}

bool test_config_UnknownKeysIgnored() {
    const std::string path = writeTempFile(
        "api_port = 7072\n"
        "unknown_key = some_value\n"
        "another_unknown = 42\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    // Should not crash and should parse known key correctly
    ASSERT_EQ(cfg.apiPort, 7072);
    return true;
}

bool test_config_WhitespaceTrimmed() {
    const std::string path = writeTempFile(
        "  api_port  =  7073  \n"
        "  db_path   =  /tmp/trimmed.db   \n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_EQ(cfg.apiPort, 7073);
    ASSERT_EQ(cfg.dbPath, std::string("/tmp/trimmed.db"));
    return true;
}

bool test_config_AllKnownFields() {
    const std::string path = writeTempFile(
        "db_path = /data/opensylab.db\n"
        "api_port = 8443\n"
        "tls_cert = /etc/ssl/cert.pem\n"
        "tls_key = /etc/ssl/key.pem\n"
        "force_https = true\n"
        "jwt_secret_file = /etc/opensylab/jwt.secret\n"
        "cors_origin = https://example.com\n"
        "log_level = debug\n"
        "log_file = /var/log/opensylab.log\n"
        "login_rate_limit_per_minute = 5\n"
        "session_timeout_minutes = 30\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());

    ASSERT_EQ(cfg.dbPath,               std::string("/data/opensylab.db"));
    ASSERT_EQ(cfg.apiPort,              8443);
    ASSERT_EQ(cfg.tlsCert,              std::string("/etc/ssl/cert.pem"));
    ASSERT_EQ(cfg.tlsKey,               std::string("/etc/ssl/key.pem"));
    ASSERT_TRUE(cfg.forceHttps);
    ASSERT_EQ(cfg.jwtSecretFile,        std::string("/etc/opensylab/jwt.secret"));
    ASSERT_EQ(cfg.corsOrigin,           std::string("https://example.com"));
    ASSERT_EQ(cfg.logLevel,             std::string("debug"));
    ASSERT_EQ(cfg.logFile,              std::string("/var/log/opensylab.log"));
    ASSERT_EQ(cfg.loginRateLimitPerMinute, 5);
    ASSERT_EQ(cfg.sessionTimeoutMinutes,   30);
    return true;
}

bool test_config_InvalidApiPortKeepsDefault() {
    const std::string path = writeTempFile("api_port = not_a_number\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_EQ(cfg.apiPort, 8080);
    return true;
}

bool test_config_InvalidRateLimitKeepsDefault() {
    const std::string path = writeTempFile("login_rate_limit_per_minute = abc\n");
    const AppConfig cfg = Config::loadFromFile(path);
    std::remove(path.c_str());
    ASSERT_EQ(cfg.loginRateLimitPerMinute, 10);
    return true;
}

bool test_config_FileExistsNonExistent() {
    ASSERT_FALSE(Config::fileExists("/tmp/opensylab_DOES_NOT_EXIST_88888.conf"));
    return true;
}

bool test_config_FileExistsExistingFile() {
    const std::string path = writeTempFile("api_port = 1234\n");
    const bool exists = Config::fileExists(path);
    std::remove(path.c_str());
    ASSERT_TRUE(exists);
    return true;
}

} // namespace

void registerConfigTests() {
    registerTest("Config::Defaults",                     test_config_Defaults);
    registerTest("Config::NonExistentFileReturnsDefaults", test_config_NonExistentFileReturnsDefaults);
    registerTest("Config::EmptyFileReturnsDefaults",     test_config_EmptyFileReturnsDefaults);
    registerTest("Config::DbPathRead",                   test_config_DbPathRead);
    registerTest("Config::ApiPortParsed",                test_config_ApiPortParsed);
    registerTest("Config::ForceHttpsTrue",               test_config_ForceHttpsTrue);
    registerTest("Config::ForceHttpsFalse",              test_config_ForceHttpsFalse);
    registerTest("Config::ForceHttpsOne",                test_config_ForceHttpsOne);
    registerTest("Config::ForceHttpsZero",               test_config_ForceHttpsZero);
    registerTest("Config::ForceHttpsYes",                test_config_ForceHttpsYes);
    registerTest("Config::ForceHttpsNo",                 test_config_ForceHttpsNo);
    registerTest("Config::CommentsIgnored",              test_config_CommentsIgnored);
    registerTest("Config::BlankLinesIgnored",            test_config_BlankLinesIgnored);
    registerTest("Config::UnknownKeysIgnored",           test_config_UnknownKeysIgnored);
    registerTest("Config::WhitespaceTrimmed",            test_config_WhitespaceTrimmed);
    registerTest("Config::AllKnownFields",               test_config_AllKnownFields);
    registerTest("Config::InvalidApiPortKeepsDefault",    test_config_InvalidApiPortKeepsDefault);
    registerTest("Config::InvalidRateLimitKeepsDefault",  test_config_InvalidRateLimitKeepsDefault);
    registerTest("Config::FileExistsNonExistent",         test_config_FileExistsNonExistent);
    registerTest("Config::FileExistsExistingFile",       test_config_FileExistsExistingFile);
}
