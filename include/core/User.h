#ifndef OPENSYLAB_USER_H
#define OPENSYLAB_USER_H

#include <ctime>
#include <string>

namespace opensylab {
namespace core {

/**
 * @brief Klasse repräsentiert einen Benutzer im LIMS
 *
 * Benutzer haben verschiedene Rollen mit unterschiedlichen Berechtigungen.
 * Passwörter werden gehasht gespeichert.
 */
class User {
public:
  /**
   * @brief Benutzerrolle
   */
  enum class Role {
    ADMIN,    // Vollzugriff, Benutzerverwaltung
    OPERATOR, // Proben, Aufträge, Ergebnisse bearbeiten
    VIEWER,   // Nur Lesezugriff
    CUSTOM    // Benutzerdefinierte Rolle
  };

  // Konstruktoren
  User();
  User(const std::string &username, const std::string &passwordHash,
       Role role = Role::OPERATOR);

  // Destruktor
  ~User() = default;

  // Getter
  int getId() const { return id_; }
  const std::string &getUsername() const { return username_; }
  const std::string &getPasswordHash() const { return passwordHash_; }
  Role getRole() const { return role_; }
  const std::string &getRoleName() const { return roleName_; }
  bool isActive() const { return active_; }
  bool mustChangePassword() const { return mustChangePassword_; }
  std::time_t getLastLogin() const { return lastLogin_; }
  std::time_t getCreatedDate() const { return createdDate_; }
  const std::string &getFullName() const { return fullName_; }
  const std::string &getEmail() const { return email_; }

  // Setter
  void setId(int id) { id_ = id; }
  void setUsername(const std::string &username) { username_ = username; }
  void setPasswordHash(const std::string &hash) { passwordHash_ = hash; }
  void setRole(Role role) {
    role_ = role;
    roleName_ = roleToString(role);
  }
  void setRoleName(const std::string &roleName);
  void setActive(bool active) { active_ = active; }
  void setMustChangePassword(bool v) { mustChangePassword_ = v; }
  void setLastLogin(std::time_t lastLogin) { lastLogin_ = lastLogin; }
  void setCreatedDate(std::time_t created) { createdDate_ = created; }
  void setFullName(const std::string &name) { fullName_ = name; }
  void setEmail(const std::string &email) { email_ = email; }

  // Passwort-Funktionen
  static std::string hashPassword(const std::string &password);
  bool verifyPassword(const std::string &password) const;
  void setPassword(const std::string &password);

  // Konvertierungsfunktionen
  std::string getRoleString() const;
  static std::string roleToString(Role role);
  static Role stringToRole(const std::string &str);

  // Login-Timestamp aktualisieren
  void updateLastLogin() { lastLogin_ = std::time(nullptr); }

private:
  int id_;
  std::string username_;
  std::string passwordHash_;
  Role role_;
  std::string roleName_;
  bool active_;
  bool mustChangePassword_ = false;
  std::time_t lastLogin_;
  std::time_t createdDate_;
  std::string fullName_;
  std::string email_;
};

} // namespace core
} // namespace opensylab

#endif // OPENSYLAB_USER_H
