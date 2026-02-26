#include "api/TlsContext.h"
#include <cstring>
#include <sstream>

namespace opensylab {
namespace api {

TlsContext::TlsContext()
    : sslContext_(nullptr),
      initialized_(false),
      lastError_() {
  // Initialize OpenSSL library
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();
}

TlsContext::~TlsContext() {
  if (sslContext_ != nullptr) {
    SSL_CTX_free(sslContext_);
    sslContext_ = nullptr;
  }

  // Cleanup OpenSSL
  EVP_cleanup();
  ERR_free_strings();
}

bool TlsContext::initialize(const std::string& certPath, const std::string& keyPath) {
  if (initialized_) {
    lastError_ = "TLS context already initialized";
    return false;
  }

  // Create new SSL context using TLS method
  const SSL_METHOD* method = TLS_server_method();
  sslContext_ = SSL_CTX_new(method);

  if (sslContext_ == nullptr) {
    loadSslError();
    return false;
  }

  // Set minimum TLS version to TLS 1.2 for security
  SSL_CTX_set_min_proto_version(sslContext_, TLS1_2_VERSION);

  // Load certificate file
  if (SSL_CTX_use_certificate_file(sslContext_, certPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
    loadSslError();
    SSL_CTX_free(sslContext_);
    sslContext_ = nullptr;
    return false;
  }

  // Load private key file
  if (SSL_CTX_use_PrivateKey_file(sslContext_, keyPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
    loadSslError();
    SSL_CTX_free(sslContext_);
    sslContext_ = nullptr;
    return false;
  }

  // Verify that private key matches certificate
  if (!SSL_CTX_check_private_key(sslContext_)) {
    lastError_ = "Private key does not match certificate";
    SSL_CTX_free(sslContext_);
    sslContext_ = nullptr;
    return false;
  }

  // Configure secure cipher suites (exclude weak ciphers)
  if (!SSL_CTX_set_cipher_list(sslContext_,
      "ECDHE-ECDSA-AES128-GCM-SHA256:"
      "ECDHE-RSA-AES128-GCM-SHA256:"
      "ECDHE-ECDSA-AES256-GCM-SHA384:"
      "ECDHE-RSA-AES256-GCM-SHA384:"
      "ECDHE-ECDSA-CHACHA20-POLY1305:"
      "ECDHE-RSA-CHACHA20-POLY1305:"
      "DHE-RSA-AES128-GCM-SHA256:"
      "DHE-RSA-AES256-GCM-SHA384")) {
    loadSslError();
    SSL_CTX_free(sslContext_);
    sslContext_ = nullptr;
    return false;
  }

  // Enable session caching for better performance
  SSL_CTX_set_session_cache_mode(sslContext_, SSL_SESS_CACHE_SERVER);

  initialized_ = true;
  lastError_.clear();
  return true;
}

SSL* TlsContext::createSslConnection(int sockfd) {
  if (!initialized_ || sslContext_ == nullptr) {
    lastError_ = "TLS context not initialized";
    return nullptr;
  }

  SSL* ssl = SSL_new(sslContext_);
  if (ssl == nullptr) {
    loadSslError();
    return nullptr;
  }

  // Bind SSL to socket
  if (!SSL_set_fd(ssl, sockfd)) {
    loadSslError();
    SSL_free(ssl);
    return nullptr;
  }

  // Perform SSL handshake
  int ret = SSL_accept(ssl);
  if (ret <= 0) {
    int err = SSL_get_error(ssl, ret);
    std::ostringstream oss;
    oss << "SSL handshake failed: ";

    switch (err) {
      case SSL_ERROR_ZERO_RETURN:
        oss << "Connection closed";
        break;
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        oss << "Non-blocking I/O";
        break;
      case SSL_ERROR_SYSCALL:
        oss << "I/O error";
        break;
      case SSL_ERROR_SSL:
        loadSslError();
        oss << lastError_;
        break;
      default:
        oss << "Unknown error " << err;
        break;
    }

    lastError_ = oss.str();
    SSL_free(ssl);
    return nullptr;
  }

  lastError_.clear();
  return ssl;
}

bool TlsContext::isInitialized() const {
  return initialized_;
}

std::string TlsContext::getLastError() const {
  return lastError_;
}

void TlsContext::freeSslConnection(SSL* ssl) {
  if (ssl != nullptr) {
    SSL_shutdown(ssl);
    SSL_free(ssl);
  }
}

void TlsContext::loadSslError() {
  char buf[256];
  unsigned long err = ERR_get_error();
  if (err != 0) {
    ERR_error_string_n(err, buf, sizeof(buf));
    lastError_ = std::string(buf);
  } else {
    lastError_ = "Unknown SSL error";
  }
}

} // namespace api
} // namespace opensylab
