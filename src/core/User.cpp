#include "core/User.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>
#include <vector>
#include <algorithm>

namespace opensylab {
namespace core {

// Konstruktoren
User::User()
    : id_(0), username_(""), passwordHash_(""), role_(Role::VIEWER),
      roleName_(roleToString(role_)), active_(true), lastLogin_(0),
      createdDate_(std::time(nullptr)), fullName_(""), email_("") {}

User::User(const std::string &username, const std::string &passwordHash,
           Role role)
    : id_(0), username_(username), passwordHash_(passwordHash), role_(role),
      roleName_(roleToString(role)), active_(true), lastLogin_(0),
      createdDate_(std::time(nullptr)), fullName_(""), email_("") {}

// Helper function: Base64 encoding for binary data
static std::string base64Encode(const unsigned char* data, size_t len) {
  static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  std::string result;
  result.reserve(((len + 2) / 3) * 4);

  for (size_t i = 0; i < len; i += 3) {
    unsigned int val = data[i] << 16;
    if (i + 1 < len) val |= data[i + 1] << 8;
    if (i + 2 < len) val |= data[i + 2];

    result.push_back(base64_chars[(val >> 18) & 0x3F]);
    result.push_back(base64_chars[(val >> 12) & 0x3F]);
    result.push_back(i + 1 < len ? base64_chars[(val >> 6) & 0x3F] : '=');
    result.push_back(i + 2 < len ? base64_chars[val & 0x3F] : '=');
  }

  return result;
}

// Helper function: Base64 decoding
static std::vector<unsigned char> base64Decode(const std::string& encoded) {
  static const unsigned char base64_table[256] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
  };

  std::vector<unsigned char> result;
  result.reserve((encoded.size() / 4) * 3);

  for (size_t i = 0; i < encoded.size(); i += 4) {
    // Get 4 decoded values (0-63 or 64 for invalid/padding)
    unsigned char v[4] = {64, 64, 64, 64};
    int validChars = 0;

    for (int j = 0; j < 4 && i + j < encoded.size(); j++) {
      unsigned char c = encoded[i + j];
      if (c != '=') {
        v[j] = base64_table[c];
        if (v[j] != 64) {
          validChars = j + 1;
        }
      }
    }

    // Decode the 4 6-bit values into 3 bytes
    // 4 chars (24 bits) -> 3 bytes (24 bits)
    // byte1 = vvvvvv vv____
    // byte2 = ____vv vvvv__
    // byte3 = ______vv vvvvvv
    if (validChars >= 2) {
      result.push_back((v[0] << 2) | (v[1] >> 4));
    }
    if (validChars >= 3) {
      result.push_back(((v[1] & 0x0F) << 4) | (v[2] >> 2));
    }
    if (validChars >= 4) {
      result.push_back(((v[2] & 0x03) << 6) | v[3]);
    }
  }

  return result;
}

// PBKDF2-SHA256 password hashing (SECURE - production-ready)
std::string User::hashPassword(const std::string &password) {
  if (password.empty()) {
    return "";
  }

  // PBKDF2 parameters
  const int iterations = 210000; // OWASP recommendation for PBKDF2-SHA256 (2023)
  const int saltLen = 16; // 128 bits
  const int hashLen = 32; // 256 bits

  // Generate random salt
  unsigned char salt[saltLen];
  if (RAND_bytes(salt, saltLen) != 1) {
    throw std::runtime_error("Failed to generate random salt");
  }

  // Derive key using PBKDF2-HMAC-SHA256
  unsigned char hash[hashLen];
  if (PKCS5_PBKDF2_HMAC(
        password.c_str(), password.length(),
        salt, saltLen,
        iterations,
        EVP_sha256(),
        hashLen,
        hash) != 1) {
    throw std::runtime_error("PBKDF2 key derivation failed");
  }

  // Encode salt and hash as base64
  std::string saltB64 = base64Encode(salt, saltLen);
  std::string hashB64 = base64Encode(hash, hashLen);

  // Format: pbkdf2_sha256$iterations$salt$hash
  // This format allows for future algorithm upgrades
  std::ostringstream oss;
  oss << "pbkdf2_sha256$" << iterations << "$" << saltB64 << "$" << hashB64;

  return oss.str();
}

bool User::verifyPassword(const std::string &password) const {
  if (passwordHash_.empty() || password.empty()) {
    return false;
  }

  // Check if this is a new PBKDF2 hash (starts with "pbkdf2_sha256$")
  if (passwordHash_.substr(0, 14) == "pbkdf2_sha256$") {
    // Parse the stored hash: pbkdf2_sha256$iterations$salt$hash
    std::istringstream iss(passwordHash_);
    std::string algorithm, iterStr, saltB64, hashB64;

    std::getline(iss, algorithm, '$');
    std::getline(iss, iterStr, '$');
    std::getline(iss, saltB64, '$');
    std::getline(iss, hashB64, '$');

    if (algorithm != "pbkdf2_sha256" || iterStr.empty() || saltB64.empty() || hashB64.empty()) {
      return false; // Malformed hash
    }

    int iterations = std::stoi(iterStr);
    std::vector<unsigned char> salt = base64Decode(saltB64);
    std::vector<unsigned char> expectedHash = base64Decode(hashB64);

    // Derive key from provided password with the same salt and iterations
    const int hashLen = 32; // 256 bits
    unsigned char derivedHash[hashLen];

    if (PKCS5_PBKDF2_HMAC(
          password.c_str(), password.length(),
          salt.data(), salt.size(),
          iterations,
          EVP_sha256(),
          hashLen,
          derivedHash) != 1) {
      return false; // Key derivation failed
    }

    // Constant-time comparison to prevent timing attacks
    if (expectedHash.size() != hashLen) {
      return false;
    }

    int result = 0;
    for (size_t i = 0; i < hashLen; i++) {
      result |= derivedHash[i] ^ expectedHash[i];
    }

    return result == 0;
  }

  // Hash format not recognised as PBKDF2 — reject (legacy DJB2 removed per CLAUDE.md)
  return false;
}

void User::setPassword(const std::string &password) {
  passwordHash_ = hashPassword(password);
}

// Rollen-Hilfsfunktionen
std::string User::getRoleString() const {
  if (!roleName_.empty()) {
    return roleName_;
  }
  return roleToString(role_);
}

std::string User::roleToString(Role role) {
  switch (role) {
  case Role::ADMIN:
    return "Administrator";
  case Role::OPERATOR:
    return "Operator";
  case Role::VIEWER:
    return "Betrachter";
  case Role::CUSTOM:
    return "Benutzerdefiniert";
  default:
    return "Unbekannt";
  }
}

User::Role User::stringToRole(const std::string &str) {
  static const std::unordered_map<std::string, Role> roleMap = {
      {"Administrator", Role::ADMIN}, {"ADMIN", Role::ADMIN},
      {"Admin", Role::ADMIN},         {"Operator", Role::OPERATOR},
      {"OPERATOR", Role::OPERATOR},   {"Betrachter", Role::VIEWER},
      {"VIEWER", Role::VIEWER},       {"Viewer", Role::VIEWER},
  };

  auto it = roleMap.find(str);
  if (it != roleMap.end()) {
    return it->second;
  }

  return Role::CUSTOM;
}

void User::setRoleName(const std::string &roleName) {
  roleName_ = roleName;
  role_ = stringToRole(roleName);
}

} // namespace core
} // namespace opensylab
