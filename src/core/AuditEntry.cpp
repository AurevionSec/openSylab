#include "core/AuditEntry.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace opensylab {
namespace core {

// Konstruktoren
AuditEntry::AuditEntry()
    : id_(0), action_(ActionType::CREATE), entity_(EntityType::SYSTEM),
      entityId_(""), user_(""), timestamp_(std::time(nullptr)), details_("") {}

AuditEntry::AuditEntry(ActionType action, EntityType entity,
                       const std::string &entityId, const std::string &user,
                       const std::string &details)
    : id_(0), action_(action), entity_(entity), entityId_(entityId),
      user_(user), timestamp_(std::time(nullptr)), details_(details) {}

// Action-Hilfsfunktionen
std::string AuditEntry::getActionString() const {
  return actionToString(action_);
}

std::string AuditEntry::actionToString(ActionType action) {
  switch (action) {
  case ActionType::CREATE:
    return "CREATE";
  case ActionType::UPDATE:
    return "UPDATE";
  case ActionType::DELETE:
    return "DELETE";
  case ActionType::LOGIN:
    return "LOGIN";
  case ActionType::LOGIN_FAILED:
    return "LOGIN_FAILED";
  case ActionType::LOGOUT:
    return "LOGOUT";
  case ActionType::VALIDATE:
    return "VALIDATE";
  case ActionType::EXPORT:
    return "EXPORT";
  case ActionType::ACCESS:
    return "ACCESS";
  default:
    return "UNKNOWN";
  }
}

AuditEntry::ActionType AuditEntry::stringToAction(const std::string &str) {
  static const std::unordered_map<std::string, ActionType> actionMap = {
      {"Erstellt", ActionType::CREATE},     {"CREATE", ActionType::CREATE},
      {"Aktualisiert", ActionType::UPDATE}, {"UPDATE", ActionType::UPDATE},
      {"Gelöscht", ActionType::DELETE},     {"DELETE", ActionType::DELETE},
      {"Angemeldet", ActionType::LOGIN},           {"LOGIN", ActionType::LOGIN},
      {"Anmeldeversuch fehlgeschlagen", ActionType::LOGIN_FAILED},
      {"LOGIN_FAILED", ActionType::LOGIN_FAILED},
      {"Abgemeldet", ActionType::LOGOUT},           {"LOGOUT", ActionType::LOGOUT},
      {"Validiert", ActionType::VALIDATE},  {"VALIDATE", ActionType::VALIDATE},
      {"Exportiert", ActionType::EXPORT},   {"EXPORT", ActionType::EXPORT},
      {"Zugriff", ActionType::ACCESS},      {"ACCESS", ActionType::ACCESS},
  };

  auto it = actionMap.find(str);
  if (it != actionMap.end()) {
    return it->second;
  }

  throw std::invalid_argument("Ungültiger Action-String: " + str);
}

// Entity-Hilfsfunktionen
std::string AuditEntry::getEntityString() const {
  return entityToString(entity_);
}

std::string AuditEntry::entityToString(EntityType entity) {
  switch (entity) {
  case EntityType::SAMPLE:
    return "SAMPLE";
  case EntityType::ORDER:
    return "ORDER";
  case EntityType::RESULT:
    return "RESULT";
  case EntityType::USER:
    return "USER";
  case EntityType::ROLE:
    return "ROLE";
  case EntityType::SYSTEM:
    return "SYSTEM";
  default:
    return "UNKNOWN";
  }
}

AuditEntry::EntityType AuditEntry::stringToEntity(const std::string &str) {
  static const std::unordered_map<std::string, EntityType> entityMap = {
      {"Probe", EntityType::SAMPLE},    {"SAMPLE", EntityType::SAMPLE},
      {"Auftrag", EntityType::ORDER},   {"ORDER", EntityType::ORDER},
      {"Ergebnis", EntityType::RESULT}, {"RESULT", EntityType::RESULT},
      {"Benutzer", EntityType::USER},   {"USER", EntityType::USER},
      {"Rolle", EntityType::ROLE},      {"ROLE", EntityType::ROLE},
      {"System", EntityType::SYSTEM},   {"SYSTEM", EntityType::SYSTEM},
  };

  auto it = entityMap.find(str);
  if (it != entityMap.end()) {
    return it->second;
  }

  throw std::invalid_argument("Ungültiger Entity-String: " + str);
}

std::string AuditEntry::getTimestampString() const {
  std::tm tm_buf{};
  localtime_r(&timestamp_, &tm_buf);
  std::ostringstream oss;
  oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

} // namespace core
} // namespace opensylab
