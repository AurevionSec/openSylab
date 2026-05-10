#include "auth/JwtAuth.h"
#include <jwt-cpp/jwt.h>
#include <chrono>
#include <stdexcept>

namespace opensylab {
namespace auth {

JwtAuth::JwtAuth(const JwtConfig &config) : config_(config) {
  if (config_.secret.empty()) {
    throw std::invalid_argument("JWT secret cannot be empty");
  }
  if (config_.secret.length() < 32) {
    throw std::invalid_argument(
        "JWT secret must be at least 32 characters (256 bits) for security");
  }
  if (config_.expirationMinutes <= 0) {
    throw std::invalid_argument("JWT expiration must be positive");
  }
}

std::string JwtAuth::generateToken(int userId, const std::string &username,
                                   const std::string &role) {
  auto now = std::chrono::system_clock::now();
  auto exp = now + std::chrono::minutes(config_.expirationMinutes);

  auto token = jwt::create()
                   .set_issuer(config_.issuer)
                   .set_type("JWT")
                   .set_issued_at(now)
                   .set_expires_at(exp)
                   .set_payload_claim("userId", jwt::claim(std::to_string(userId)))
                   .set_payload_claim("username", jwt::claim(username))
                   .set_payload_claim("role", jwt::claim(role))
                   .sign(jwt::algorithm::hs256{config_.secret});

  return token;
}

std::optional<JwtAuth::TokenPayload>
JwtAuth::validateToken(const std::string &token) {
  try {
    // Decode and verify token
    auto decoded = jwt::decode(token);

    // Verify signature and claims
    auto verifier = jwt::verify()
                        .allow_algorithm(jwt::algorithm::hs256{config_.secret})
                        .with_issuer(config_.issuer);

    verifier.verify(decoded);

    // Extract payload
    TokenPayload payload;

    // Get userId (stored as string, convert to int)
    if (decoded.has_payload_claim("userId")) {
      std::string userIdStr = decoded.get_payload_claim("userId").as_string();
      payload.userId = std::stoi(userIdStr);
    } else {
      return std::nullopt;
    }

    // Get username
    if (decoded.has_payload_claim("username")) {
      payload.username = decoded.get_payload_claim("username").as_string();
    } else {
      return std::nullopt;
    }

    // Get role
    if (decoded.has_payload_claim("role")) {
      payload.role = decoded.get_payload_claim("role").as_string();
    } else {
      return std::nullopt;
    }

    // Get timestamps
    if (decoded.has_expires_at()) {
      payload.exp = std::chrono::system_clock::to_time_t(
          decoded.get_expires_at());
    } else {
      return std::nullopt;
    }

    if (decoded.has_issued_at()) {
      payload.iat = std::chrono::system_clock::to_time_t(
          decoded.get_issued_at());
    } else {
      payload.iat = 0;
    }

    if (decoded.has_issuer()) {
      payload.issuer = decoded.get_issuer();
    } else {
      payload.issuer = "";
    }

    // Check if expired
    if (isTokenExpired(payload)) {
      return std::nullopt;
    }

    return payload;

  } catch (const jwt::error::token_verification_exception &e) {
    // Token verification failed (invalid signature, expired, etc.)
    return std::nullopt;
  } catch (const std::exception &e) {
    // Other errors (malformed token, etc.)
    return std::nullopt;
  }
}

bool JwtAuth::isTokenExpired(const TokenPayload &payload) {
  auto now = std::chrono::system_clock::now();
  auto nowTime = std::chrono::system_clock::to_time_t(now);
  return nowTime > payload.exp;
}

int JwtAuth::getSecondsUntilExpiration(const TokenPayload &payload) {
  auto now = std::chrono::system_clock::now();
  auto nowTime = std::chrono::system_clock::to_time_t(now);
  return static_cast<int>(payload.exp - nowTime);
}

} // namespace auth
} // namespace opensylab
