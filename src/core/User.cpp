#include "core/User.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace opensylab {
namespace core {

// Konstruktoren
User::User()
    : id_(0), username_(""), passwordHash_(""), role_(Role::VIEWER),
      active_(true), lastLogin_(0), createdDate_(std::time(nullptr)),
      fullName_(""), email_("") {}

User::User(const std::string &username, const std::string &passwordHash,
           Role role)
    : id_(0), username_(username), passwordHash_(passwordHash), role_(role),
      active_(true), lastLogin_(0), createdDate_(std::time(nullptr)),
      fullName_(""), email_("") {}

// Einfache Hash-Funktion (NICHT kryptographisch sicher!)
// In einer Produktionsumgebung sollte bcrypt oder argon2 verwendet werden
std::string User::hashPassword(const std::string &password) {
  if (password.empty()) {
    return "";
  }

  // Einfacher Hash basierend auf DJB2-Algorithmus mit Salt
  const std::string salt = "OpenSylab_v0.2_Salt";
  std::string salted = salt + password + salt;

  unsigned long hash1 = 5381;
  unsigned long hash2 = 52711;

  for (char c : salted) {
    hash1 = ((hash1 << 5) + hash1) ^ static_cast<unsigned char>(c);
    hash2 = ((hash2 << 5) + hash2) + static_cast<unsigned char>(c);
  }

  // Kombinieren und als Hex-String ausgeben
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  oss << std::setw(16) << hash1;
  oss << std::setw(16) << hash2;

  return oss.str();
}

bool User::verifyPassword(const std::string &password) const {
  return hashPassword(password) == passwordHash_;
}

void User::setPassword(const std::string &password) {
  passwordHash_ = hashPassword(password);
}

// Rollen-Hilfsfunktionen
std::string User::getRoleString() const { return roleToString(role_); }

std::string User::roleToString(Role role) {
  switch (role) {
  case Role::ADMIN:
    return "Administrator";
  case Role::OPERATOR:
    return "Operator";
  case Role::VIEWER:
    return "Betrachter";
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

  throw std::invalid_argument("Ungültiger Rollen-String: " + str);
}

} // namespace core
} // namespace opensylab
