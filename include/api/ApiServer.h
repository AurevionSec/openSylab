#ifndef OPENSYLAB_APISERVER_H
#define OPENSYLAB_APISERVER_H

#include "core/Order.h"
#include "core/Sample.h"
#include "core/TestResult.h"
#include "db/IDatabase.h"
#include "api/TlsContext.h"
#include "auth/JwtAuth.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <openssl/ssl.h>

namespace opensylab {
namespace api {

struct ApiRequest {
  std::string method;
  std::string path;
  std::unordered_map<std::string, std::string> headers;
  std::string body;

  // JWT Authentication context (populated after successful auth)
  std::optional<int> userId;
  std::optional<std::string> username;
  std::optional<std::string> userRole;
};

struct ApiResponse {
  int status = 200;
  std::string body;
  std::string contentType = "application/json";
};

class ApiRouter {
public:
  explicit ApiRouter(std::shared_ptr<db::IDatabase> database);

  ApiResponse handleRequest(const ApiRequest &request);

  static std::string sampleToJson(const core::Sample &sample);
  static std::string orderToJson(const core::Order &order);
  static std::string resultToJson(const core::TestResult &result, const std::string &orderIdStr = "");

private:
  // Authentication handlers
  ApiResponse handleLogin(const ApiRequest &request);

  // JWT validation helper
  std::optional<auth::JwtAuth::TokenPayload>
  extractAndValidateJwt(const std::unordered_map<std::string, std::string> &headers);

  std::shared_ptr<db::IDatabase> database_;
  std::unique_ptr<auth::JwtAuth> jwtAuth_;
  mutable std::unordered_set<std::string> tokenBlacklist_;
  mutable std::mutex blacklistMutex_;
};

class ApiServer {
public:
  ApiServer(std::shared_ptr<db::IDatabase> database, int port = 8080);

  bool run();
  void stop();
  ~ApiServer();

  /**
   * @brief Enable TLS/HTTPS support
   *
   * @param certPath Path to PEM-encoded certificate file
   * @param keyPath Path to PEM-encoded private key file
   * @return true if TLS enabled successfully, false otherwise
   */
  bool enableTls(const std::string& certPath, const std::string& keyPath);

  /**
   * @brief Disable TLS (use plain HTTP)
   */
  void disableTls();

  /**
   * @brief Check if TLS is enabled
   *
   * @return true if TLS enabled, false otherwise
   */
  bool isTlsEnabled() const;

  /**
   * @brief Get last TLS error message
   *
   * @return Error message string
   */
  std::string getTlsError() const;

private:
  bool bindAndListen();
  void serveLoop();
  void handleClient(int clientFd);
  void handleClientTls(int clientFd);
  void handleClientPlain(int clientFd);

  // Thread-pool configuration
  static constexpr int kMaxThreads = 32;

  std::shared_ptr<db::IDatabase> database_;
  ApiRouter router_;
  int port_;
  int serverFd_;
  std::atomic<bool> running_;
  std::atomic<int> activeConnections_{0};
  std::unique_ptr<TlsContext> tlsContext_;
  bool tlsEnabled_;
  std::string corsOrigin_;

  // Rate limiting: IP → (attempts, window_start)
  std::unordered_map<std::string, std::pair<int, std::chrono::steady_clock::time_point>> loginAttempts_;
  std::mutex loginMutex_;
  bool isRateLimited(const std::string &ip);
};

} // namespace api
} // namespace opensylab

#endif // OPENSYLAB_APISERVER_H
