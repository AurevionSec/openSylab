#include "api/ApiServer.h"
#include "version.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <unordered_set>
#include <netinet/in.h>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include "utils/Hl7.h"
#include "utils/Fhir.h"

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
        if (decoded != 0) {
          result.push_back(static_cast<char>(decoded));
        }
        // Skip %00 (null byte) — do not emit it or fall through to literal emission
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
  case 200: return "OK";
  case 201: return "Created";
  case 204: return "No Content";
  case 400: return "Bad Request";
  case 401: return "Unauthorized";
  case 403: return "Forbidden";
  case 404: return "Not Found";
  case 405: return "Method Not Allowed";
  case 409: return "Conflict";
  case 422: return "Unprocessable Entity";
  case 429: return "Too Many Requests";
  case 500: return "Internal Server Error";
  default:  return "Unknown";
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
  std::cerr << "[DB] " << message << "\n";
  const std::string lower = toLower(message);
  if (lower.find("nicht gefunden") != std::string::npos) {
    return makeError(404, "not_found", "Record not found",
                     "The requested record does not exist.");
  }
  if (lower.find("unique constraint failed") != std::string::npos ||
      lower.find("duplicate") != std::string::npos) {
    return makeError(409, "conflict", "Duplicate record",
                     "A record with this identifier already exists.");
  }
  if (lower.find("darf nicht leer") != std::string::npos ||
      lower.find("ung\u00fcltig") != std::string::npos) {
    return makeError(400, "validation_error", "Invalid input",
                     "One or more fields contain invalid values.");
  }
  if (lower.find("letzten aktiven administrator") != std::string::npos) {
    return makeError(400, "validation_error",
                     "Operation would remove the last active administrator",
                     "At least one active administrator must remain.");
  }
  return makeError(500, "internal_error", "An internal error occurred",
                   "Please contact the system administrator.");
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

// ===== VALIDATION HELPERS =====

// Validate string length (min and max inclusive)
bool validateStringLength(const std::string &value, size_t minLen, size_t maxLen, std::string &error) {
  const std::string trimmed = trim(value);

  if (trimmed.empty() && minLen > 0) {
    error = "Value cannot be empty or whitespace-only";
    return false;
  }

  if (trimmed.size() < minLen) {
    error = "Value must be at least " + std::to_string(minLen) + " characters";
    return false;
  }

  if (trimmed.size() > maxLen) {
    error = "Value must not exceed " + std::to_string(maxLen) + " characters";
    return false;
  }

  return true;
}

// Validate password strength (min 8 chars, must contain letters and numbers)
bool validatePassword(const std::string &password, std::string &error) {
  if (password.size() < 8) {
    error = "Password must be at least 8 characters";
    return false;
  }

  if (password.size() > 128) {
    error = "Password must not exceed 128 characters";
    return false;
  }

  bool hasLetter = false;
  bool hasDigit = false;

  for (char ch : password) {
    if (std::isalpha(static_cast<unsigned char>(ch))) {
      hasLetter = true;
    }
    if (std::isdigit(static_cast<unsigned char>(ch))) {
      hasDigit = true;
    }
    if (hasLetter && hasDigit) {
      break;
    }
  }

  if (!hasLetter || !hasDigit) {
    error = "Password must contain both letters and numbers";
    return false;
  }

  return true;
}

// Validate email format (basic check)
bool validateEmail(const std::string &email, std::string &error) {
  const std::string trimmed = trim(email);

  if (trimmed.size() < 5 || trimmed.size() > 255) {
    error = "Email must be between 5 and 255 characters";
    return false;
  }

  const size_t atPos = trimmed.find('@');
  if (atPos == std::string::npos || atPos == 0 || atPos == trimmed.size() - 1) {
    error = "Email must contain '@' with text before and after";
    return false;
  }

  const size_t dotPos = trimmed.find('.', atPos);
  if (dotPos == std::string::npos || dotPos == trimmed.size() - 1) {
    error = "Email must contain '.' after '@'";
    return false;
  }

  return true;
}

// Validate username format (alphanumeric + underscore only, 3-64 chars)
bool validateUsername(const std::string &username, std::string &error) {
  const std::string trimmed = trim(username);

  if (trimmed.size() < 3 || trimmed.size() > 64) {
    error = "Username must be between 3 and 64 characters";
    return false;
  }

  for (char ch : trimmed) {
    if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_') {
      error = "Username must contain only letters, numbers, and underscores";
      return false;
    }
  }

  return true;
}

// Validate and cap limit parameter (DoS protection)
constexpr int MAX_PAGINATION_LIMIT = 1000;

bool validateAndCapLimit(int &limit, std::string &error) {
  if (limit <= 0) {
    error = "Limit must be positive";
    return false;
  }

  if (limit > MAX_PAGINATION_LIMIT) {
    limit = MAX_PAGINATION_LIMIT;  // Auto-cap to max
  }

  return true;
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
        if (code <= 0x7F) {
          value << static_cast<char>(code);
        } else if (code <= 0x7FF) {
          value << static_cast<char>(0xC0 | (code >> 6));
          value << static_cast<char>(0x80 | (code & 0x3F));
        } else {
          value << static_cast<char>(0xE0 | (code >> 12));
          value << static_cast<char>(0x80 | ((code >> 6) & 0x3F));
          value << static_cast<char>(0x80 | (code & 0x3F));
        }
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
    : database_(std::move(database)) {
  // Initialize JWT authentication from environment variable
  auth::JwtAuth::JwtConfig jwtConfig;

  // Load JWT secret from environment variable (REQUIRED in production)
  const char* envSecret = std::getenv("OPENSYLAB_JWT_SECRET");
  if (envSecret != nullptr && std::strlen(envSecret) >= 32) {
    jwtConfig.secret = envSecret;
  } else {
    // Fallback to development secret ONLY if environment variable not set
    // WARNING: This is NOT secure for production!
    const char* devSecret = "opensylab-dev-secret-min-256-bits-change-in-production-12345";
    jwtConfig.secret = devSecret;

    // Log warning about using development secret
    std::cerr << "[WARNING] Using development JWT secret! Set OPENSYLAB_JWT_SECRET environment variable for production.\n";
  }

  jwtConfig.expirationMinutes = 60;
  jwtConfig.issuer = "opensylab";

  jwtAuth_ = std::make_unique<auth::JwtAuth>(jwtConfig);
}

std::string ApiRouter::sampleToJson(const core::Sample &sample) {
  std::ostringstream out;
  out << "{"
      << "\"id\":" << sample.getId() << ","
      << "\"sample_id\":" << jsonString(sample.getSampleId()) << ","
      << "\"patient_id\":" << jsonString(sample.getPatientId()) << ","
      << "\"patient_name\":" << jsonString(sample.getPatientName()) << ","
      << "\"description\":" << jsonString(sample.getDescription()) << ","
      << "\"status\":" << jsonString(sample.getStatusString()) << ","
      << "\"registration_date\":" << static_cast<long long>(sample.getRegistrationDate()) << ","
      << "\"updated_at\":" << static_cast<long long>(sample.getUpdatedAt())
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

std::string ApiRouter::resultToJson(const core::TestResult &result, const std::string &orderIdStr) {
  std::ostringstream out;
  out << "{"
      << "\"id\":" << result.getId() << ","
      << "\"result_id\":" << jsonString(result.getResultId()) << ","
      << "\"order_id\":" << jsonString(orderIdStr.empty() ? std::to_string(result.getOrderId()) : orderIdStr) << ","
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

std::string userToJson(const core::User &user) {
  std::ostringstream out;
  out << "{"
      << "\"id\":" << user.getId() << ","
      << "\"username\":" << jsonString(user.getUsername()) << ","
      << "\"role\":" << jsonString(user.getRoleString()) << ","
      << "\"active\":" << (user.isActive() ? "true" : "false") << ","
      << "\"must_change_password\":" << (user.mustChangePassword() ? "true" : "false") << ","
      << "\"created_at\":" << static_cast<long long>(user.getCreatedDate()) << ","
      << "\"last_login\":" << static_cast<long long>(user.getLastLogin()) << ","
      << "\"full_name\":" << jsonString(user.getFullName()) << ","
      << "\"email\":" << jsonString(user.getEmail());
  out << "}";
  return out.str();
}

std::string auditEntryToJson(const core::AuditEntry &entry) {
  std::ostringstream out;
  out << "{"
      << "\"id\":" << entry.getId() << ","
      << "\"action\":" << jsonString(entry.getActionString()) << ","
      << "\"entity\":" << jsonString(entry.getEntityString()) << ","
      << "\"entity_id\":" << jsonString(entry.getEntityId()) << ","
      << "\"user\":" << jsonString(entry.getUser()) << ","
      << "\"timestamp\":" << static_cast<long long>(entry.getTimestamp()) << ","
      << "\"details\":" << jsonString(entry.getDetails())
      << "}";
  return out.str();
}

std::string statsToJson(const db::Database::EntityStats &stats, const std::string &entityType) {
  std::ostringstream out;
  out << "{"
      << "\"entity_type\":" << jsonString(entityType) << ","
      << "\"total\":" << stats.total << ","
      << "\"by_status\":[";
  for (size_t i = 0; i < stats.byStatus.size(); ++i) {
    if (i > 0) out << ",";
    out << "{"
        << "\"status\":" << jsonString(stats.byStatus[i].status) << ","
        << "\"count\":" << stats.byStatus[i].count
        << "}";
  }
  out << "]}";
  return out.str();
}

ApiResponse ApiRouter::handleLogin(const ApiRequest &request) {
  // Parse JSON body
  const std::string body = trimLeadingNewlines(request.body);
  if (body.empty()) {
    return makeError(400, "validation_error", "Missing request body",
                     "Provide JSON with username and password.");
  }

  std::unordered_map<std::string, std::string> payload;
  std::string parseError;
  if (!parseJsonObject(body, payload, parseError)) {
    return makeError(400, "validation_error", "Invalid JSON payload",
                     parseError);
  }

  // Extract credentials
  auto usernameIt = payload.find("username");
  auto passwordIt = payload.find("password");

  if (usernameIt == payload.end() || usernameIt->second.empty()) {
    return makeError(400, "validation_error", "Missing username",
                     "Provide username in request body.");
  }
  if (passwordIt == payload.end() || passwordIt->second.empty()) {
    return makeError(400, "validation_error", "Missing password",
                     "Provide password in request body.");
  }

  const std::string &username = usernameIt->second;
  const std::string &password = passwordIt->second;

  // Extract optional MFA code
  std::optional<std::string> mfaCode;
  auto mfaIt = payload.find("mfa_code");
  if (mfaIt != payload.end() && !mfaIt->second.empty()) {
    mfaCode = mfaIt->second;
  }

  // Authenticate user via database
  auto user = database_->authenticateUser(username, password, mfaCode);
  if (!user) {
    const std::string &dbError = database_->getLastError();
    // Return 403 with actionable message when MFA is required
    if (dbError.find("MFA") != std::string::npos) {
      return makeError(403, "mfa_required",
                       "MFA code required",
                       "Provide mfa_code in request body.");
    }
    // Authentication failed - return 401
    return makeError(401, "authentication_failed", "Invalid credentials",
                     "Username or password is incorrect.");
  }

  // Generate JWT token
  std::string token;
  try {
    token = jwtAuth_->generateToken(user->getId(), user->getUsername(),
                                    user->getRoleString());
  } catch (const std::exception &e) {
    return makeError(500, "internal_error", "Token generation failed",
                     e.what());
  }

  // Calculate token expiration
  const int expiresIn = jwtAuth_->getConfig().expirationMinutes * 60; // seconds

  // Build response JSON
  std::ostringstream response;
  response << "{"
           << "\"token\":" << jsonString(token) << ","
           << "\"user\":{"
           << "\"id\":" << user->getId() << ","
           << "\"username\":" << jsonString(user->getUsername()) << ","
           << "\"role\":" << jsonString(user->getRoleString()) << ","
           << "\"must_change_password\":" << (user->mustChangePassword() ? "true" : "false")
           << "},"
           << "\"expiresIn\":" << expiresIn
           << "}";

  return ApiResponse{200, response.str(), "application/json"};
}

std::optional<auth::JwtAuth::TokenPayload>
ApiRouter::extractAndValidateJwt(
    const std::unordered_map<std::string, std::string> &headers) {
  // Look for Authorization header
  auto authIt = headers.find("authorization");
  if (authIt == headers.end()) {
    return std::nullopt;
  }

  // Extract Bearer token
  const std::string authValue = trim(authIt->second);
  const std::string bearerPrefix = "bearer ";

  if (authValue.size() <= bearerPrefix.size() ||
      toLower(authValue.substr(0, bearerPrefix.size())) != bearerPrefix) {
    // Not a Bearer token
    return std::nullopt;
  }

  const std::string token = trim(authValue.substr(bearerPrefix.size()));
  if (token.empty()) {
    return std::nullopt;
  }

  // Validate JWT token
  return jwtAuth_->validateToken(token);
}

ApiResponse ApiRouter::handleRequest(const ApiRequest &request) {
  if (!database_) {
    return makeError(500, "internal_error", "Database unavailable",
                     "Database instance is not configured.");
  }

  const std::string method = toLower(request.method);

  // Handle CORS preflight requests (OPTIONS) without authentication
  if (method == "options") {
    return ApiResponse{200, "", "text/plain"};
  }

  // Extract path early for routing
  std::string path = request.path;
  const size_t qpos = path.find('?');
  if (qpos != std::string::npos) {
    path = path.substr(0, qpos);
  }

  // Route: GET /api/v1/health (unauthenticated)
  if (method == "get" && path == "/api/v1/health") {
    std::ostringstream h;
    h << "{"
      << "\"status\":\"ok\","
      << "\"version\":" << jsonString(OPENSYLAB_VERSION) << ","
      << "\"service\":\"opensylab-lims\""
      << "}";
    return ApiResponse{200, h.str(), "application/json"};
  }

  // Route: POST /api/v1/auth/login (no authentication required)
  if (method == "post" && path == "/api/v1/auth/login") {
    return handleLogin(request);
  }

  // Authentication: Try JWT first, fall back to API-Key
  std::optional<auth::JwtAuth::TokenPayload> jwtPayload =
      extractAndValidateJwt(request.headers);
  std::optional<std::string> apiKeyRole;

  bool authenticated = false;
  std::string actor;

  if (jwtPayload.has_value()) {
    // JWT authentication successful
    authenticated = true;
    actor = "user:" + jwtPayload->username + " (id:" +
            std::to_string(jwtPayload->userId) + ")";
  } else {
    // Try API-Key authentication
    const std::string apiKey = extractApiKey(request.headers);
    apiKeyRole = database_->isApiKeyValid(apiKey);
    if (apiKeyRole.has_value()) {
      authenticated = true;
      actor = sanitizeActor(apiKey);
    }
  }

  if (!authenticated) {
    return makeError(401, "unauthorized", "Invalid API credentials",
                     "Provide X-API-Key or Authorization: Bearer <token>.");
  }

  // Determine effective role: JWT role or API-key role (default OPERATOR)
  const std::string effectiveRole = jwtPayload.has_value()
      ? jwtPayload->role
      : (apiKeyRole.has_value() ? apiKeyRole.value() : std::string("OPERATOR"));

  const bool isGet = method == "get";
  const bool isPost = method == "post";
  const bool isPut = method == "put";
  const bool isDelete = method == "delete";
  if (!isGet && !isPost && !isPut && !isDelete) {
    return makeError(405, "validation_error", "Method not allowed",
                     "Use GET for reads, POST for creates, PUT for updates, DELETE for deletes.");
  }

  // Extract query string (path was already extracted earlier)
  std::string queryString;
  const size_t qposQuery = request.path.find('?');
  if (qposQuery != std::string::npos) {
    queryString = request.path.substr(qposQuery + 1);
  }

  const std::unordered_map<std::string, std::string> query =
      parseQuery(queryString);

  // RBAC: VIEWER role cannot write lab data (applies to POST, PUT, DELETE)
  // Exception: allow VIEWER/CUSTOM to change their own password
  if (!isGet && (effectiveRole == "VIEWER" || effectiveRole == "CUSTOM") &&
      !(isPut && path == "/api/v1/users/me/password")) {
    return makeError(403, "forbidden", "Insufficient permissions",
                     "VIEWER and CUSTOM roles cannot create, update, or delete records.");
  }

  if ((isPost || isPut) && path.rfind("/api/v1/users", 0) != 0) {
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

      // Validate sample_id and patient_id length
      std::string validationError;
      if (!validateStringLength(payload["sample_id"], 1, 64, validationError)) {
        return makeError(400, "validation_error", "Invalid sample_id", validationError);
      }
      if (!validateStringLength(payload["patient_id"], 1, 64, validationError)) {
        return makeError(400, "validation_error", "Invalid patient_id", validationError);
      }

      core::Sample sample(payload["sample_id"], payload["patient_id"]);
      auto nameIt = payload.find("patient_name");
      if (nameIt != payload.end()) {
        // Validate patient_name length if provided
        if (!nameIt->second.empty() && !validateStringLength(nameIt->second, 1, 255, validationError)) {
          return makeError(400, "validation_error", "Invalid patient_name", validationError);
        }
        sample.setPatientName(nameIt->second);
      }
      auto descIt = payload.find("description");
      if (descIt != payload.end()) {
        // Validate description length if provided
        if (!descIt->second.empty() && !validateStringLength(descIt->second, 1, 5000, validationError)) {
          return makeError(400, "validation_error", "Invalid description", validationError);
        }
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
      database_->clearError(); // Read-back failure is non-fatal; fall back to in-memory object
      const core::Sample &responseSample = created ? *created : sample;
      return ApiResponse{201,
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

      // Validate required field lengths
      std::string validationError;
      if (!validateStringLength(payload["order_id"], 1, 64, validationError)) {
        return makeError(400, "validation_error", "Invalid order_id", validationError);
      }
      if (!validateStringLength(payload["sample_id"], 1, 64, validationError)) {
        return makeError(400, "validation_error", "Invalid sample_id", validationError);
      }
      // Validate sample_id references an existing sample
      auto sampleRef = database_->getSampleByBarcode(payload["sample_id"]);
      if (!sampleRef) {
        return makeError(422, "unprocessable_entity", "Sample not found",
                         "The provided sample_id does not exist.");
      }
      if (!validateStringLength(payload["test_type"], 1, 255, validationError)) {
        return makeError(400, "validation_error", "Invalid test_type", validationError);
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
        // Validate requested_by length if provided
        if (!requesterIt->second.empty() && !validateStringLength(requesterIt->second, 1, 255, validationError)) {
          return makeError(400, "validation_error", "Invalid requested_by", validationError);
        }
        order.setRequestedBy(requesterIt->second);
      }
      auto notesIt = payload.find("notes");
      if (notesIt != payload.end()) {
        // Validate notes length if provided
        if (!notesIt->second.empty() && !validateStringLength(notesIt->second, 1, 5000, validationError)) {
          return makeError(400, "validation_error", "Invalid notes", validationError);
        }
        order.setNotes(notesIt->second);
      }

      if (!database_->createOrder(order, actor)) {
        return makeDbErrorResponse(database_->getLastError());
      }

      auto created = database_->getOrderByOrderId(order.getOrderId());
      database_->clearError(); // Read-back failure is non-fatal; fall back to in-memory object
      const core::Order &responseOrder = created ? *created : order;
      return ApiResponse{201,
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
      // Validate order exists and has active status (orderId is the numeric PK)
      auto orderRef = database_->getOrder(orderId);
      if (!orderRef) {
        return makeError(422, "unprocessable_entity", "Order not found",
                         "Provide the numeric order id (orders.id, not the order_id string).");
      }
      {
        auto orderStatus = orderRef->getStatus();
        if (orderStatus != core::Order::Status::IN_PROGRESS &&
            orderStatus != core::Order::Status::COMPLETED) {
          return makeError(409, "conflict", "Order not active",
                           "Results can only be added to IN_PROGRESS or COMPLETED orders.");
        }
      }

      // Validate required field lengths
      std::string validationError;
      if (!validateStringLength(payload["result_id"], 1, 64, validationError)) {
        return makeError(400, "validation_error", "Invalid result_id", validationError);
      }
      if (!validateStringLength(payload["test_parameter"], 1, 255, validationError)) {
        return makeError(400, "validation_error", "Invalid test_parameter", validationError);
      }
      if (!validateStringLength(payload["value"], 1, 255, validationError)) {
        return makeError(400, "validation_error", "Invalid value", validationError);
      }
      if (!validateStringLength(payload["unit"], 1, 255, validationError)) {
        return makeError(400, "validation_error", "Invalid unit", validationError);
      }

      core::TestResult result(payload["result_id"], orderId,
                              payload["test_parameter"]);
      result.setValue(payload["value"]);
      result.setUnit(payload["unit"]);
      auto refRangeIt = payload.find("reference_range");
      if (refRangeIt != payload.end()) {
        // Validate reference_range length if provided
        if (!refRangeIt->second.empty() && !validateStringLength(refRangeIt->second, 1, 255, validationError)) {
          return makeError(400, "validation_error", "Invalid reference_range", validationError);
        }
        result.setReferenceRange(refRangeIt->second);
      }
      auto refLowIt = payload.find("reference_low");
      auto refHighIt = payload.find("reference_high");
      double refLow = 0.0;
      double refHigh = 0.0;
      bool hasRefLow = false;
      bool hasRefHigh = false;

      if (refLowIt != payload.end() && !refLowIt->second.empty()) {
        if (!parseDoubleValue(refLowIt->second, refLow)) {
          return makeError(400, "validation_error", "Invalid reference_low",
                           "Provide numeric reference_low.");
        }
        hasRefLow = true;
        result.setReferenceLow(refLow);
      }
      if (refHighIt != payload.end() && !refHighIt->second.empty()) {
        if (!parseDoubleValue(refHighIt->second, refHigh)) {
          return makeError(400, "validation_error", "Invalid reference_high",
                           "Provide numeric reference_high.");
        }
        hasRefHigh = true;
        result.setReferenceHigh(refHigh);
      }
      // Validate that reference_high > reference_low when both are provided
      if (hasRefLow && hasRefHigh && refHigh <= refLow) {
        return makeError(400, "validation_error", "Invalid reference range",
                         "reference_high must be greater than reference_low.");
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
        // Validate measured_by length if provided
        if (!measuredByIt->second.empty() && !validateStringLength(measuredByIt->second, 1, 255, validationError)) {
          return makeError(400, "validation_error", "Invalid measured_by", validationError);
        }
        result.setMeasuredBy(measuredByIt->second);
      }
      auto commentIt = payload.find("comment");
      if (commentIt != payload.end()) {
        // Validate comment length if provided
        if (!commentIt->second.empty() && !validateStringLength(commentIt->second, 1, 5000, validationError)) {
          return makeError(400, "validation_error", "Invalid comment", validationError);
        }
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
      database_->clearError(); // Read-back failure is non-fatal; fall back to in-memory object
      const core::TestResult &responseResult = created ? *created : result;
      {
        std::string oidStr;
        auto parentOrder = database_->getOrder(responseResult.getOrderId());
        if (parentOrder) { oidStr = parentOrder->getOrderId(); }
        return ApiResponse{201,
                           "{\"data\":" + resultToJson(responseResult, oidStr) + "}",
                           "application/json"};
      }
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

      // Immutability guard: VALIDATED and ARCHIVED samples cannot be edited
      {
        const auto s = existing->getStatus();
        if (s == core::Sample::Status::VALIDATED || s == core::Sample::Status::ARCHIVED) {
          return makeError(409, "conflict", "Sample is immutable",
                           "VALIDATED and ARCHIVED samples cannot be edited (ISO 15189).");
        }
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

      // Validate patient_id length
      std::string validationError;
      if (!validateStringLength(patientIt->second, 1, 64, validationError)) {
        return makeError(400, "validation_error", "Invalid patient_id", validationError);
      }

      core::Sample updated = *existing;
      updated.setSampleId(sampleId);
      updated.setPatientId(patientIt->second);
      auto nameIt = payload.find("patient_name");
      if (nameIt != payload.end()) {
        // Validate patient_name length if provided
        if (!nameIt->second.empty() && !validateStringLength(nameIt->second, 1, 255, validationError)) {
          return makeError(400, "validation_error", "Invalid patient_name", validationError);
        }
        updated.setPatientName(nameIt->second);
      }
      auto descIt = payload.find("description");
      if (descIt != payload.end()) {
        // Validate description length if provided
        if (!descIt->second.empty() && !validateStringLength(descIt->second, 1, 5000, validationError)) {
          return makeError(400, "validation_error", "Invalid description", validationError);
        }
        updated.setDescription(descIt->second);
      }
      auto statusIt = payload.find("status");
      if (statusIt != payload.end() && !statusIt->second.empty()) {
        try {
          const auto newStatus = core::Sample::stringToStatus(statusIt->second);
          static const std::unordered_map<std::string,std::vector<std::string>>
              kSampleTrans = {
                {"REGISTERED",  {"IN_ANALYSIS", "ARCHIVED"}},
                {"IN_ANALYSIS", {"ANALYZED",    "ARCHIVED"}},
                {"ANALYZED",    {"VALIDATED",   "ARCHIVED"}},
                {"VALIDATED",   {"ARCHIVED"}},
                {"ARCHIVED",    {}},
              };
          const std::string cs = existing->getStatusString();
          const std::string ns = core::Sample::statusToString(newStatus);
          const auto ti = kSampleTrans.find(cs);
          if (ti == kSampleTrans.end()) {
            return makeError(409, "conflict", "Unknown current status",
                             "Status '" + cs + "' is not recognized; transition rejected.");
          }
          const auto &allowed = ti->second;
          if (std::find(allowed.begin(), allowed.end(), ns) == allowed.end() && cs != ns) {
            return makeError(409, "conflict", "Invalid status transition",
                             "Transition from " + cs + " to " + ns + " is not allowed.");
          }
          updated.setStatus(newStatus);
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
      auto refreshedSample = database_->getSampleByBarcode(updated.getSampleId());
      const core::Sample &rspSample = refreshedSample ? *refreshedSample : updated;
      return ApiResponse{200,
                         "{\"data\":" + sampleToJson(rspSample) + "}",
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

      // Immutability guard: VALIDATED and CANCELLED orders cannot be edited
      {
        const auto s = existing->getStatus();
        if (s == core::Order::Status::VALIDATED || s == core::Order::Status::CANCELLED) {
          return makeError(409, "conflict", "Order is immutable",
                           "VALIDATED and CANCELLED orders cannot be edited.");
        }
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

      // Validate required field lengths
      std::string validationError;
      if (!validateStringLength(sampleIt->second, 1, 64, validationError)) {
        return makeError(400, "validation_error", "Invalid sample_id", validationError);
      }
      if (!validateStringLength(testIt->second, 1, 255, validationError)) {
        return makeError(400, "validation_error", "Invalid test_type", validationError);
      }

      core::Order updated = *existing;
      updated.setOrderId(orderId);
      updated.setSampleId(sampleIt->second);
      updated.setTestType(testIt->second);
      auto statusIt = payload.find("status");
      if (statusIt != payload.end() && !statusIt->second.empty()) {
        // Order Status-Transition-Validierung (ISO 15189)
        {
          static const std::unordered_map<std::string,std::vector<std::string>> kTrans = {
              {"REQUESTED",   {"IN_PROGRESS", "CANCELLED"}},
              {"IN_PROGRESS", {"COMPLETED",   "CANCELLED"}},
              {"COMPLETED",   {"VALIDATED"}},
              {"VALIDATED",   {}},
              {"CANCELLED",   {}},
          };
          const std::string &ns = statusIt->second;
          const std::string &cs = existing->getStatusString();
          auto ti = kTrans.find(cs);
          if (ti == kTrans.end()) {
            return makeError(409, "conflict", "Unknown current status",
                             "Status '" + cs + "' is not recognized; transition rejected.");
          }
          const auto &allowed = ti->second;
          if (std::find(allowed.begin(), allowed.end(), ns) == allowed.end() && ns != cs) {
            return makeError(409, "conflict", "Invalid status transition",
                "Transition " + cs + " -> " + ns + " is not allowed.");
          }
        }
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
        // Validate requested_by length if provided
        if (!requesterIt->second.empty() && !validateStringLength(requesterIt->second, 1, 255, validationError)) {
          return makeError(400, "validation_error", "Invalid requested_by", validationError);
        }
        updated.setRequestedBy(requesterIt->second);
      }
      auto notesIt = payload.find("notes");
      if (notesIt != payload.end()) {
        // Validate notes length if provided
        if (!notesIt->second.empty() && !validateStringLength(notesIt->second, 1, 5000, validationError)) {
          return makeError(400, "validation_error", "Invalid notes", validationError);
        }
        updated.setNotes(notesIt->second);
      }

      if (!database_->updateOrder(updated, actor)) {
        return makeDbErrorResponse(database_->getLastError());
      }
      auto refreshedOrder = database_->getOrderByOrderId(updated.getOrderId());
      const core::Order &rspOrder = refreshedOrder ? *refreshedOrder : updated;
      return ApiResponse{200,
                         "{\"data\":" + orderToJson(rspOrder) + "}",
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

      // Immutability guard: VALIDATED and REJECTED results cannot be edited
      {
        const auto s = existing->getStatus();
        if (s == core::TestResult::Status::VALIDATED ||
            s == core::TestResult::Status::REJECTED) {
          return makeError(409, "conflict", "Result is immutable",
                           "VALIDATED and REJECTED results cannot be edited (ISO 15189).");
        }
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

      // Validate required field lengths
      std::string validationError;
      if (!validateStringLength(paramIt->second, 1, 255, validationError)) {
        return makeError(400, "validation_error", "Invalid test_parameter", validationError);
      }
      if (!validateStringLength(valueIt->second, 1, 255, validationError)) {
        return makeError(400, "validation_error", "Invalid value", validationError);
      }
      if (!validateStringLength(unitIt->second, 1, 255, validationError)) {
        return makeError(400, "validation_error", "Invalid unit", validationError);
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
        // Validate reference_range length if provided
        if (!refRangeIt->second.empty() && !validateStringLength(refRangeIt->second, 1, 255, validationError)) {
          return makeError(400, "validation_error", "Invalid reference_range", validationError);
        }
        updated.setReferenceRange(refRangeIt->second);
      }
      auto refLowIt = payload.find("reference_low");
      auto refHighIt = payload.find("reference_high");
      bool updatedRefLow = false;
      bool updatedRefHigh = false;

      if (refLowIt != payload.end() && !refLowIt->second.empty()) {
        double refLow = 0.0;
        if (!parseDoubleValue(refLowIt->second, refLow)) {
          return makeError(400, "validation_error", "Invalid reference_low",
                           "Provide numeric reference_low.");
        }
        updated.setReferenceLow(refLow);
        updatedRefLow = true;
      }
      if (refHighIt != payload.end() && !refHighIt->second.empty()) {
        double refHigh = 0.0;
        if (!parseDoubleValue(refHighIt->second, refHigh)) {
          return makeError(400, "validation_error", "Invalid reference_high",
                           "Provide numeric reference_high.");
        }
        updated.setReferenceHigh(refHigh);
        updatedRefHigh = true;
      }
      // Validate that reference_high > reference_low (check final state if either was updated)
      if (updatedRefLow || updatedRefHigh) {
        double finalRefLow = updated.getReferenceLow();
        double finalRefHigh = updated.getReferenceHigh();
        // Only validate if both have non-zero values
        if (finalRefLow != 0.0 && finalRefHigh != 0.0 && finalRefHigh <= finalRefLow) {
          return makeError(400, "validation_error", "Invalid reference range",
                           "reference_high must be greater than reference_low.");
        }
      }
      auto statusIt = payload.find("status");
      if (statusIt != payload.end() && !statusIt->second.empty()) {
        try {
          const auto newStatus = core::TestResult::stringToStatus(statusIt->second);
          // Enforce terminal-state rule: REJECTED results cannot be re-validated
          static const std::unordered_map<std::string, std::vector<std::string>>
              kResultTrans = {
                {"PENDING",   {"ENTERED", "REJECTED"}},
                {"ENTERED",   {"VALIDATED", "REJECTED", "REPEATED"}},
                {"VALIDATED", {}},
                {"REPEATED",  {"ENTERED", "VALIDATED", "REJECTED"}},
                {"REJECTED",  {}},  // terminal — no transitions allowed
              };
          const std::string cs = existing->getStatusString();
          const std::string ns = core::TestResult::statusToString(newStatus);
          const auto ti = kResultTrans.find(cs);
          if (ti == kResultTrans.end()) {
            return makeError(409, "conflict", "Unknown current status",
                             "Status '" + cs + "' is not recognized; transition rejected.");
          }
          const auto &allowed = ti->second;
          if (std::find(allowed.begin(), allowed.end(), ns) == allowed.end() && cs != ns) {
            return makeError(409, "conflict", "Invalid status transition",
                             "Transition from " + cs + " to " + ns + " is not allowed.");
          }
          updated.setStatus(newStatus);
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
        // Validate measured_by length if provided
        if (!measuredByIt->second.empty() && !validateStringLength(measuredByIt->second, 1, 255, validationError)) {
          return makeError(400, "validation_error", "Invalid measured_by", validationError);
        }
        updated.setMeasuredBy(measuredByIt->second);
      }
      auto commentIt = payload.find("comment");
      if (commentIt != payload.end()) {
        // Validate comment length if provided
        if (!commentIt->second.empty() && !validateStringLength(commentIt->second, 1, 5000, validationError)) {
          return makeError(400, "validation_error", "Invalid comment", validationError);
        }
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
      auto refreshedResult = database_->getTestResultByResultId(updated.getResultId());
      const core::TestResult &rspResult = refreshedResult ? *refreshedResult : updated;
      {
        std::string oidStr;
        auto parentOrder = database_->getOrder(rspResult.getOrderId());
        if (parentOrder) { oidStr = parentOrder->getOrderId(); }
        return ApiResponse{200,
                           "{\"data\":" + resultToJson(rspResult, oidStr) + "}",
                           "application/json"};
      }
    }
  }

  // DELETE endpoints
  if (isDelete) {
    if (path.rfind("/api/v1/samples/", 0) == 0) {
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

      {
        const auto s = existing->getStatus();
        if (s == core::Sample::Status::ARCHIVED) {
          return makeError(409, "conflict", "Sample already archived",
                           "Sample is in terminal state.");
        }
        if (s == core::Sample::Status::IN_ANALYSIS) {
          return makeError(409, "conflict", "Sample cannot be deleted",
                           "Sample is currently IN_ANALYSIS; complete or archive it first.");
        }
        if (s == core::Sample::Status::ANALYZED) {
          return makeError(409, "conflict", "Sample cannot be deleted",
                           "Sample has been ANALYZED; validate or archive it first.");
        }
        if (s == core::Sample::Status::VALIDATED) {
          return makeError(409, "conflict", "Sample cannot be deleted",
                           "VALIDATED samples are immutable (ISO 15189).");
        }
      }

      // Pre-check: active orders block sample deletion (returns clean 409 vs DB-level error)
      {
        auto orders = database_->getOrdersBySampleId(existing->getSampleId());
        for (const auto &ord : orders) {
          if (ord->getStatus() != core::Order::Status::CANCELLED) {
            return makeError(409, "conflict", "Sample has active orders",
                             "Order " + ord->getOrderId() +
                             " must be cancelled before deleting the sample.");
          }
        }
      }
      if (!database_->deleteSample(existing->getId(), actor)) {
        return makeDbErrorResponse(database_->getLastError());
      }

      return ApiResponse{204, "", "application/json"};
    }

    if (path.rfind("/api/v1/orders/", 0) == 0) {
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

      {
        const auto s = existing->getStatus();
        if (s == core::Order::Status::CANCELLED) {
          return makeError(409, "conflict", "Order already cancelled",
                           "Order is in terminal state.");
        }
        if (s == core::Order::Status::VALIDATED || s == core::Order::Status::COMPLETED) {
          return makeError(409, "conflict", "Order cannot be cancelled",
                           "Only REQUESTED or IN_PROGRESS orders can be cancelled.");
        }
      }

      // Pre-check: active results block cancellation (returns clean 409 vs DB-level error)
      {
        auto results = database_->getTestResultsByOrderId(existing->getId());
        for (const auto &res : results) {
          const auto rs = res->getStatus();
          if (rs != core::TestResult::Status::VALIDATED &&
              rs != core::TestResult::Status::REJECTED) {
            return makeError(409, "conflict", "Order has active results",
                             "Result " + res->getResultId() +
                             " must be completed or rejected before cancelling the order.");
          }
        }
      }
      if (!database_->deleteOrder(existing->getId(), actor)) {
        return makeDbErrorResponse(database_->getLastError());
      }

      return ApiResponse{204, "", "application/json"};
    }

    if (path.rfind("/api/v1/results/", 0) == 0) {
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

      {
        const auto s = existing->getStatus();
        if (s == core::TestResult::Status::REJECTED) {
          // REJECTED = already soft-deleted; return 204 to make DELETE idempotent
          return ApiResponse{204, "", "application/json"};
        }
        if (s == core::TestResult::Status::VALIDATED) {
          return makeError(409, "conflict", "Result cannot be deleted",
                           "VALIDATED results are immutable (ISO 15189).");
        }
      }

      if (!database_->deleteTestResult(existing->getId(), actor)) {
        return makeDbErrorResponse(database_->getLastError());
      }

      return ApiResponse{204, "", "application/json"};
    }

    // Don't return 404 here for /api/v1/users/ — that handler comes after this block
    if (path.rfind("/api/v1/users/", 0) != 0) {
      return makeError(404, "not_found", "Endpoint not found",
                       "Check the requested path.");
    }
  }

  // Allow user management endpoints (POST/PUT/DELETE /api/v1/users/...) to proceed
  // They are handled later in the routing logic
  if (!isGet && !(path.rfind("/api/v1/users", 0) == 0 && (isPost || isPut || isDelete))) {
    return makeError(405, "validation_error", "Method not allowed",
                     "Use POST/PUT/DELETE for write endpoints.");
  }

  if (path == "/api/v1/samples") {
    db::Database::SampleFilter filter;
    auto qIt = query.find("q");
    if (qIt != query.end()) {
      filter.query = qIt->second;
    }
    auto statusIt = query.find("status");
    if (statusIt != query.end()) {
      if (!core::Sample::isValidStatusString(statusIt->second)) {
        return makeError(400, "validation_error", "Invalid status",
                         "Use Erfasst, In Analyse, Analysiert, Validiert, Archiviert.");
      }
      filter.status = core::Sample::statusToString(
          core::Sample::stringToStatus(statusIt->second));
    }
    auto limitIt = query.find("limit");
    if (limitIt != query.end()) {
      int limitValue = 0;
      if (!parseIntValue(limitIt->second, limitValue)) {
        return makeError(400, "validation_error", "Invalid limit",
                         "Provide integer limit.");
      }
      // Validate and cap limit to prevent DoS
      std::string validationError;
      if (!validateAndCapLimit(limitValue, validationError)) {
        return makeError(400, "validation_error", "Invalid limit", validationError);
      }
      filter.limit = limitValue;
    }
    auto offsetIt = query.find("offset");
    if (offsetIt != query.end()) {
      int offsetValue = 0;
      if (!parseIntValue(offsetIt->second, offsetValue) || offsetValue < 0) {
        return makeError(400, "validation_error", "Invalid offset",
                         "Provide non-negative integer offset.");
      }
      if (!filter.limit.has_value()) {
        return makeError(400, "validation_error", "Offset requires limit",
                         "Provide limit when using offset.");
      }
      filter.offset = offsetValue;
    }
    auto fromIt = query.find("from");
    auto toIt = query.find("to");
    bool hasFrom = false;
    bool hasTo = false;

    if (fromIt != query.end()) {
      std::time_t ts{};
      if (!parseTimeValue(fromIt->second, ts)) {
        return makeError(400, "validation_error", "Invalid 'from' value",
                         "Provide Unix timestamp for from.");
      }
      filter.fromDate = ts;
      hasFrom = true;
    }
    if (toIt != query.end()) {
      std::time_t ts{};
      if (!parseTimeValue(toIt->second, ts)) {
        return makeError(400, "validation_error", "Invalid 'to' value",
                         "Provide Unix timestamp for to.");
      }
      filter.toDate = ts;
      hasTo = true;
    }
    // Validate that from <= to when both are provided
    if (hasFrom && hasTo && filter.fromDate > filter.toDate) {
      return makeError(400, "validation_error", "Invalid date range",
                       "'from' date must be less than or equal to 'to' date.");
    }

    auto samples = database_->getSamplesByFilter(filter);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }

    int total = database_->getSamplesCount(filter);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    if (total < 0) {
        total = static_cast<int>(samples.size());
    }

    std::ostringstream out;
    out << "{\"data\":[";
    for (size_t i = 0; i < samples.size(); ++i) {
      if (i > 0) {
        out << ",";
      }
      out << sampleToJson(*samples[i]);
    }
    out << "],\"total\":" << total << "}";

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
                               : "any")
            << "; limit="
            << (filter.limit.has_value() ? std::to_string(*filter.limit)
                                         : "any")
            << "; offset="
            << (filter.offset.has_value() ? std::to_string(*filter.offset)
                                          : "any");
    core::AuditEntry entry(core::AuditEntry::ActionType::ACCESS,
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
      return makeDbErrorResponse(database_->getLastError());
    }
    if (!sample) {
      return makeError(404, "not_found", "Sample not found",
                       "Verify the sample_id.");
    }

    core::AuditEntry entry(core::AuditEntry::ActionType::ACCESS,
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
      if (!core::Order::isValidStatusString(statusIt->second)) {
        return makeError(400, "validation_error", "Invalid status",
                         "Use REQUESTED, IN_PROGRESS, COMPLETED, VALIDATED, CANCELLED.");
      }
      filter.status = core::Order::statusToString(
          core::Order::stringToStatus(statusIt->second));
    }
    auto sampleIt = query.find("sample_id");
    if (sampleIt != query.end()) {
      filter.sampleId = sampleIt->second;
    }
    auto priorityIt = query.find("priority");
    if (priorityIt != query.end()) {
      if (!core::Order::isValidPriorityString(priorityIt->second)) {
        return makeError(400, "validation_error", "Invalid priority",
                         "Use NORMAL, URGENT, EMERGENCY.");
      }
      filter.priority = core::Order::priorityToString(
          core::Order::stringToPriority(priorityIt->second));
    }
    auto limitIt = query.find("limit");
    if (limitIt != query.end()) {
      int limitValue = 0;
      if (!parseIntValue(limitIt->second, limitValue)) {
        return makeError(400, "validation_error", "Invalid limit",
                         "Provide integer limit.");
      }
      // Validate and cap limit to prevent DoS
      std::string validationError;
      if (!validateAndCapLimit(limitValue, validationError)) {
        return makeError(400, "validation_error", "Invalid limit", validationError);
      }
      filter.limit = limitValue;
    }
    auto offsetIt = query.find("offset");
    if (offsetIt != query.end()) {
      int offsetValue = 0;
      if (!parseIntValue(offsetIt->second, offsetValue) || offsetValue < 0) {
        return makeError(400, "validation_error", "Invalid offset",
                         "Provide non-negative integer offset.");
      }
      if (!filter.limit.has_value()) {
        return makeError(400, "validation_error", "Offset requires limit",
                         "Provide limit when using offset.");
      }
      filter.offset = offsetValue;
    }

    auto orders = database_->getOrdersByFilter(filter);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }

    std::ostringstream out;
    out << "{\"data\":[";
    for (size_t i = 0; i < orders.size(); ++i) {
      if (i > 0) {
        out << ",";
      }
      out << orderToJson(*orders[i]);
    }
    int ordersTotal = database_->getOrdersCount(filter);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    if (ordersTotal < 0) ordersTotal = static_cast<int>(orders.size());
    out << "],\"total\":" << ordersTotal << "}";

    std::ostringstream details;
    details << "API READ /orders"
            << "; count=" << orders.size()
            << "; status=" << (filter.status.empty() ? "any" : filter.status)
            << "; sample_id="
            << (filter.sampleId.empty() ? "any" : filter.sampleId)
            << "; priority="
            << (filter.priority.empty() ? "any" : filter.priority)
            << "; limit="
            << (filter.limit.has_value() ? std::to_string(*filter.limit)
                                         : "any")
            << "; offset="
            << (filter.offset.has_value() ? std::to_string(*filter.offset)
                                          : "any");
    core::AuditEntry entry(core::AuditEntry::ActionType::ACCESS,
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
      return makeDbErrorResponse(database_->getLastError());
    }
    if (!order) {
      return makeError(404, "not_found", "Order not found",
                       "Verify the order_id.");
    }

    core::AuditEntry entry(core::AuditEntry::ActionType::ACCESS,
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
    std::optional<int> limit;
    std::optional<int> offset;
    auto limitIt = query.find("limit");
    if (limitIt != query.end()) {
      int limitValue = 0;
      if (!parseIntValue(limitIt->second, limitValue)) {
        return makeError(400, "validation_error", "Invalid limit",
                         "Provide integer limit.");
      }
      // Validate and cap limit to prevent DoS
      std::string validationError;
      if (!validateAndCapLimit(limitValue, validationError)) {
        return makeError(400, "validation_error", "Invalid limit", validationError);
      }
      limit = limitValue;
    }
    auto offsetIt = query.find("offset");
    if (offsetIt != query.end()) {
      int offsetValue = 0;
      if (!parseIntValue(offsetIt->second, offsetValue) || offsetValue < 0) {
        return makeError(400, "validation_error", "Invalid offset",
                         "Provide non-negative integer offset.");
      }
      if (!limit.has_value()) {
        return makeError(400, "validation_error", "Offset requires limit",
                         "Provide limit when using offset.");
      }
      offset = offsetValue;
    }
    // Parse optional status and flag filters
    std::string statusFilter;
    std::string flagFilter;
    auto statusResultIt = query.find("status");
    if (statusResultIt != query.end() && !statusResultIt->second.empty()) {
      statusFilter = statusResultIt->second;
    }

    auto flagResultIt = query.find("flag");
    if (flagResultIt != query.end() && !flagResultIt->second.empty()) {
      flagFilter = flagResultIt->second;
    }

    std::optional<int> resultsOrderIdFilter;
    auto orderIt = query.find("order_id");
    // When in-memory filters are active, fetch all records to get accurate total
    // then apply pagination after filtering
    bool hasMemFilter = !statusFilter.empty() || !flagFilter.empty();
    std::optional<int> dbLimit  = hasMemFilter ? std::nullopt : limit;
    std::optional<int> dbOffset = hasMemFilter ? std::nullopt : offset;
    if (orderIt != query.end()) {
      int orderId = 0;
      if (!parseIntValue(orderIt->second, orderId) || orderId <= 0) {
        return makeError(400, "validation_error", "Invalid order_id",
                         "Provide numeric order_id.");
      }
      results = database_->getTestResultsByOrderId(orderId, dbLimit, dbOffset);
      resultsOrderIdFilter = orderId;
    } else {
      results = database_->getAllTestResults(dbLimit, dbOffset);
    }

    // Apply status filter in-memory if provided
    if (!statusFilter.empty()) {
      std::vector<std::unique_ptr<core::TestResult>> filtered;
      for (auto &r : results) {
        if (r->getStatusString() == statusFilter) {
          filtered.push_back(std::move(r));
        }
      }
      results = std::move(filtered);
    }
    // Apply flag filter in-memory if provided
    if (!flagFilter.empty()) {
      std::vector<std::unique_ptr<core::TestResult>> filtered;
      for (auto &r : results) {
        if (r->getFlagString() == flagFilter) {
          filtered.push_back(std::move(r));
        }
      }
      results = std::move(filtered);
    }

    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }

    // Total is now accurate: either filtered count (hasMem) or DB count (no filter)
    int resultsTotal = hasMemFilter
        ? static_cast<int>(results.size())
        : database_->getTestResultsCount(resultsOrderIdFilter);
    if (!hasMemFilter && database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    // When in-memory filters ran (no DB limit/offset), apply pagination now
    if (hasMemFilter && (limit.has_value() || offset.has_value())) {
      int startIdx = offset.value_or(0);
      int endIdx   = limit.has_value()
                   ? std::min(startIdx + *limit, static_cast<int>(results.size()))
                   : static_cast<int>(results.size());
      if (startIdx < static_cast<int>(results.size())) {
        std::vector<std::unique_ptr<core::TestResult>> paged;
        for (int pi = startIdx; pi < endIdx; ++pi) {
          paged.push_back(std::move(results[static_cast<size_t>(pi)]));
        }
        results = std::move(paged);
      } else {
        results.clear();
      }
    }
    std::unordered_map<int, std::string> orderIdCache;
    for (const auto &r : results) {
      const int oid = r->getOrderId();
      if (orderIdCache.find(oid) == orderIdCache.end()) {
        auto ord = database_->getOrder(oid);
        if (ord) { orderIdCache[oid] = ord->getOrderId(); }
      }
    }
    std::ostringstream out;
    out << "{\"data\":[";
    for (size_t i = 0; i < results.size(); ++i) {
      if (i > 0) {
        out << ",";
      }
      const auto cit = orderIdCache.find(results[i]->getOrderId());
      const std::string &oidStr = (cit != orderIdCache.end()) ? cit->second : "";
      out << resultToJson(*results[i], oidStr);
    }
    if (resultsTotal < 0) resultsTotal = static_cast<int>(results.size());
    out << "],\"total\":" << resultsTotal << "}";

    std::ostringstream details;
    details << "API READ /results"
            << "; count=" << results.size()
            << "; order_id="
            << (orderIt != query.end() ? orderIt->second : "any")
            << "; limit=" << (limit.has_value() ? std::to_string(*limit) : "any")
            << "; offset="
            << (offset.has_value() ? std::to_string(*offset) : "any");
    core::AuditEntry entry(core::AuditEntry::ActionType::ACCESS,
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
      return makeDbErrorResponse(database_->getLastError());
    }
    if (!result) {
      return makeError(404, "not_found", "Result not found",
                       "Verify the result_id.");
    }

    core::AuditEntry entry(core::AuditEntry::ActionType::ACCESS,
                           core::AuditEntry::EntityType::RESULT, resultId,
                           actor, "API READ /results/" + resultId);
    if (!database_->logAudit(entry)) {
      return makeError(500, "internal_error", "Audit log failed",
                       database_->getLastError());
    }

    {
      std::string oidStr;
      auto parentOrder = database_->getOrder(result->getOrderId());
      if (parentOrder) { oidStr = parentOrder->getOrderId(); }
      return ApiResponse{200, "{\"data\":" + resultToJson(*result, oidStr) + "}",
                         "application/json"};
    }
  }

  // GET /api/v1/users - List all users (admin only)
  if (path == "/api/v1/users" && isGet) {
    // Check if user is admin
    if (!jwtPayload.has_value() || (jwtPayload->role != "ADMIN" && jwtPayload->role != "Administrator")) {
      return makeError(403, "forbidden", "Admin access required",
                       "Only administrators can list users.");
    }

    auto users = database_->getAllUsers();
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }

    std::ostringstream out;
    out << "{\"data\":[";
    for (size_t i = 0; i < users.size(); ++i) {
      if (i > 0) out << ",";
      out << userToJson(*users[i]); // Don't include password hash
    }
    out << "]}";

    return ApiResponse{200, out.str(), "application/json"};
  }

  // GET /api/v1/users/me - Get current user profile
  if (path == "/api/v1/users/me" && isGet) {
    if (!jwtPayload.has_value()) {
      return makeError(401, "unauthorized", "JWT required",
                       "Use JWT authentication to access profile.");
    }

    auto user = database_->getUser(jwtPayload->userId);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    if (!user) {
      return makeError(404, "not_found", "User not found",
                       "User profile not found.");
    }

    return ApiResponse{200, "{\"data\":" + userToJson(*user) + "}",
                       "application/json"};
  }

  // POST /api/v1/users - Create new user (admin only)
  if (path == "/api/v1/users" && isPost) {
    if (!jwtPayload.has_value() || (jwtPayload->role != "ADMIN" && jwtPayload->role != "Administrator")) {
      return makeError(403, "forbidden", "Admin access required",
                       "Only administrators can create users.");
    }

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

    auto usernameIt = payload.find("username");
    auto passwordIt = payload.find("password");
    auto roleIt = payload.find("role");

    if (usernameIt == payload.end() || usernameIt->second.empty()) {
      return makeError(400, "validation_error", "Missing username",
                       "Provide username in request body.");
    }
    if (passwordIt == payload.end() || passwordIt->second.empty()) {
      return makeError(400, "validation_error", "Missing password",
                       "Provide password in request body.");
    }

    // Validate username format
    std::string validationError;
    if (!validateUsername(usernameIt->second, validationError)) {
      return makeError(400, "validation_error", "Invalid username", validationError);
    }

    // Validate password strength
    if (!validatePassword(passwordIt->second, validationError)) {
      return makeError(400, "validation_error", "Invalid password", validationError);
    }

    core::User::Role role = core::User::Role::OPERATOR; // Default
    if (roleIt != payload.end() && !roleIt->second.empty()) {
      static const std::unordered_set<std::string> kValidRoles{
          "ADMIN", "OPERATOR", "VIEWER", "CUSTOM"};
      if (kValidRoles.find(roleIt->second) == kValidRoles.end()) {
        return makeError(400, "validation_error", "Invalid role",
                         "Role must be one of: ADMIN, OPERATOR, VIEWER, CUSTOM");
      }
      role = core::User::stringToRole(roleIt->second);
    }

    core::User newUser(usernameIt->second, "", role);
    newUser.setPassword(passwordIt->second);

    auto fullNameIt = payload.find("full_name");
    if (fullNameIt != payload.end()) {
      if (!fullNameIt->second.empty() && !validateStringLength(fullNameIt->second, 1, 255, validationError)) {
        return makeError(400, "validation_error", "Invalid full_name", validationError);
      }
      newUser.setFullName(fullNameIt->second);
    }
    auto emailIt = payload.find("email");
    if (emailIt != payload.end()) {
      if (!emailIt->second.empty() && !validateEmail(emailIt->second, validationError)) {
        return makeError(400, "validation_error", "Invalid email", validationError);
      }
      newUser.setEmail(emailIt->second);
    }
    auto activeIt = payload.find("active");
    if (activeIt != payload.end()) {
      newUser.setActive(activeIt->second == "true" || activeIt->second == "1");
    }

    if (!database_->createUser(newUser, actor)) {
      return makeDbErrorResponse(database_->getLastError());
    }

    auto created = database_->getUserByUsername(newUser.getUsername());
    database_->clearError(); // Read-back failure is non-fatal; fall back to in-memory object
    const core::User &responseUser = created ? *created : newUser;
    return ApiResponse{201,
                       "{\"data\":" + userToJson(responseUser) + "}",
                       "application/json"};
  }

  // PUT /api/v1/users/:id - Update user (admin only)
  if (isPut && path.rfind("/api/v1/users/", 0) == 0) {
    const std::string userIdStr =
        path.substr(std::string("/api/v1/users/").size());
    if (userIdStr.empty() || userIdStr == "me" || userIdStr.rfind("me/", 0) == 0) {
      // Handle "me" endpoint separately or fall through
      goto after_user_update;
    }

    // RBAC check before ID parsing — non-admins always get 403
    if (!jwtPayload.has_value() || (jwtPayload->role != "ADMIN" && jwtPayload->role != "Administrator")) {
      return makeError(403, "forbidden", "Admin access required",
                       "Only administrators can update users.");
    }

    int userId = 0;
    if (!parseIntValue(userIdStr, userId) || userId <= 0) {
      return makeError(400, "validation_error", "Invalid user_id",
                       "Provide numeric user_id.");
    }

    auto existing = database_->getUser(userId);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    if (!existing) {
      return makeError(404, "not_found", "User not found",
                       "Verify the user_id.");
    }

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

    core::User updated = *existing;

    auto usernameIt = payload.find("username");
    if (usernameIt != payload.end()) {
      // Validate username format
      std::string validationError;
      if (!validateUsername(usernameIt->second, validationError)) {
        return makeError(400, "validation_error", "Invalid username", validationError);
      }
      updated.setUsername(usernameIt->second);
    }
    auto roleIt = payload.find("role");
    if (roleIt != payload.end() && !roleIt->second.empty()) {
      static const std::unordered_set<std::string> kValidRoles{
          "ADMIN", "OPERATOR", "VIEWER", "CUSTOM"};
      if (kValidRoles.find(roleIt->second) == kValidRoles.end()) {
        return makeError(400, "validation_error", "Invalid role",
                         "Role must be one of: ADMIN, OPERATOR, VIEWER, CUSTOM");
      }
      updated.setRole(core::User::stringToRole(roleIt->second));
    }
    auto fullNameIt = payload.find("full_name");
    if (fullNameIt != payload.end()) {
      // Validate full_name length if provided and not empty
      if (!fullNameIt->second.empty()) {
        std::string validationError;
        if (!validateStringLength(fullNameIt->second, 1, 255, validationError)) {
          return makeError(400, "validation_error", "Invalid full_name", validationError);
        }
      }
      updated.setFullName(fullNameIt->second);
    }
    auto emailIt = payload.find("email");
    if (emailIt != payload.end()) {
      // Validate email format if provided and not empty
      if (!emailIt->second.empty()) {
        std::string validationError;
        if (!validateEmail(emailIt->second, validationError)) {
          return makeError(400, "validation_error", "Invalid email", validationError);
        }
      }
      updated.setEmail(emailIt->second);
    }
    auto activeIt = payload.find("active");
    if (activeIt != payload.end()) {
      updated.setActive(activeIt->second == "true" || activeIt->second == "1");
    }
    auto passwordIt = payload.find("password");
    if (passwordIt != payload.end() && !passwordIt->second.empty()) {
      // Validate password strength
      std::string validationError;
      if (!validatePassword(passwordIt->second, validationError)) {
        return makeError(400, "validation_error", "Invalid password", validationError);
      }
      updated.setPassword(passwordIt->second);
      updated.setMustChangePassword(false);
    }

    if (!database_->updateUser(updated, actor)) {
      return makeDbErrorResponse(database_->getLastError());
    }

    return ApiResponse{200,
                       "{\"data\":" + userToJson(updated) + "}",
                       "application/json"};
  }

after_user_update:

  // PUT /api/v1/users/me/password - Change own password
  if (path == "/api/v1/users/me/password" && isPut) {
    if (!jwtPayload.has_value()) {
      return makeError(401, "unauthorized", "JWT required",
                       "Use JWT authentication to change password.");
    }

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

    auto currentPwdIt = payload.find("current_password");
    auto newPwdIt = payload.find("new_password");

    if (currentPwdIt == payload.end() || currentPwdIt->second.empty()) {
      return makeError(400, "validation_error", "Missing current_password",
                       "Provide current_password in request body.");
    }
    if (newPwdIt == payload.end() || newPwdIt->second.empty()) {
      return makeError(400, "validation_error", "Missing new_password",
                       "Provide new_password in request body.");
    }

    // Validate new password strength
    std::string validationError;
    if (!validatePassword(newPwdIt->second, validationError)) {
      return makeError(400, "validation_error", "Invalid new_password", validationError);
    }

    auto user = database_->getUser(jwtPayload->userId);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    if (!user) {
      return makeError(404, "not_found", "User not found",
                       "User profile not found.");
    }

    // Verify current password
    if (!user->verifyPassword(currentPwdIt->second)) {
      return makeError(401, "unauthorized", "Invalid current password",
                       "Current password is incorrect.");
    }

    // Update password and clear forced-change flag
    user->setPassword(newPwdIt->second);
    user->setMustChangePassword(false);
    if (!database_->updateUser(*user, actor)) {
      return makeDbErrorResponse(database_->getLastError());
    }

    return ApiResponse{200, "{\"success\":true}", "application/json"};
  }

  // DELETE /api/v1/users/:id - Delete user (admin only)
  if (isDelete && path.rfind("/api/v1/users/", 0) == 0) {
    const std::string userIdStr =
        path.substr(std::string("/api/v1/users/").size());
    if (userIdStr.empty()) {
      return makeError(400, "validation_error", "Missing user_id",
                       "Provide user_id in URL path.");
    }

    // RBAC check before ID parsing — non-admins always get 403
    if (!jwtPayload.has_value() || (jwtPayload->role != "ADMIN" && jwtPayload->role != "Administrator")) {
      return makeError(403, "forbidden", "Admin access required",
                       "Only administrators can delete users.");
    }

    int userId = 0;
    if (!parseIntValue(userIdStr, userId) || userId <= 0) {
      return makeError(400, "validation_error", "Invalid user_id",
                       "Provide numeric user_id.");
    }

    if (jwtPayload.has_value() && jwtPayload->userId == userId) {
      return makeError(400, "validation_error", "Cannot delete own account",
                       "Administrators cannot deactivate their own account.");
    }

    auto existing = database_->getUser(userId);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    if (!existing) {
      return makeError(404, "not_found", "User not found",
                       "Verify the user_id.");
    }

    if (!database_->deleteUser(userId, actor)) {
      return makeDbErrorResponse(database_->getLastError());
    }

    return ApiResponse{204, "", "application/json"};
  }

  // GET /api/v1/audit - Get audit log (admin only)
  if (path == "/api/v1/audit" && isGet) {
    if (!jwtPayload.has_value() || (jwtPayload->role != "ADMIN" && jwtPayload->role != "Administrator")) {
      return makeError(403, "forbidden", "Admin access required",
                       "Only administrators can view audit logs.");
    }

    db::Database::AuditLogFilter filter;

    auto limitIt = query.find("limit");
    if (limitIt != query.end()) {
      int limitValue = 0;
      if (parseIntValue(limitIt->second, limitValue) && limitValue > 0) {
        // Validate and cap limit to prevent DoS
        std::string validationError;
        if (!validateAndCapLimit(limitValue, validationError)) {
          return makeError(400, "validation_error", "Invalid limit", validationError);
        }
        filter.limit = limitValue;
      }
    }

    auto userFilterIt = query.find("user");
    if (userFilterIt != query.end() && !userFilterIt->second.empty()) {
      filter.user = userFilterIt->second;
    }

    auto actionIt = query.find("action");
    if (actionIt != query.end() && !actionIt->second.empty()) {
      try {
        filter.action = core::AuditEntry::stringToAction(actionIt->second);
      } catch (...) {
        // Ignore invalid action
      }
    }

    auto entityIt = query.find("entity");
    if (entityIt != query.end() && !entityIt->second.empty()) {
      try {
        filter.entity = core::AuditEntry::stringToEntity(entityIt->second);
      } catch (...) {
        // Ignore invalid entity
      }
    }

    auto fromIt = query.find("from");
    if (fromIt != query.end()) {
      std::time_t ts{};
      if (parseTimeValue(fromIt->second, ts)) {
        filter.fromTime = ts;
      }
    }

    auto toIt = query.find("to");
    if (toIt != query.end()) {
      std::time_t ts{};
      if (parseTimeValue(toIt->second, ts)) {
        filter.toTime = ts;
      }
    }

    auto entries = database_->getAuditLogFiltered(filter);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }

    std::ostringstream out;
    out << "{\"data\":[";
    for (size_t i = 0; i < entries.size(); ++i) {
      if (i > 0) out << ",";
      out << auditEntryToJson(*entries[i]);
    }
    out << "]}";

    return ApiResponse{200, out.str(), "application/json"};
  }

  // GET /api/v1/stats - Get dashboard statistics
  if (path == "/api/v1/stats" && isGet) {
    db::Database::StatsFilter filter;

    auto fromIt = query.find("from");
    if (fromIt != query.end()) {
      std::time_t ts{};
      if (parseTimeValue(fromIt->second, ts)) {
        filter.fromDate = ts;
      }
    }

    auto toIt = query.find("to");
    if (toIt != query.end()) {
      std::time_t ts{};
      if (parseTimeValue(toIt->second, ts)) {
        filter.toDate = ts;
      }
    }

    auto sampleStats = database_->getSampleStats(filter);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    auto orderStats = database_->getOrderStats(filter);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    auto resultStats = database_->getResultStats(filter);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }

    auto orderPriorityStats = database_->getOrderPriorityStats();
    const int criticalCount = database_->getCriticalResultCount();

    std::ostringstream out;
    out << "{"
        << "\"samples\":" << statsToJson(sampleStats, "samples") << ","
        << "\"orders\":" << statsToJson(orderStats, "orders") << ","
        << "\"results\":" << statsToJson(resultStats, "results") << ","
        << "\"order_priority\":[";
    for (size_t i = 0; i < orderPriorityStats.size(); ++i) {
      if (i > 0) out << ",";
      out << "{" << "\"status\":" << jsonString(orderPriorityStats[i].status)
          << ",\"count\":" << orderPriorityStats[i].count << "}";
    }
    out << "],"
        << "\"critical_count\":" << criticalCount
        << "}";

    return ApiResponse{200, out.str(), "application/json"};
  }

  // GET /api/v1/audit/export — Audit-Log als CSV herunterladen (ADMIN only)
  if (isGet && path == "/api/v1/audit/export") {
    if (!jwtPayload.has_value() || (jwtPayload->role != "ADMIN" && jwtPayload->role != "Administrator")) {
      return makeError(403, "forbidden", "Admin access required",
                       "Only administrators can export the audit log.");
    }

    db::Database::AuditLogFilter filter;
    auto fromIt = query.find("from");
    if (fromIt != query.end()) {
      std::time_t ts{};
      if (parseTimeValue(fromIt->second, ts)) filter.fromTime = ts;
    }
    auto toIt = query.find("to");
    if (toIt != query.end()) {
      std::time_t ts{};
      if (parseTimeValue(toIt->second, ts)) filter.toTime = ts;
    }
    auto limitIt = query.find("limit");
    if (limitIt != query.end()) {
      int lim = 0;
      if (parseIntValue(limitIt->second, lim) && lim > 0) {
        filter.limit = std::min(lim, MAX_PAGINATION_LIMIT);
      }
    }

    static std::atomic<unsigned> exportSeq{0};
    const std::string tmpPath = "/tmp/opensylab_audit_export_"
        + std::to_string(::getpid()) + "_"
        + std::to_string(std::time(nullptr))
        + "_" + std::to_string(exportSeq.fetch_add(1)) + ".csv";
    int exportedCount = 0;
    if (!database_->exportAuditLogToCsv(tmpPath, filter, actor, exportedCount)) {
      std::remove(tmpPath.c_str());
      return makeDbErrorResponse(database_->getLastError());
    }

    // Read file into response body
    std::ifstream f(tmpPath, std::ios::binary);
    if (!f.is_open()) {
      std::remove(tmpPath.c_str());
      return makeError(500, "internal_error", "Export failed",
                       "Could not read export file.");
    }
    std::ostringstream buf;
    buf << f.rdbuf();
    std::remove(tmpPath.c_str());

    return ApiResponse{200, buf.str(), "text/csv; charset=utf-8"};
  }

  // POST /api/v1/hl7/import - HL7 v2.5.1 Import
  if (method == "post" && path == "/api/v1/hl7/import") {
    if (effectiveRole == "VIEWER" || effectiveRole == "CUSTOM") {
      return makeError(403, "forbidden", "Insufficient permissions",
                       "OPERATOR or ADMIN role required.");
    }
    const std::string hl7Body = trimLeadingNewlines(request.body);
    if (hl7Body.empty()) {
      return makeError(400, "validation_error", "Missing request body",
                       "Provide HL7 v2.5.1 message in request body.");
    }
    utils::Hl7Exchange exchange(database_);
    utils::Hl7Exchange::ImportSummary summary;
    if (!exchange.importOruR01Message(hl7Body, actor, summary)) {
      return makeError(422, "import_error", "HL7 import failed",
                       summary.lastError);
    }
    std::ostringstream jsonBody;
    jsonBody << "{\"imported\":{"
             << "\"samples\":" << summary.samplesCreated << ","
             << "\"orders\":" << summary.ordersCreated << ","
             << "\"results\":" << summary.resultsCreated
             << "}}";
    return ApiResponse{200, jsonBody.str(), "application/json"};
  }

  // GET /api/v1/hl7/export/{id} - HL7 v2.5.1 Export
  if (method == "get" && path.rfind("/api/v1/hl7/export/", 0) == 0) {
    if (effectiveRole == "VIEWER" || effectiveRole == "CUSTOM") {
      return makeError(403, "forbidden", "Insufficient permissions",
                       "OPERATOR or ADMIN role required.");
    }
    const std::string pathSegment = path.substr(path.rfind('/') + 1);
    if (pathSegment.empty()) {
      return makeError(400, "validation_error", "Missing sample id",
                       "Provide sample id in URL path.");
    }
    int sampleId = 0;
    if (!parseIntValue(pathSegment, sampleId) || sampleId <= 0) {
      return makeError(400, "validation_error", "Invalid sample id",
                       "Provide numeric sample id.");
    }
    auto sample = database_->getSample(sampleId);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    if (!sample) {
      return makeError(404, "not_found", "Sample not found",
                       "Verify the sample id.");
    }
    auto orders = database_->getOrdersBySampleId(sample->getSampleId());
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    if (orders.empty()) {
      return makeError(404, "not_found", "No orders found",
                       "Sample has no associated orders.");
    }
    const core::Order &order = *orders[0];
    auto resultPtrs = database_->getTestResultsByOrderId(order.getId(), 1000, 0);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }

    std::vector<core::TestResult> resultCopies;
    resultCopies.reserve(resultPtrs.size());
    for (const auto &r : resultPtrs) {
      resultCopies.push_back(*r);
    }

    utils::Hl7Exchange exchange(database_);
    std::string hl7Body = exchange.exportOruR01Message(*sample, order, resultCopies);
    return ApiResponse{200, hl7Body, "application/hl7-v2"};
  }

  // POST /api/v1/fhir/import - FHIR R4 Import
  if (method == "post" && path == "/api/v1/fhir/import") {
    if (effectiveRole == "VIEWER" || effectiveRole == "CUSTOM") {
      return makeError(403, "forbidden", "Insufficient permissions",
                       "OPERATOR or ADMIN role required.");
    }
    const std::string fhirBody = trimLeadingNewlines(request.body);
    if (fhirBody.empty()) {
      return makeError(400, "validation_error", "Missing request body",
                       "Provide FHIR R4 Bundle JSON in request body.");
    }
    utils::FhirExchange exchange(database_);
    utils::FhirExchange::ImportSummary summary;
    if (!exchange.importBundle(fhirBody, actor, summary)) {
      return makeError(422, "import_error", "FHIR import failed",
                       summary.lastError);
    }
    std::ostringstream jsonBody;
    jsonBody << "{\"imported\":{"
             << "\"samples\":" << summary.samplesCreated << ","
             << "\"orders\":" << summary.ordersCreated << ","
             << "\"results\":" << summary.resultsCreated
             << "}}";
    return ApiResponse{200, jsonBody.str(), "application/json"};
  }

  // GET /api/v1/fhir/export/{id} - FHIR R4 Export
  if (method == "get" && path.rfind("/api/v1/fhir/export/", 0) == 0) {
    if (effectiveRole == "VIEWER" || effectiveRole == "CUSTOM") {
      return makeError(403, "forbidden", "Insufficient permissions",
                       "OPERATOR or ADMIN role required.");
    }
    const std::string pathSegment = path.substr(path.rfind('/') + 1);
    if (pathSegment.empty()) {
      return makeError(400, "validation_error", "Missing sample id",
                       "Provide sample id in URL path.");
    }
    int sampleId = 0;
    if (!parseIntValue(pathSegment, sampleId) || sampleId <= 0) {
      return makeError(400, "validation_error", "Invalid sample id",
                       "Provide numeric sample id.");
    }
    auto sample = database_->getSample(sampleId);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    if (!sample) {
      return makeError(404, "not_found", "Sample not found",
                       "Verify the sample id.");
    }
    auto orders = database_->getOrdersBySampleId(sample->getSampleId());
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }
    if (orders.empty()) {
      return makeError(404, "not_found", "No orders found",
                       "Sample has no associated orders.");
    }
    const core::Order &order = *orders[0];
    auto resultPtrs = database_->getTestResultsByOrderId(order.getId(), 1000, 0);
    if (database_->hasError()) {
      return makeDbErrorResponse(database_->getLastError());
    }

    std::vector<core::TestResult> resultCopies;
    resultCopies.reserve(resultPtrs.size());
    for (const auto &r : resultPtrs) {
      resultCopies.push_back(*r);
    }

    utils::FhirExchange exchange(database_);
    std::string fhirBody = exchange.exportBundle(*sample, order, resultCopies);
    return ApiResponse{200, fhirBody, "application/fhir+json"};
  }

  return makeError(404, "not_found", "Endpoint not found",
                   "Check the requested path.");
}

ApiServer::ApiServer(std::shared_ptr<db::Database> database, int port)
    : database_(std::move(database)),
      router_(database_),
      port_(port),
      serverFd_(-1),
      running_(false),
      tlsContext_(nullptr),
      tlsEnabled_(false) {
  // CORS-Origin einmalig lesen (verhindert doppeltes getenv in TLS + Plain)
  const char* cors = std::getenv("OPENSYLAB_CORS_ORIGIN");
  corsOrigin_ = (cors && *cors) ? std::string(cors) : "http://localhost:5173";
  const std::string trim_cors = corsOrigin_.substr(corsOrigin_.find_first_not_of(" \t"));
  corsOrigin_ = trim_cors.substr(0, trim_cors.find_last_not_of(" \t") + 1);

  // JWT-Secret-Validierung
  const char* jwtSecret = std::getenv("OPENSYLAB_JWT_SECRET");
  if (!jwtSecret || std::string(jwtSecret).find("dev") != std::string::npos ||
      std::string(jwtSecret).find("change") != std::string::npos ||
      std::string(jwtSecret).length() < 32) {
    std::cerr << "[SECURITY WARNING] Unsicheres oder fehlendes JWT-Secret. "
                 "Setze OPENSYLAB_JWT_SECRET (min. 32 Zeichen) fuer Produktion!\n";
  }
}

ApiServer::~ApiServer() {
  running_ = false;
  if (serverFd_ >= 0) {
    close(serverFd_);
    serverFd_ = -1;
  }
}

bool ApiServer::isRateLimited(const std::string &ip) {
  std::lock_guard<std::mutex> lock(loginMutex_);
  auto now = std::chrono::steady_clock::now();

  // Prune expired entries to bound map growth (max 10000 IPs)
  if (loginAttempts_.size() > 10000) {
    for (auto it = loginAttempts_.begin(); it != loginAttempts_.end(); ) {
      if (now - it->second.second > std::chrono::seconds(60)) {
        it = loginAttempts_.erase(it);
      } else {
        ++it;
      }
    }
  }

  auto &entry = loginAttempts_[ip];
  // Fenster: 60 Sekunden, max. 10 Versuche
  if (now - entry.second > std::chrono::seconds(60)) {
    entry = {1, now};
    return false;
  }
  return ++entry.first > 10;
}

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
    close(serverFd_);
    serverFd_ = -1;
    return false;
  }

  if (listen(serverFd_, 10) < 0) {
    close(serverFd_);
    serverFd_ = -1;
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

bool ApiServer::enableTls(const std::string& certPath, const std::string& keyPath) {
  if (running_) {
    return false; // Cannot enable TLS while server is running
  }

  tlsContext_ = std::make_unique<TlsContext>();
  if (!tlsContext_->initialize(certPath, keyPath)) {
    tlsContext_.reset();
    return false;
  }

  tlsEnabled_ = true;
  return true;
}

void ApiServer::disableTls() {
  if (running_) {
    return; // Cannot disable TLS while server is running
  }

  tlsEnabled_ = false;
  tlsContext_.reset();
}

bool ApiServer::isTlsEnabled() const {
  return tlsEnabled_ && tlsContext_ != nullptr && tlsContext_->isInitialized();
}

std::string ApiServer::getTlsError() const {
  if (tlsContext_ != nullptr) {
    return tlsContext_->getLastError();
  }
  return "TLS context not initialized";
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

    struct timeval tv{30, 0};
    if (setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ||
        setsockopt(clientFd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
      close(clientFd);
      continue;
    }

    if (isTlsEnabled()) {
      handleClientTls(clientFd);
    } else {
      handleClientPlain(clientFd);
    }

    close(clientFd);
  }
}

void ApiServer::handleClient(int clientFd) {
  if (isTlsEnabled()) {
    handleClientTls(clientFd);
  } else {
    handleClientPlain(clientFd);
  }
}

void ApiServer::handleClientTls(int clientFd) {
  // Create SSL connection
  SSL* ssl = tlsContext_->createSslConnection(clientFd);
  if (ssl == nullptr) {
    // SSL handshake failed - connection will be closed
    return;
  }

  char buffer[8192];
  const int readBytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);
  if (readBytes <= 0) {
    TlsContext::freeSslConnection(ssl);
    return;
  }
  buffer[readBytes] = '\0';

  std::istringstream requestStream{std::string(buffer)};
  std::string requestLine;
  if (!std::getline(requestStream, requestLine)) {
    TlsContext::freeSslConnection(ssl);
    return;
  }
  requestLine = trim(requestLine);

  std::istringstream lineStream(requestLine);
  ApiRequest request;
  lineStream >> request.method;
  // Normalize method to uppercase for consistent comparisons
  for (char &ch : request.method) { ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch))); }
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
  std::string body = bodyStream.str(); // Keep raw for Content-Length accumulation
  auto lengthIt = request.headers.find("content-length");
  if (lengthIt != request.headers.end()) {
    try {
      constexpr size_t kMaxBodySize = 10 * 1024 * 1024;
          size_t length = std::min(static_cast<size_t>(std::stoul(lengthIt->second)), kMaxBodySize);
      while (body.size() < length) {
        const int more = SSL_read(ssl, buffer, sizeof(buffer) - 1);
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
  request.body = trimLeadingNewlines(body);

  // Rate-Limiting fuer Login — keyed by real TCP peer address (not spoofable)
  const std::string _rlPath = [&]{ auto p = request.path; auto q = p.find('?'); return q != std::string::npos ? p.substr(0, q) : p; }();
  if (request.method == "POST" && _rlPath == "/api/v1/auth/login") {
    sockaddr_in peerAddrTls{};
    socklen_t peerLenTls = sizeof(peerAddrTls);
    std::string clientIp;
    if (getpeername(clientFd, reinterpret_cast<sockaddr *>(&peerAddrTls), &peerLenTls) == 0) {
      clientIp = inet_ntoa(peerAddrTls.sin_addr);
    } else {
      clientIp = "conn:" + std::to_string(clientFd);
    }
    if (isRateLimited(clientIp)) {
      const std::string rlBody =
          R"({"error":{"code":"rate_limit","message":"Zu viele Login-Versuche","hint":"Bitte 60 Sekunden warten."}})";
      std::ostringstream rlOut;
      rlOut << "HTTP/1.1 429 Too Many Requests\r\n"
            << "Content-Type: application/json\r\n"
            << "Access-Control-Allow-Origin: " << corsOrigin_ << "\r\n"
            << "X-Content-Type-Options: nosniff\r\n"
            << "X-Frame-Options: DENY\r\n"
            << "X-XSS-Protection: 1; mode=block\r\n"
            << "Strict-Transport-Security: max-age=31536000\r\n"
            << "Content-Length: " << rlBody.size() << "\r\n\r\n"
            << rlBody;
      const std::string rlStr = rlOut.str();
      SSL_write(ssl, rlStr.c_str(), static_cast<int>(rlStr.size()));
      TlsContext::freeSslConnection(ssl);
      return;
    }
  }

  ApiResponse response = router_.handleRequest(request);

  std::ostringstream out;
  out << "HTTP/1.1 " << response.status << " " << statusMessage(response.status)
      << "\r\n";
  out << "Content-Type: " << response.contentType << "\r\n";
  out << "Content-Length: " << response.body.size() << "\r\n";
  out << "Access-Control-Allow-Origin: " << corsOrigin_ << "\r\n";
  out << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
  out << "Access-Control-Allow-Headers: Content-Type, X-API-Key, Authorization\r\n";
  out << "Access-Control-Max-Age: 3600\r\n";
  out << "X-Content-Type-Options: nosniff\r\n";
  out << "X-Frame-Options: DENY\r\n";
  out << "X-XSS-Protection: 1; mode=block\r\n";
  out << "Strict-Transport-Security: max-age=31536000\r\n";
  out << "Connection: close\r\n\r\n";
  out << response.body;

  const std::string responseStr = out.str();
  SSL_write(ssl, responseStr.c_str(), static_cast<int>(responseStr.size()));

  TlsContext::freeSslConnection(ssl);
}

void ApiServer::handleClientPlain(int clientFd) {
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
  // Normalize method to uppercase for consistent comparisons
  for (char &ch : request.method) { ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch))); }
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
  std::string body = bodyStream.str(); // Keep raw for Content-Length accumulation
  auto lengthIt = request.headers.find("content-length");
  if (lengthIt != request.headers.end()) {
    try {
      constexpr size_t kMaxBodySize = 10 * 1024 * 1024;
          size_t length = std::min(static_cast<size_t>(std::stoul(lengthIt->second)), kMaxBodySize);
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
  request.body = trimLeadingNewlines(body);

  // Rate-Limiting fuer Login — keyed by real TCP peer address (not spoofable)
  const std::string _rlPath = [&]{ auto p = request.path; auto q = p.find('?'); return q != std::string::npos ? p.substr(0, q) : p; }();
  if (request.method == "POST" && _rlPath == "/api/v1/auth/login") {
    sockaddr_in peerAddrPlain{};
    socklen_t peerLenPlain = sizeof(peerAddrPlain);
    std::string clientIp;
    if (getpeername(clientFd, reinterpret_cast<sockaddr *>(&peerAddrPlain), &peerLenPlain) == 0) {
      clientIp = inet_ntoa(peerAddrPlain.sin_addr);
    } else {
      clientIp = "conn:" + std::to_string(clientFd);
    }
    if (isRateLimited(clientIp)) {
      const std::string rlBody =
          R"({"error":{"code":"rate_limit","message":"Zu viele Login-Versuche","hint":"Bitte 60 Sekunden warten."}})";
      std::ostringstream rlOut;
      rlOut << "HTTP/1.1 429 Too Many Requests\r\n"
            << "Content-Type: application/json\r\n"
            << "Access-Control-Allow-Origin: " << corsOrigin_ << "\r\n"
            << "X-Content-Type-Options: nosniff\r\n"
            << "X-Frame-Options: DENY\r\n"
            << "X-XSS-Protection: 1; mode=block\r\n"
            << "Content-Length: " << rlBody.size() << "\r\n\r\n"
            << rlBody;
      const std::string rlStr = rlOut.str();
      send(clientFd, rlStr.c_str(), rlStr.size(), MSG_NOSIGNAL);
      return;
    }
  }

  ApiResponse response = router_.handleRequest(request);

  std::ostringstream out;
  out << "HTTP/1.1 " << response.status << " " << statusMessage(response.status)
      << "\r\n";
  out << "Content-Type: " << response.contentType << "\r\n";
  out << "Content-Length: " << response.body.size() << "\r\n";
  out << "Access-Control-Allow-Origin: " << corsOrigin_ << "\r\n";
  out << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
  out << "Access-Control-Allow-Headers: Content-Type, X-API-Key, Authorization\r\n";
  out << "Access-Control-Max-Age: 3600\r\n";
  out << "X-Content-Type-Options: nosniff\r\n";
  out << "X-Frame-Options: DENY\r\n";
  out << "X-XSS-Protection: 1; mode=block\r\n";
  out << "Connection: close\r\n\r\n";
  out << response.body;

  const std::string responseStr = out.str();
  send(clientFd, responseStr.c_str(), responseStr.size(), MSG_NOSIGNAL);
}

} // namespace api
} // namespace opensylab
