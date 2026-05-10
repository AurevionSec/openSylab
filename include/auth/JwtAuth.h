#ifndef OPENSYLAB_JWTAUTH_H
#define OPENSYLAB_JWTAUTH_H

#include <ctime>
#include <optional>
#include <string>

namespace opensylab {
namespace auth {

/**
 * @brief JWT Token Authentication Manager
 *
 * Provides JWT token generation, validation, and expiration checking
 * for secure API authentication using the HS256 algorithm.
 */
class JwtAuth {
public:
  /**
   * @brief Configuration for JWT token generation
   */
  struct JwtConfig {
    std::string secret;              ///< Secret key for signing (min 256 bits)
    int expirationMinutes = 60;      ///< Token expiration time in minutes
    std::string issuer = "opensylab"; ///< Token issuer identifier
  };

  /**
   * @brief Decoded JWT token payload
   */
  struct TokenPayload {
    int userId = 0;          ///< User ID from database
    std::string username;    ///< Username
    std::string role;        ///< User role (admin, user, etc.)
    std::time_t exp = 0;     ///< Expiration timestamp (Unix epoch)
    std::time_t iat = 0;     ///< Issued at timestamp (Unix epoch)
    std::string issuer;      ///< Token issuer
  };

  /**
   * @brief Construct JWT authentication manager
   * @param config JWT configuration (secret, expiration, issuer)
   */
  explicit JwtAuth(const JwtConfig &config);

  /**
   * @brief Generate JWT token for user
   * @param userId User database ID
   * @param username Username
   * @param role User role
   * @return JWT token string (Base64URL encoded)
   */
  [[nodiscard]] std::string generateToken(int userId,
                                          const std::string &username,
                                          const std::string &role);

  /**
   * @brief Validate and decode JWT token
   * @param token JWT token string
   * @return Token payload if valid, std::nullopt if invalid or expired
   */
  [[nodiscard]] std::optional<TokenPayload>
  validateToken(const std::string &token);

  /**
   * @brief Check if token payload is expired
   * @param payload Token payload to check
   * @return true if expired, false otherwise
   */
  [[nodiscard]] static bool isTokenExpired(const TokenPayload &payload);

  /**
   * @brief Get time until token expiration
   * @param payload Token payload
   * @return Seconds until expiration (negative if expired)
   */
  [[nodiscard]] static int64_t getSecondsUntilExpiration(
      const TokenPayload &payload);

  /**
   * @brief Get configuration
   * @return Current JWT configuration
   */
  [[nodiscard]] const JwtConfig &getConfig() const { return config_; }

private:
  JwtConfig config_; ///< JWT configuration
};

} // namespace auth
} // namespace opensylab

#endif // OPENSYLAB_JWTAUTH_H
