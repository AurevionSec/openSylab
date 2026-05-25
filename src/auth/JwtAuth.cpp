#include "auth/JwtAuth.h"
#include <jwt-cpp/jwt.h>
#include <chrono>
#include <random>
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

static std::string generateJti(int userId) {
  std::random_device rd;
  std::seed_seq seq{rd(), rd(), rd(), rd()};
  std::mt19937_64 rng(seq);
  const uint64_t r1 = std::uniform_int_distribution<uint64_t>{}(rng);
  const uint64_t r2 = std::uniform_int_distribution<uint64_t>{}(rng);
  const auto ts = static_cast<uint64_t>(
      std::chrono::system_clock::now().time_since_epoch().count());
  return std::to_string(userId) + "_" + std::to_string(ts) + "_" +
         std::to_string(r1) + "_" + std::to_string(r2);
}

std::string JwtAuth::generateToken(int userId, const std::string &username,
                                   const std::string &role) {
  auto now = std::chrono::system_clock::now();
  auto exp = now + std::chrono::minutes(config_.expirationMinutes);

  auto token = jwt::create()
                   .set_issuer(config_.issuer)
                   .set_type("JWT")
                   .set_id(generateJti(userId))
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

    // Get userId (stored as string, convert to int with bounds check)
    if (decoded.has_payload_claim("userId")) {
      const std::string userIdStr = decoded.get_payload_claim("userId").as_string();
      size_t idx = 0;
      const long long parsed = std::stoll(userIdStr, &idx);
      if (idx != userIdStr.size() || parsed <= 0 || parsed > INT_MAX) {
        return std::nullopt;
      }
      payload.userId = static_cast<int>(parsed);
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
      return std::nullopt;
    }

    if (decoded.has_issuer()) {
      payload.issuer = decoded.get_issuer();
    } else {
      payload.issuer = "";
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

int64_t JwtAuth::getSecondsUntilExpiration(const TokenPayload &payload) {
  auto now = std::chrono::system_clock::now();
  auto nowTime = std::chrono::system_clock::to_time_t(now);
  return static_cast<int64_t>(payload.exp) - static_cast<int64_t>(nowTime);
}

} // namespace auth
} // namespace opensylab
