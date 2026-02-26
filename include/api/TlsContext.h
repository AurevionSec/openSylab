#ifndef OPENSYLAB_TLSCONTEXT_H
#define OPENSYLAB_TLSCONTEXT_H

#include <memory>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace opensylab {
namespace api {

/**
 * @brief TLS Context Manager for OpenSSL
 *
 * Manages SSL/TLS context initialization, certificate loading,
 * and SSL connection handling for the ApiServer.
 *
 * Supports both production certificates and self-signed development certificates.
 */
class TlsContext {
public:
  /**
   * @brief Constructor - initializes OpenSSL library
   */
  TlsContext();

  /**
   * @brief Destructor - cleans up SSL context
   */
  ~TlsContext();

  // Prevent copying
  TlsContext(const TlsContext&) = delete;
  TlsContext& operator=(const TlsContext&) = delete;

  /**
   * @brief Initialize SSL context with certificate and private key
   *
   * @param certPath Path to PEM-encoded certificate file
   * @param keyPath Path to PEM-encoded private key file
   * @return true if initialization successful, false otherwise
   */
  bool initialize(const std::string& certPath, const std::string& keyPath);

  /**
   * @brief Create SSL connection for a socket file descriptor
   *
   * @param sockfd Socket file descriptor
   * @return SSL* pointer or nullptr on failure
   */
  SSL* createSslConnection(int sockfd);

  /**
   * @brief Check if TLS context is initialized
   *
   * @return true if initialized, false otherwise
   */
  bool isInitialized() const;

  /**
   * @brief Get last SSL error message
   *
   * @return Error message string
   */
  std::string getLastError() const;

  /**
   * @brief Free an SSL connection
   *
   * @param ssl SSL connection to free
   */
  static void freeSslConnection(SSL* ssl);

private:
  SSL_CTX* sslContext_;
  bool initialized_;
  mutable std::string lastError_;

  /**
   * @brief Load error string from OpenSSL error queue
   */
  void loadSslError();
};

} // namespace api
} // namespace opensylab

#endif // OPENSYLAB_TLSCONTEXT_H
