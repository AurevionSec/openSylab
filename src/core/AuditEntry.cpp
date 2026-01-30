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
    return "Erstellt";
  case ActionType::UPDATE:
    return "Aktualisiert";
  case ActionType::DELETE:
    return "Gelöscht";
  case ActionType::LOGIN:
    return "Angemeldet";
  case ActionType::LOGOUT:
    return "Abgemeldet";
  case ActionType::VALIDATE:
    return "Validiert";
  case ActionType::EXPORT:
    return "Exportiert";
  default:
    return "Unbekannt";
  }
}

AuditEntry::ActionType AuditEntry::stringToAction(const std::string &str) {
  static const std::unordered_map<std::string, ActionType> actionMap = {
      {"Erstellt", ActionType::CREATE},     {"CREATE", ActionType::CREATE},
      {"Aktualisiert", ActionType::UPDATE}, {"UPDATE", ActionType::UPDATE},
      {"Gelöscht", ActionType::DELETE},     {"DELETE", ActionType::DELETE},
      {"Angemeldet", ActionType::LOGIN},    {"LOGIN", ActionType::LOGIN},
      {"Abgemeldet", ActionType::LOGOUT},   {"LOGOUT", ActionType::LOGOUT},
      {"Validiert", ActionType::VALIDATE},  {"VALIDATE", ActionType::VALIDATE},
      {"Exportiert", ActionType::EXPORT},   {"EXPORT", ActionType::EXPORT},
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
    return "Probe";
  case EntityType::ORDER:
    return "Auftrag";
  case EntityType::RESULT:
    return "Ergebnis";
  case EntityType::USER:
    return "Benutzer";
  case EntityType::ROLE:
    return "Rolle";
  case EntityType::SYSTEM:
    return "System";
  default:
    return "Unbekannt";
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
  std::tm *tm = std::localtime(&timestamp_);
  std::ostringstream oss;
  oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

} // namespace core
} // namespace opensylab
