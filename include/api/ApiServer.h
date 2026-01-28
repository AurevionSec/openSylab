#ifndef OPENSYLAB_APISERVER_H
#define OPENSYLAB_APISERVER_H

#include "core/Order.h"
#include "core/Sample.h"
#include "core/TestResult.h"
#include "db/Database.h"
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>

namespace opensylab {
namespace api {

struct ApiRequest {
  std::string method;
  std::string path;
  std::unordered_map<std::string, std::string> headers;
};

struct ApiResponse {
  int status = 200;
  std::string body;
  std::string contentType = "application/json";
};

class ApiRouter {
public:
  explicit ApiRouter(std::shared_ptr<db::Database> database);

  ApiResponse handleRequest(const ApiRequest &request);

  static std::string sampleToJson(const core::Sample &sample);
  static std::string orderToJson(const core::Order &order);
  static std::string resultToJson(const core::TestResult &result);

private:
  std::shared_ptr<db::Database> database_;
};

class ApiServer {
public:
  ApiServer(std::shared_ptr<db::Database> database, int port = 8080);

  bool run();
  void stop();

private:
  bool bindAndListen();
  void serveLoop();
  void handleClient(int clientFd);

  std::shared_ptr<db::Database> database_;
  ApiRouter router_;
  int port_;
  int serverFd_;
  std::atomic<bool> running_;
};

} // namespace api
} // namespace opensylab

#endif // OPENSYLAB_APISERVER_H
