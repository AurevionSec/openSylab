#ifndef OPENSYLAB_AUDITENTRY_H
#define OPENSYLAB_AUDITENTRY_H

#include <ctime>
#include <string>

namespace opensylab {
namespace core {

/**
 * @brief Klasse repräsentiert einen Audit-Log-Eintrag
 *
 * Speichert Informationen über Aktionen im System zur Nachverfolgbarkeit
 * und Compliance (GxP-Anforderungen).
 */
class AuditEntry {
public:
  /**
   * @brief Art der Aktion
   */
  enum class ActionType {
    CREATE,  // Neuer Datensatz erstellt
    UPDATE,  // Datensatz aktualisiert
    DELETE,  // Datensatz gelöscht
    LOGIN,   // Benutzer angemeldet
    LOGOUT,  // Benutzer abgemeldet
    VALIDATE, // Ergebnis validiert
    EXPORT    // Daten exportiert
  };

  /**
   * @brief Betroffene Entität
   */
  enum class EntityType { SAMPLE, ORDER, RESULT, USER, ROLE, SYSTEM };

  // Konstruktoren
  AuditEntry();
  AuditEntry(ActionType action, EntityType entity, const std::string &entityId,
             const std::string &user, const std::string &details = "");

  // Destruktor
  ~AuditEntry() = default;

  // Getter
  int getId() const { return id_; }
  ActionType getAction() const { return action_; }
  EntityType getEntity() const { return entity_; }
  const std::string &getEntityId() const { return entityId_; }
  const std::string &getUser() const { return user_; }
  std::time_t getTimestamp() const { return timestamp_; }
  const std::string &getDetails() const { return details_; }

  // Setter
  void setId(int id) { id_ = id; }
  void setAction(ActionType action) { action_ = action; }
  void setEntity(EntityType entity) { entity_ = entity; }
  void setEntityId(const std::string &id) { entityId_ = id; }
  void setUser(const std::string &user) { user_ = user; }
  void setTimestamp(std::time_t ts) { timestamp_ = ts; }
  void setDetails(const std::string &details) { details_ = details; }

  // Konvertierungsfunktionen
  std::string getActionString() const;
  static std::string actionToString(ActionType action);
  static ActionType stringToAction(const std::string &str);

  std::string getEntityString() const;
  static std::string entityToString(EntityType entity);
  static EntityType stringToEntity(const std::string &str);

  // Formatierte Ausgabe
  std::string getTimestampString() const;

private:
  int id_;
  ActionType action_;
  EntityType entity_;
  std::string entityId_;
  std::string user_;
  std::time_t timestamp_;
  std::string details_;
};

} // namespace core
} // namespace opensylab

#endif // OPENSYLAB_AUDITENTRY_H
