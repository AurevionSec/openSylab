#include "api/ApiServer.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace opensylab {
namespace api {
namespace {

std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::string trim(const std::string &value) {
  const auto start = value.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  const auto end = value.find_last_not_of(" \t\r\n");
  return value.substr(start, end - start + 1);
}

std::string urlDecode(const std::string &value) {
  std::string result;
  result.reserve(value.size());
  for (size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '%' && i + 2 < value.size()) {
      const std::string hex = value.substr(i + 1, 2);
      char *end = nullptr;
      const long decoded = std::strtol(hex.c_str(), &end, 16);
      if (end && *end == '\0') {
        result.push_back(static_cast<char>(decoded));
        i += 2;
        continue;
      }
    } else if (value[i] == '+') {
      result.push_back(' ');
      continue;
    }
    result.push_back(value[i]);
  }
  return result;
}

std::unordered_map<std::string, std::string> parseQuery(const std::string &query) {
  std::unordered_map<std::string, std::string> params;
  size_t start = 0;
  while (start < query.size()) {
    size_t end = query.find('&', start);
    if (end == std::string::npos) {
      end = query.size();
    }
    const std::string pair = query.substr(start, end - start);
    const size_t eq = pair.find('=');
    if (eq != std::string::npos) {
      const std::string key = urlDecode(pair.substr(0, eq));
      const std::string value = urlDecode(pair.substr(eq + 1));
      if (!key.empty()) {
        params[toLower(key)] = value;
      }
    } else if (!pair.empty()) {
      params[toLower(pair)] = "";
    }
    start = end + 1;
  }
  return params;
}

std::string escapeJson(const std::string &value) {
  std::ostringstream out;
  for (char ch : value) {
    switch (ch) {
    case '\"':
      out << "\\\"";
      break;
    case '\\':
      out << "\\\\";
      break;
    case '\b':
      out << "\\b";
      break;
    case '\f':
      out << "\\f";
      break;
    case '\n':
      out << "\\n";
      break;
    case '\r':
      out << "\\r";
      break;
    case '\t':
      out << "\\t";
      break;
    default:
      if (static_cast<unsigned char>(ch) < 0x20) {
        out << "\\u"
            << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
            << static_cast<int>(static_cast<unsigned char>(ch))
            << std::dec << std::nouppercase;
      } else {
        out << ch;
      }
    }
  }
  return out.str();
}

std::string jsonString(const std::string &value) {
  return "\"" + escapeJson(value) + "\"";
}

std::string statusMessage(int status) {
  switch (status) {
  case 200:
    return "OK";
  case 400:
    return "Bad Request";
  case 401:
    return "Unauthorized";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 409:
    return "Conflict";
  case 500:
    return "Internal Server Error";
  default:
    return "Error";
  }
}

ApiResponse makeError(int status, const std::string &code,
                      const std::string &message,
                      const std::string &hint) {
  ApiResponse response;
  response.status = status;
  std::ostringstream body;
  body << "{"
       << "\"error\":{"
       << "\"code\":" << jsonString(code) << ","
       << "\"message\":" << jsonString(message) << ","
       << "\"hint\":" << jsonString(hint)
       << "}"
       << "}";
  response.body = body.str();
  return response;
}

ApiResponse makeDbErrorResponse(const std::string &message) {
  const std::string lower = toLower(message);
  if (lower.find("nicht gefunden") != std::string::npos) {
    return makeError(404, "not_found", "Record not found", message);
  }
  if (lower.find("unique constraint failed") != std::string::npos) {
    return makeError(409, "conflict", "Duplicate record", message);
  }
  if (lower.find("darf nicht leer") != std::string::npos ||
      lower.find("ung\u00fcltig") != std::string::npos) {
    return makeError(400, "validation_error", "Invalid input", message);
  }
  return makeError(500, "internal_error", "Database error", message);
}

bool parseIntValue(const std::string &value, int &out) {
  try {
    size_t idx = 0;
    int parsed = std::stoi(value, &idx);
    if (idx != value.size()) {
      return false;
    }
    out = parsed;
    return true;
  } catch (...) {
    return false;
  }
}

bool parseDoubleValue(const std::string &value, double &out) {
  try {
    size_t idx = 0;
    double parsed = std::stod(value, &idx);
    if (idx != value.size()) {
      return false;
    }
    out = parsed;
    return true;
  } catch (...) {
    return false;
  }
}

bool parseTimeValue(const std::string &value, std::time_t &out) {
  try {
    size_t idx = 0;
    long long parsed = std::stoll(value, &idx);
    if (idx != value.size()) {
      return false;
    }
    out = static_cast<std::time_t>(parsed);
    return true;
  } catch (...) {
    return false;
  }
}

std::string extractApiKey(const std::unordered_map<std::string, std::string> &headers) {
  auto it = headers.find("x-api-key");
  if (it != headers.end()) {
    return trim(it->second);
  }
  it = headers.find("authorization");
  if (it != headers.end()) {
    const std::string value = trim(it->second);
    const std::string prefix = "bearer ";
    if (value.size() > prefix.size() &&
        toLower(value.substr(0, prefix.size())) == prefix) {
      return trim(value.substr(prefix.size()));
    }
  }
  return "";
}

std::string sanitizeActor(const std::string &apiKey) {
  if (apiKey.empty()) {
    return "api-key:unknown";
  }
  const size_t keep = std::min<size_t>(6, apiKey.size());
  return "api-key:" + apiKey.substr(0, keep);
}

std::string trimLeadingNewlines(const std::string &value) {
  size_t start = 0;
  while (start < value.size() &&
         (value[start] == '\r' || value[start] == '\n')) {
    ++start;
  }
  return value.substr(start);
}

void skipWhitespace(const std::string &input, size_t &pos) {
  while (pos < input.size() &&
         std::isspace(static_cast<unsigned char>(input[pos])) != 0) {
    ++pos;
  }
}

bool parseJsonString(const std::string &input, size_t &pos, std::string &out,
                     std::string &error) {
  if (pos >= input.size() || input[pos] != '"') {
    error = "Expected string";
    return false;
  }
  ++pos;
  std::ostringstream value;
  while (pos < input.size()) {
    char ch = input[pos++];
    if (ch == '"') {
      out = value.str();
      return true;
    }
    if (ch == '\\') {
      if (pos >= input.size()) {
        error = "Invalid escape sequence";
        return false;
      }
      char esc = input[pos++];
      switch (esc) {
      case '"':
        value << '"';
        break;
      case '\\':
        value << '\\';
        break;
      case '/':
        value << '/';
        break;
      case 'b':
        value << '\b';
        break;
      case 'f':
        value << '\f';
        break;
      case 'n':
        value << '\n';
        break;
      case 'r':
        value << '\r';
        break;
      case 't':
        value << '\t';
        break;
      case 'u': {
        if (pos + 4 > input.size()) {
          error = "Invalid unicode escape";
          return false;
        }
        unsigned int code = 0;
        for (int i = 0; i < 4; ++i) {
          char hex = input[pos++];
          code <<= 4;
          if (hex >= '0' && hex <= '9') {
            code += static_cast<unsigned int>(hex - '0');
          } else if (hex >= 'a' && hex <= 'f') {
            code += static_cast<unsigned int>(hex - 'a' + 10);
          } else if (hex >= 'A' && hex <= 'F') {
            code += static_cast<unsigned int>(hex - 'A' + 10);
          } else {
            error = "Invalid unicode escape";
            return false;
          }
        }
        value << (code <= 0x7F ? static_cast<char>(code) : '?');
        break;
      }
      default:
        error = "Unsupported escape sequence";
        return false;
      }
      continue;
    }
    value << ch;
  }
  error = "Unterminated string";
  return false;
}

bool parseJsonValue(const std::string &input, size_t &pos, std::string &out,
                    std::string &error) {
  skipWhitespace(input, pos);
  if (pos >= input.size()) {
    error = "Unexpected end of JSON";
    return false;
  }
  if (input[pos] == '"') {
    return parseJsonString(input, pos, out, error);
  }
  if (input.compare(pos, 4, "true") == 0) {
    out = "true";
    pos += 4;
    return true;
  }
  if (input.compare(pos, 5, "false") == 0) {
    out = "false";
    pos += 5;
    return true;
  }
  if (input.compare(pos, 4, "null") == 0) {
    out = "null";
    pos += 4;
    return true;
  }
  if (input[pos] == '-' || std::isdigit(static_cast<unsigned char>(input[pos])) != 0) {
    size_t start = pos;
    while (pos < input.size()) {
      char ch = input[pos];
      if (std::isdigit(static_cast<unsigned char>(ch)) != 0 || ch == '-' ||
          ch == '+' || ch == '.' || ch == 'e' || ch == 'E') {
        ++pos;
        continue;
      }
      break;
    }
    if (start == pos) {
      error = "Invalid number";
      return false;
    }
    out = trim(input.substr(start, pos - start));
    return true;
  }
  error = "Unsupported JSON value";
  return false;
}

bool parseJsonObject(const std::string &input,
                     std::unordered_map<std::string, std::string> &out,
                     std::string &error) {
  size_t pos = 0;
  skipWhitespace(input, pos);
  if (pos >= input.size() || input[pos] != '{') {
    error = "Expected JSON object";
    return false;
  }
  ++pos;
  skipWhitespace(input, pos);
  if (pos < input.size() && input[pos] == '}') {
    ++pos;
    return true;
  }
  while (pos < input.size()) {
    std::string key;
    if (!parseJsonString(input, pos, key, error)) {
      return false;
    }
    skipWhitespace(input, pos);
    if (pos >= input.size() || input[pos] != ':') {
      error = "Expected ':' after key";
      return false;
    }
    ++pos;
    std::string value;
    if (!parseJsonValue(input, pos, value, error)) {
      return false;
    }
    out[toLower(key)] = value;
    skipWhitespace(input, pos);
    if (pos >= input.size()) {
      error = "Unexpected end of JSON";
      return false;
    }
    if (input[pos] == ',') {
      ++pos;
      skipWhitespace(input, pos);
      continue;
    }
    if (input[pos] == '}') {
      ++pos;
      return true;
    }
    error = "Expected ',' or '}'";
    return false;
  }
  error = "Unexpected end of JSON";
  return false;
}

} // namespace

ApiRouter::ApiRouter(std::shared_ptr<db::Database> database)
    : database_(std::move(database)) {}

std::string ApiRouter::sampleToJson(const core::Sample &sample) {
  std::ostringstream out;
  out << "{"
      << "\"id\":" << sample.getId() << ","
      << "\"sample_id\":" << jsonString(sample.getSampleId()) << ","
      << "\"patient_id\":" << jsonString(sample.getPatientId()) << ","
      << "\"patient_name\":" << jsonString(sample.getPatientName()) << ","
      << "\"description\":" << jsonString(sample.getDescription()) << ","
      << "\"status\":" << jsonString(sample.getStatusString()) << ","
      << "\"registration_date\":" << static_cast<long long>(sample.getRegistrationDate())
      << "}";
  return out.str();
}

std::string ApiRouter::orderToJson(const core::Order &order) {
  std::ostringstream out;
  out << "{"
      << "\"id\":" << order.getId() << ","
      << "\"order_id\":" << jsonString(order.getOrderId()) << ","
      << "\"sample_id\":" << jsonString(order.getSampleId()) << ","
      << "\"test_type\":" << jsonString(order.getTestType()) << ","
      << "\"status\":" << jsonString(order.getStatusString()) << ","
      << "\"priority\":" << jsonString(order.getPriorityString()) << ","
      << "\"requested_date\":" << static_cast<long long>(order.getRequestedDate()) << ","
      << "\"completed_date\":" << static_cast<long long>(order.getCompletedDate()) << ","
      << "\"requested_by\":" << jsonString(order.getRequestedBy()) << ","
      << "\"notes\":" << jsonString(order.getNotes())
      << "}";
  return out.str();
}

std::string ApiRouter::resultToJson(const core::TestResult &result) {
  std::ostringstream out;
  out << "{"
      << "\"id\":" << result.getId() << ","
      << "\"result_id\":" << jsonString(result.getResultId()) << ","
      << "\"order_id\":" << result.getOrderId() << ","
      << "\"test_parameter\":" << jsonString(result.getTestParameter()) << ","
      << "\"value\":" << jsonString(result.getValue()) << ","
      << "\"unit\":" << jsonString(result.getUnit()) << ","
      << "\"reference_range\":" << jsonString(result.getReferenceRange()) << ","
      << "\"reference_low\":" << result.getReferenceLow() << ","
      << "\"reference_high\":" << result.getReferenceHigh() << ","
      << "\"status\":" << jsonString(result.getStatusString()) << ","
      << "\"flag\":" << jsonString(result.getFlagString()) << ","
      << "\"measured_date\":" << static_cast<long long>(result.getMeasuredDate()) << ","
      << "\"measured_by\":" << jsonString(result.getMeasuredBy()) << ","
      << "\"comment\":" << jsonString(result.getComment())
      << "}";
  return out.str();
}

ApiResponse ApiRouter::handleRequest(const ApiRequest &request) {
  if (!database_) {
    return makeError(500, "internal_error", "Database unavailable",
                     "Database instance is not configured.");
  }

  const std::string apiKey = extractApiKey(request.headers);
  if (apiKey.empty() || !database_->isApiKeyValid(apiKey)) {
    return makeError(401, "unauthorized", "Invalid API credentials",
                     "Provide X-API-Key or Authorization: Bearer <token>.");
  }

  const std::string method = toLower(request.method);
  const bool isGet = method == "get";
  const bool isPost = method == "post";
  const bool isPut = method == "put";
  if (!isGet && !isPost && !isPut) {
    return makeError(405, "validation_error", "Method not allowed",
                     "Use GET for read endpoints or POST/PUT for writes.");
  }

  std::string path = request.path;
  std::string queryString;
  const size_t qpos = path.find('?');
  if (qpos != std::string::npos) {
    queryString = path.substr(qpos + 1);
    path = path.substr(0, qpos);
  }

  const std::unordered_map<std::string, std::string> query =
      parseQuery(queryString);

  const std::string actor = sanitizeActor(apiKey);

  if (isPost || isPut) {
    const std::string body = trimLeadingNewlines(request.body);
    if (body.empty()) {
      return makeError(400, "validation_error", "Missing request body",
                       "Provide JSON payload in request body.");
    }

    std::unordered_map<std::string, std::string> payload;
    std::string parseError;
    if (!parseJsonObject(body, payload, parseError)) {
      return makeError(400, "validation_error", "Invalid JSON payload",
                       parseError);
    }

    if (path == "/api/v1/samples" && isPost) {
      if (payload.find("sample_id") == payload.end() ||
          payload["sample_id"].empty()) {
        return makeError(400, "validation_error", "Missing sample_id",
                         "Provide sample_id in request body.");
      }
      if (payload.find("patient_id") == payload.end() ||
          payload["patient_id"].empty()) {
        return makeError(400, "validation_error", "Missing patient_id",
                         "Provide patient_id in request body.");
      }

      core::Sample sample(payload["sample_id"], payload["patient_id"]);
      auto nameIt = payload.find("patient_name");
      if (nameIt != payload.end()) {
        sample.setPatientName(nameIt->second);
      }
      auto descIt = payload.find("description");
      if (descIt != payload.end()) {
        sample.setDescription(descIt->second);
      }
      auto statusIt = payload.find("status");
      if (statusIt != payload.end() && !statusIt->second.empty()) {
        try {
          sample.setStatus(core::Sample::stringToStatus(statusIt->second));
        } catch (const std::exception &e) {
          return makeError(400, "validation_error", "Invalid status",
                           e.what());
        }
      }
      auto regIt = payload.find("registration_date");
      if (regIt != payload.end() && !regIt->second.empty()) {
        std::time_t ts{};
        if (!parseTimeValue(regIt->second, ts)) {
          return makeError(400, "validation_error",
                           "Invalid registration_date",
                           "Provide Unix timestamp for registration_date.");
        }
        sample.setRegistrationDate(ts);
      }

      if (!database_->createSample(sample, actor)) {
        return makeDbErrorResponse(database_->getLastError());
      }

      auto created = database_->getSampleByBarcode(sample.getSampleId());
      if (database_->hasError()) {
        return makeDbErrorResponse(database_->getLastError());
      }
      const core::Sample &responseSample = created ? *created : sample;
      return ApiResponse{200,
                         "{\"data\":" + sampleToJson(responseSample) + "}",
                         "application/json"};
    }

    if (path == "/api/v1/orders" && isPost) {
      if (payload.find("order_id") == payload.end() ||
          payload["order_id"].empty()) {
        return makeError(400, "validation_error", "Missing order_id",
                         "Provide order_id in request body.");
      }
      if (payload.find("sample_id") == payload.end() ||
          payload["sample_id"].empty()) {
        return makeError(400, "validation_error", "Missing sample_id",
                         "Provide sample_id in request body.");
      }
      if (payload.find("test_type") == payload.end() ||
          payload["test_type"].empty()) {
        return makeError(400, "validation_error", "Missing test_type",
                         "Provide test_type in request body.");
      }

      core::Order order(payload["order_id"], payload["sample_id"],
                        payload["test_type"]);
      auto statusIt = payload.find("status");
      if (statusIt != payload.end() && !statusIt->second.empty()) {
        try {
          order.setStatus(core::Order::stringToStatus(statusIt->second));
        } catch (const std::exception &e) {
          return makeError(400, "validation_error", "Invalid status",
                           e.what());
        }
      }
      auto priorityIt = payload.find("priority");
      if (priorityIt != payload.end() && !priorityIt->second.empty()) {
        try {
          order.setPriority(core::Order::stringToPriority(priorityIt->second));
        } catch (const std::exception &e) {
          return makeError(400, "validation_error", "Invalid priority",
                           e.what());
        }
      }
      auto requestedIt = payload.find("requested_date");
      if (requestedIt != payload.end() && !requestedIt->second.empty()) {
        std::time_t ts{};
        if (!parseTimeValue(requestedIt->second, ts)) {
          return makeError(400, "validation_error",
                           "Invalid requested_date",
                           "Provide Unix timestamp for requested_date.");
        }
        order.setRequestedDate(ts);
      }
      auto completedIt = payload.find("completed_date");
      if (completedIt != payload.end() && !completedIt->second.empty()) {
        std::time_t ts{};
        if (!parseTimeValue(completedIt->second, ts)) {
          return makeError(400, "validation_error",
                           "Invalid completed_date",
                           "Provide Unix timestamp for completed_date.");
        }
        order.setCompletedDate(ts);
      }
      auto requesterIt = payload.find("requested_by");
      if (requesterIt != payload.end()) {
        order.setRequestedBy(requesterIt->second);
      }
      auto notesIt = payload.find("notes");
      if (notesIt != payload.end()) {
        order.setNotes(notesIt->second);
      }

      if (!database_->createOrder(order, actor)) {
        return makeDbErrorResponse(database_->getLastError());
      }

      auto created = database_->getOrderByOrderId(order.getOrderId());
      if (database_->hasError()) {
        return makeDbErrorResponse(database_->getLastError());
      }
      const core::Order &responseOrder = created ? *created : order;
      return ApiResponse{200,
                         "{\"data\":" + orderToJson(responseOrder) + "}",
                         "application/json"};
    }

    if (path == "/api/v1/results" && isPost) {
      if (payload.find("result_id") == payload.end() ||
          payload["result_id"].empty()) {
        return makeError(400, "validation_error", "Missing result_id",
                         "Provide result_id in request body.");
      }
      if (payload.find("order_id") == payload.end() ||
          payload["order_id"].empty()) {
        return makeError(400, "validation_error", "Missing order_id",
                         "Provide order_id in request body.");
      }
      if (payload.find("test_parameter") == payload.end() ||
          payload["test_parameter"].empty()) {
        return makeError(400, "validation_error",
                         "Missing test_parameter",
                         "Provide test_parameter in request body.");
      }
      if (payload.find("value") == payload.end() ||
          payload["value"].empty()) {
        return makeError(400, "validation_error", "Missing value",
                         "Provide value in request body.");
      }
      if (payload.find("unit") == payload.end() ||
          payload["unit"].empty()) {
        return makeError(400, "validation_error", "Missing unit",
                         "Provide unit in request body.");
      }

      int orderId = 0;
      if (!parseIntValue(payload["order_id"], orderId) || orderId <= 0) {
        return makeError(400, "validation_error", "Invalid order_id",
                         "Provide numeric order_id.");
      }

      core::TestResult result(payload["result_id"], orderId,
                              payload["test_parameter"]);
      result.setValue(payload["value"]);
      result.setUnit(payload["unit"]);
      auto refRangeIt = payload.find("reference_range");
      if (refRangeIt != payload.end()) {
        result.setReferenceRange(refRangeIt->second);
      }
      auto refLowIt = payload.find("reference_low");
      if (refLowIt != payload.end() && !refLowIt->second.empty()) {
        double refLow = 0.0;
        if (!parseDoubleValue(refLowIt->second, refLow)) {
          return makeError(400, "validation_error", "Invalid reference_low",
                           "Provide numeric reference_low.");
        }
        result.setReferenceLow(refLow);
      }
      auto refHighIt = payload.find("reference_high");
      if (refHighIt != payload.end() && !refHighIt->second.empty()) {
        double refHigh = 0.0;
        if (!parseDoubleValue(refHighIt->second, refHigh)) {
          return makeError(400, "validation_error", "Invalid reference_high",
                           "Provide numeric reference_high.");
        }
        result.setReferenceHigh(refHigh);
      }
      auto statusIt = payload.find("status");
      if (statusIt != payload.end() && !statusIt->second.empty()) {
        try {
          result.setStatus(
              core::TestResult::stringToStatus(statusIt->second));
        } catch (const std::exception &e) {
          return makeError(400, "validation_error", "Invalid status",
                           e.what());
        }
      } else {
        result.setStatus(core::TestResult::Status::ENTERED);
      }
      auto measuredIt = payload.find("measured_date");
      if (measuredIt != payload.end() && !measuredIt->second.empty()) {
        std::time_t ts{};
        if (!parseTimeValue(measuredIt->second, ts)) {
          return makeError(400, "validation_error", "Invalid measured_date",
                           "Provide Unix timestamp for measured_date.");
        }
        result.setMeasuredDate(ts);
      } else {
        result.setMeasuredDate(std::time(nullptr));
      }
      auto measuredByIt = payload.find("measured_by");
      if (measuredByIt != payload.end()) {
        result.setMeasuredBy(measuredByIt->second);
      }
      auto commentIt = payload.find("comment");
      if (commentIt != payload.end()) {
        result.setComment(commentIt->second);
      }
      auto flagIt = payload.find("flag");
      if (flagIt != payload.end() && !flagIt->second.empty()) {
        try {
          result.setFlag(core::TestResult::stringToFlag(flagIt->second));
        } catch (const std::exception &e) {
          return makeError(400, "validation_error", "Invalid flag", e.what());
        }
      } else {
        result.setFlag(result.evaluateFlag());
      }

      if (!database_->createTestResult(result, actor)) {
        return makeDbErrorResponse(database_->getLastError());
      }

      auto created = database_->getTestResultByResultId(result.getResultId());
      if (database_->hasError()) {
        return makeDbErrorResponse(database_->getLastError());
      }
      const core::TestResult &responseResult = created ? *created : result;
      return ApiResponse{200,
                         "{\"data\":" + resultToJson(responseResult) + "}",
                         "application/json"};
    }

    if (isPut && path.rfind("/api/v1/samples/", 0) == 0) {
      const std::string sampleId =
          path.substr(std::string("/api/v1/samples/").size());
      if (sampleId.empty()) {
        return makeError(400, "validation_error", "Missing sample_id",
                         "Provide sample_id in URL path.");
      }
      auto existing = database_->getSampleByBarcode(sampleId);
      if (database_->hasError()) {
        return makeDbErrorResponse(database_->getLastError());
      }
      if (!existing) {
        return makeError(404, "not_found", "Sample not found",
                         "Verify the sample_id.");
      }

      auto idIt = payload.find("sample_id");
      if (idIt != payload.end() && idIt->second != sampleId) {
        return makeError(409, "conflict", "sample_id mismatch",
                         "sample_id in body must match URL.");
      }
      auto patientIt = payload.find("patient_id");
      if (patientIt == payload.end() || patientIt->second.empty()) {
        return makeError(400, "validation_error", "Missing patient_id",
                         "Provide patient_id in request body.");
      }

      core::Sample updated = *existing;
      updated.setSampleId(sampleId);
      updated.setPatientId(patientIt->second);
      auto nameIt = payload.find("patient_name");
      if (nameIt != payload.end()) {
        updated.setPatientName(nameIt->second);
      }
      auto descIt = payload.find("description");
      if (descIt != payload.end()) {
        updated.setDescription(descIt->second);
      }
      auto statusIt = payload.find("status");
      if (statusIt != payload.end() && !statusIt->second.empty()) {
        try {
          updated.setStatus(core::Sample::stringToStatus(statusIt->second));
        } catch (const std::exception &e) {
          return makeError(400, "validation_error", "Invalid status",
                           e.what());
        }
      }
      auto regIt = payload.find("registration_date");
      if (regIt != payload.end() && !regIt->second.empty()) {
        std::time_t ts{};
        if (!parseTimeValue(regIt->second, ts)) {
          return makeError(400, "validation_error",
                           "Invalid registration_date",
                           "Provide Unix timestamp for registration_date.");
        }
        updated.setRegistrationDate(ts);
      }

      if (!database_->updateSample(updated, actor)) {
        return makeDbErrorResponse(database_->getLastError());
      }
      return ApiResponse{200,
                         "{\"data\":" + sampleToJson(updated) + "}",
                         "application/json"};
    }

    if (isPut && path.rfind("/api/v1/orders/", 0) == 0) {
      const std::string orderId =
          path.substr(std::string("/api/v1/orders/").size());
      if (orderId.empty()) {
        return makeError(400, "validation_error", "Missing order_id",
                         "Provide order_id in URL path.");
      }
      auto existing = database_->getOrderByOrderId(orderId);
      if (database_->hasError()) {
        return makeDbErrorResponse(database_->getLastError());
      }
      if (!existing) {
        return makeError(404, "not_found", "Order not found",
                         "Verify the order_id.");
      }

      auto idIt = payload.find("order_id");
      if (idIt != payload.end() && idIt->second != orderId) {
        return makeError(409, "conflict", "order_id mismatch",
                         "order_id in body must match URL.");
      }
      auto sampleIt = payload.find("sample_id");
      if (sampleIt == payload.end() || sampleIt->second.empty()) {
        return makeError(400, "validation_error", "Missing sample_id",
                         "Provide sample_id in request body.");
      }
      auto testIt = payload.find("test_type");
      if (testIt == payload.end() || testIt->second.empty()) {
        return makeError(400, "validation_error", "Missing test_type",
                         "Provide test_type in request body.");
      }

      core::Order updated = *existing;
      updated.setOrderId(orderId);
      updated.setSampleId(sampleIt->second);
      updated.setTestType(testIt->second);
      auto statusIt = payload.find("status");
      if (statusIt != payload.end() && !statusIt->second.empty()) {
        try {
          updated.setStatus(core::Order::stringToStatus(statusIt->second));
        } catch (const std::exception &e) {
          return makeError(400, "validation_error", "Invalid status",
                           e.what());
        }
      }
      auto priorityIt = payload.find("priority");
      if (priorityIt != payload.end() && !priorityIt->second.empty()) {
        try {
          updated.setPriority(core::Order::stringToPriority(priorityIt->second));
        } catch (const std::exception &e) {
          return makeError(400, "validation_error", "Invalid priority",
                           e.what());
        }
      }
      auto requestedIt = payload.find("requested_date");
      if (requestedIt != payload.end() && !requestedIt->second.empty()) {
        std::time_t ts{};
        if (!parseTimeValue(requestedIt->second, ts)) {
          return makeError(400, "validation_error",
                           "Invalid requested_date",
                           "Provide Unix timestamp for requested_date.");
        }
        updated.setRequestedDate(ts);
      }
      auto completedIt = payload.find("completed_date");
      if (completedIt != payload.end() && !completedIt->second.empty()) {
        std::time_t ts{};
        if (!parseTimeValue(completedIt->second, ts)) {
          return makeError(400, "validation_error",
                           "Invalid completed_date",
                           "Provide Unix timestamp for completed_date.");
        }
        updated.setCompletedDate(ts);
      }
      auto requesterIt = payload.find("requested_by");
      if (requesterIt != payload.end()) {
        updated.setRequestedBy(requesterIt->second);
      }
      auto notesIt = payload.find("notes");
      if (notesIt != payload.end()) {
        updated.setNotes(notesIt->second);
      }

      if (!database_->updateOrder(updated, actor)) {
        return makeDbErrorResponse(database_->getLastError());
      }
      return ApiResponse{200,
                         "{\"data\":" + orderToJson(updated) + "}",
                         "application/json"};
    }

    if (isPut && path.rfind("/api/v1/results/", 0) == 0) {
      const std::string resultId =
          path.substr(std::string("/api/v1/results/").size());
      if (resultId.empty()) {
        return makeError(400, "validation_error", "Missing result_id",
                         "Provide result_id in URL path.");
      }
      auto existing = database_->getTestResultByResultId(resultId);
      if (database_->hasError()) {
        return makeDbErrorResponse(database_->getLastError());
      }
      if (!existing) {
        return makeError(404, "not_found", "Result not found",
                         "Verify the result_id.");
      }

      auto idIt = payload.find("result_id");
      if (idIt != payload.end() && idIt->second != resultId) {
        return makeError(409, "conflict", "result_id mismatch",
                         "result_id in body must match URL.");
      }
      auto paramIt = payload.find("test_parameter");
      if (paramIt == payload.end() || paramIt->second.empty()) {
        return makeError(400, "validation_error",
                         "Missing test_parameter",
                         "Provide test_parameter in request body.");
      }
      auto valueIt = payload.find("value");
      if (valueIt == payload.end() || valueIt->second.empty()) {
        return makeError(400, "validation_error", "Missing value",
                         "Provide value in request body.");
      }
      auto unitIt = payload.find("unit");
      if (unitIt == payload.end() || unitIt->second.empty()) {
        return makeError(400, "validation_error", "Missing unit",
                         "Provide unit in request body.");
      }

      core::TestResult updated = *existing;
      updated.setResultId(resultId);
      updated.setTestParameter(paramIt->second);
      updated.setValue(valueIt->second);
      updated.setUnit(unitIt->second);
      auto orderIt = payload.find("order_id");
      if (orderIt != payload.end() && !orderIt->second.empty()) {
        int orderId = 0;
        if (!parseIntValue(orderIt->second, orderId) || orderId <= 0) {
          return makeError(400, "validation_error", "Invalid order_id",
                           "Provide numeric order_id.");
        }
        updated.setOrderId(orderId);
      }
      auto refRangeIt = payload.find("reference_range");
      if (refRangeIt != payload.end()) {
        updated.setReferenceRange(refRangeIt->second);
      }
      auto refLowIt = payload.find("reference_low");
      if (refLowIt != payload.end() && !refLowIt->second.empty()) {
        double refLow = 0.0;
        if (!parseDoubleValue(refLowIt->second, refLow)) {
          return makeError(400, "validation_error", "Invalid reference_low",
                           "Provide numeric reference_low.");
        }
        updated.setReferenceLow(refLow);
      }
      auto refHighIt = payload.find("reference_high");
      if (refHighIt != payload.end() && !refHighIt->second.empty()) {
        double refHigh = 0.0;
        if (!parseDoubleValue(refHighIt->second, refHigh)) {
          return makeError(400, "validation_error", "Invalid reference_high",
                           "Provide numeric reference_high.");
        }
        updated.setReferenceHigh(refHigh);
      }
      auto statusIt = payload.find("status");
      if (statusIt != payload.end() && !statusIt->second.empty()) {
        try {
          updated.setStatus(
              core::TestResult::stringToStatus(statusIt->second));
        } catch (const std::exception &e) {
          return makeError(400, "validation_error", "Invalid status",
                           e.what());
        }
      }
      auto measuredIt = payload.find("measured_date");
      if (measuredIt != payload.end() && !measuredIt->second.empty()) {
        std::time_t ts{};
        if (!parseTimeValue(measuredIt->second, ts)) {
          return makeError(400, "validation_error", "Invalid measured_date",
                           "Provide Unix timestamp for measured_date.");
        }
        updated.setMeasuredDate(ts);
      }
      auto measuredByIt = payload.find("measured_by");
      if (measuredByIt != payload.end()) {
        updated.setMeasuredBy(measuredByIt->second);
      }
      auto commentIt = payload.find("comment");
      if (commentIt != payload.end()) {
        updated.setComment(commentIt->second);
      }
      auto flagIt = payload.find("flag");
      if (flagIt != payload.end() && !flagIt->second.empty()) {
        try {
          updated.setFlag(core::TestResult::stringToFlag(flagIt->second));
        } catch (const std::exception &e) {
          return makeError(400, "validation_error", "Invalid flag", e.what());
        }
      } else {
        updated.setFlag(updated.evaluateFlag());
      }

      if (!database_->updateTestResult(updated, actor)) {
        return makeDbErrorResponse(database_->getLastError());
      }
      return ApiResponse{200,
                         "{\"data\":" + resultToJson(updated) + "}",
                         "application/json"};
    }
  }

  if (!isGet) {
    return makeError(405, "validation_error", "Method not allowed",
                     "Use POST/PUT for write endpoints.");
  }

  if (path == "/api/v1/samples") {
    db::Database::SampleFilter filter;
    auto qIt = query.find("q");
    if (qIt != query.end()) {
      filter.query = qIt->second;
    }
    auto statusIt = query.find("status");
    if (statusIt != query.end()) {
      filter.status = statusIt->second;
    }
    auto fromIt = query.find("from");
    if (fromIt != query.end()) {
      std::time_t ts{};
      if (!parseTimeValue(fromIt->second, ts)) {
        return makeError(400, "validation_error", "Invalid 'from' value",
                         "Provide Unix timestamp for from.");
      }
      filter.fromDate = ts;
    }
    auto toIt = query.find("to");
    if (toIt != query.end()) {
      std::time_t ts{};
      if (!parseTimeValue(toIt->second, ts)) {
        return makeError(400, "validation_error", "Invalid 'to' value",
                         "Provide Unix timestamp for to.");
      }
      filter.toDate = ts;
    }

    auto samples = database_->getSamplesByFilter(filter);
    if (database_->hasError()) {
      return makeError(500, "internal_error", database_->getLastError(),
                       "Check server logs for details.");
    }

    std::ostringstream out;
    out << "{\"data\":[";
    for (size_t i = 0; i < samples.size(); ++i) {
      if (i > 0) {
        out << ",";
      }
      out << sampleToJson(*samples[i]);
    }
    out << "]}";

    std::ostringstream details;
    details << "API READ /samples"
            << "; count=" << samples.size()
            << "; q=" << (filter.query.empty() ? "any" : filter.query)
            << "; status=" << (filter.status.empty() ? "any" : filter.status)
            << "; from=" << (filter.fromDate.has_value()
                                 ? std::to_string(*filter.fromDate)
                                 : "any")
            << "; to=" << (filter.toDate.has_value()
                               ? std::to_string(*filter.toDate)
                               : "any");
    core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                           core::AuditEntry::EntityType::SAMPLE, "*",
                           actor, details.str());
    if (!database_->logAudit(entry)) {
      return makeError(500, "internal_error", "Audit log failed",
                       database_->getLastError());
    }

    return ApiResponse{200, out.str(), "application/json"};
  }

  if (path.rfind("/api/v1/samples/", 0) == 0) {
    const std::string sampleId =
        path.substr(std::string("/api/v1/samples/").size());
    if (sampleId.empty()) {
      return makeError(400, "validation_error", "Missing sample_id",
                       "Provide sample_id in URL path.");
    }
    auto sample = database_->getSampleByBarcode(sampleId);
    if (database_->hasError()) {
      return makeError(500, "internal_error", database_->getLastError(),
                       "Check server logs for details.");
    }
    if (!sample) {
      return makeError(404, "not_found", "Sample not found",
                       "Verify the sample_id.");
    }

    core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                           core::AuditEntry::EntityType::SAMPLE, sampleId,
                           actor, "API READ /samples/" + sampleId);
    if (!database_->logAudit(entry)) {
      return makeError(500, "internal_error", "Audit log failed",
                       database_->getLastError());
    }

    return ApiResponse{200, "{\"data\":" + sampleToJson(*sample) + "}",
                       "application/json"};
  }

  if (path == "/api/v1/orders") {
    db::Database::OrderFilter filter;
    auto statusIt = query.find("status");
    if (statusIt != query.end()) {
      filter.status = statusIt->second;
    }
    auto sampleIt = query.find("sample_id");
    if (sampleIt != query.end()) {
      filter.sampleId = sampleIt->second;
    }
    auto priorityIt = query.find("priority");
    if (priorityIt != query.end()) {
      filter.priority = priorityIt->second;
    }

    auto orders = database_->getOrdersByFilter(filter);
    if (database_->hasError()) {
      return makeError(500, "internal_error", database_->getLastError(),
                       "Check server logs for details.");
    }

    std::ostringstream out;
    out << "{\"data\":[";
    for (size_t i = 0; i < orders.size(); ++i) {
      if (i > 0) {
        out << ",";
      }
      out << orderToJson(*orders[i]);
    }
    out << "]}";

    std::ostringstream details;
    details << "API READ /orders"
            << "; count=" << orders.size()
            << "; status=" << (filter.status.empty() ? "any" : filter.status)
            << "; sample_id="
            << (filter.sampleId.empty() ? "any" : filter.sampleId)
            << "; priority="
            << (filter.priority.empty() ? "any" : filter.priority);
    core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                           core::AuditEntry::EntityType::ORDER, "*",
                           actor, details.str());
    if (!database_->logAudit(entry)) {
      return makeError(500, "internal_error", "Audit log failed",
                       database_->getLastError());
    }

    return ApiResponse{200, out.str(), "application/json"};
  }

  if (path.rfind("/api/v1/orders/", 0) == 0) {
    const std::string orderId =
        path.substr(std::string("/api/v1/orders/").size());
    if (orderId.empty()) {
      return makeError(400, "validation_error", "Missing order_id",
                       "Provide order_id in URL path.");
    }
    auto order = database_->getOrderByOrderId(orderId);
    if (database_->hasError()) {
      return makeError(500, "internal_error", database_->getLastError(),
                       "Check server logs for details.");
    }
    if (!order) {
      return makeError(404, "not_found", "Order not found",
                       "Verify the order_id.");
    }

    core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                           core::AuditEntry::EntityType::ORDER, orderId,
                           actor, "API READ /orders/" + orderId);
    if (!database_->logAudit(entry)) {
      return makeError(500, "internal_error", "Audit log failed",
                       database_->getLastError());
    }

    return ApiResponse{200, "{\"data\":" + orderToJson(*order) + "}",
                       "application/json"};
  }

  if (path == "/api/v1/results") {
    std::vector<std::unique_ptr<core::TestResult>> results;
    auto orderIt = query.find("order_id");
    if (orderIt != query.end()) {
      int orderId = 0;
      if (!parseIntValue(orderIt->second, orderId) || orderId <= 0) {
        return makeError(400, "validation_error", "Invalid order_id",
                         "Provide numeric order_id.");
      }
      results = database_->getTestResultsByOrderId(orderId);
    } else {
      results = database_->getAllTestResults();
    }

    if (database_->hasError()) {
      return makeError(500, "internal_error", database_->getLastError(),
                       "Check server logs for details.");
    }

    std::ostringstream out;
    out << "{\"data\":[";
    for (size_t i = 0; i < results.size(); ++i) {
      if (i > 0) {
        out << ",";
      }
      out << resultToJson(*results[i]);
    }
    out << "]}";

    std::ostringstream details;
    details << "API READ /results"
            << "; count=" << results.size()
            << "; order_id=" << (orderIt != query.end() ? orderIt->second : "any");
    core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                           core::AuditEntry::EntityType::RESULT, "*",
                           actor, details.str());
    if (!database_->logAudit(entry)) {
      return makeError(500, "internal_error", "Audit log failed",
                       database_->getLastError());
    }

    return ApiResponse{200, out.str(), "application/json"};
  }

  if (path.rfind("/api/v1/results/", 0) == 0) {
    const std::string resultId =
        path.substr(std::string("/api/v1/results/").size());
    if (resultId.empty()) {
      return makeError(400, "validation_error", "Missing result_id",
                       "Provide result_id in URL path.");
    }
    auto result = database_->getTestResultByResultId(resultId);
    if (database_->hasError()) {
      return makeError(500, "internal_error", database_->getLastError(),
                       "Check server logs for details.");
    }
    if (!result) {
      return makeError(404, "not_found", "Result not found",
                       "Verify the result_id.");
    }

    core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                           core::AuditEntry::EntityType::RESULT, resultId,
                           actor, "API READ /results/" + resultId);
    if (!database_->logAudit(entry)) {
      return makeError(500, "internal_error", "Audit log failed",
                       database_->getLastError());
    }

    return ApiResponse{200, "{\"data\":" + resultToJson(*result) + "}",
                       "application/json"};
  }

  return makeError(404, "not_found", "Endpoint not found",
                   "Check the requested path.");
}

ApiServer::ApiServer(std::shared_ptr<db::Database> database, int port)
    : database_(std::move(database)),
      router_(database_),
      port_(port),
      serverFd_(-1),
      running_(false) {}

bool ApiServer::bindAndListen() {
  serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd_ < 0) {
    return false;
  }

  int opt = 1;
  setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(static_cast<uint16_t>(port_));

  if (bind(serverFd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
    return false;
  }

  if (listen(serverFd_, 10) < 0) {
    return false;
  }

  return true;
}

bool ApiServer::run() {
  if (running_) {
    return false;
  }
  if (!bindAndListen()) {
    if (serverFd_ >= 0) {
      close(serverFd_);
      serverFd_ = -1;
    }
    return false;
  }

  running_ = true;
  serveLoop();
  return true;
}

void ApiServer::stop() {
  running_ = false;
  if (serverFd_ >= 0) {
    close(serverFd_);
    serverFd_ = -1;
  }
}

void ApiServer::serveLoop() {
  while (running_) {
    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    const int clientFd =
        accept(serverFd_, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
    if (clientFd < 0) {
      if (running_) {
        continue;
      }
      break;
    }
    handleClient(clientFd);
    close(clientFd);
  }
}

void ApiServer::handleClient(int clientFd) {
  char buffer[8192];
  const ssize_t readBytes = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
  if (readBytes <= 0) {
    return;
  }
  buffer[readBytes] = '\0';

  std::istringstream requestStream{std::string(buffer)};
  std::string requestLine;
  if (!std::getline(requestStream, requestLine)) {
    return;
  }
  requestLine = trim(requestLine);

  std::istringstream lineStream(requestLine);
  ApiRequest request;
  lineStream >> request.method;
  lineStream >> request.path;

  std::string headerLine;
  while (std::getline(requestStream, headerLine)) {
    if (headerLine == "\r" || headerLine.empty()) {
      break;
    }
    const size_t sep = headerLine.find(':');
    if (sep == std::string::npos) {
      continue;
    }
    const std::string key = toLower(trim(headerLine.substr(0, sep)));
    const std::string value = trim(headerLine.substr(sep + 1));
    request.headers[key] = value;
  }

  std::ostringstream bodyStream;
  bodyStream << requestStream.rdbuf();
  std::string body = trimLeadingNewlines(bodyStream.str());
  auto lengthIt = request.headers.find("content-length");
  if (lengthIt != request.headers.end()) {
    try {
      size_t length = static_cast<size_t>(std::stoul(lengthIt->second));
      while (body.size() < length) {
        const ssize_t more = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
        if (more <= 0) {
          break;
        }
        buffer[more] = '\0';
        body.append(buffer, static_cast<size_t>(more));
      }
      if (body.size() > length) {
        body.resize(length);
      }
    } catch (...) {
      // Ignore invalid content-length and use whatever was read.
    }
  }
  request.body = body;

  ApiResponse response = router_.handleRequest(request);

  std::ostringstream out;
  out << "HTTP/1.1 " << response.status << " " << statusMessage(response.status)
      << "\r\n";
  out << "Content-Type: " << response.contentType << "\r\n";
  out << "Content-Length: " << response.body.size() << "\r\n";
  out << "Connection: close\r\n\r\n";
  out << response.body;

  const std::string responseStr = out.str();
  send(clientFd, responseStr.c_str(), responseStr.size(), 0);
}

} // namespace api
} // namespace opensylab
