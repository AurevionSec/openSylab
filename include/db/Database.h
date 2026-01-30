#ifndef OPENSYLAB_DATABASE_H
#define OPENSYLAB_DATABASE_H

#include "core/AuditEntry.h"
#include "core/Order.h"
#include "core/Sample.h"
#include "core/TestResult.h"
#include "core/User.h"
#include <ctime>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Forward declaration für SQLite
struct sqlite3;

namespace opensylab {
namespace db {

/**
 * @brief Datenbank-Zugriffsschicht für OpenSylab
 *
 * Diese Klasse kapselt alle Datenbankoperationen und bietet
 * eine typsichere Schnittstelle für CRUD-Operationen.
 */
class Database {
public:
  struct SampleFilter {
    std::string query;
    std::string status;
    std::optional<std::time_t> fromDate;
    std::optional<std::time_t> toDate;
    bool excludeArchived = false;
  };

  struct OrderFilter {
    std::string status;
    std::string sampleId;
    std::string priority;
  };

  struct AuditLogFilter {
    std::optional<std::string> user;
    std::optional<core::AuditEntry::ActionType> action;
    std::optional<core::AuditEntry::EntityType> entity;
    std::optional<std::string> entityId;
    std::optional<std::time_t> fromTime;
    std::optional<std::time_t> toTime;
    int limit = 100;
  };

  struct DiagnosticsFilter {
    std::optional<core::AuditEntry::EntityType> component;
    std::optional<std::time_t> fromTime;
    std::optional<std::time_t> toTime;
    int limit = 200;
  };

  enum class AuthMethod { LOCAL, LDAP };

  struct AuthResult {
    std::unique_ptr<core::User> user;
    AuthMethod method = AuthMethod::LOCAL;
    bool requiresMfa = false;
    std::string mfaSecret;
    std::string message;
  };

  struct StatusCount {
    std::string status;
    int count = 0;
  };

  struct EntityStats {
    int total = 0;
    std::vector<StatusCount> byStatus;
  };

  struct StatsFilter {
    std::optional<std::time_t> fromDate;
    std::optional<std::time_t> toDate;
    std::optional<std::string> status;
  };

  struct HealthStatus {
    bool dbOpen = false;
    bool schemaOk = false;
    std::vector<std::string> missingTables;
  };

  struct SessionInfo {
    int id = 0;
    int userId = 0;
    std::string username;
    std::string method;
    std::time_t loginTs = 0;
    std::optional<std::time_t> logoutTs;
    std::string details;
  };
  /**
   * @brief Konstruktor
   * @param dbPath Pfad zur SQLite-Datenbankdatei
   */
  explicit Database(const std::string &dbPath);

  /**
   * @brief Destruktor - schließt die Datenbankverbindung
   */
  ~Database();

  // Nicht kopierbar oder bewegbar (hat SQLite-Handle)
  Database(const Database &) = delete;
  Database &operator=(const Database &) = delete;
  Database(Database &&) = delete;
  Database &operator=(Database &&) = delete;

  // Datenbankoperationen
  [[nodiscard]] bool open();
  bool close(); // Kein nodiscard - wird im Destruktor ohne Check aufgerufen
  [[nodiscard]] bool isOpen() const { return isOpen_; }

  /**
   * @brief Initialisiert das Datenbankschema
   * @return true bei Erfolg
   */
  [[nodiscard]] bool initializeSchema();

  // Systemstatus
  [[nodiscard]] HealthStatus getHealthStatus();

  // Sample-Operationen (CRUD)
  [[nodiscard]] bool createSample(const core::Sample &sample,
                                  const std::string &actor = "");
  [[nodiscard]] std::unique_ptr<core::Sample> getSample(int id);
  [[nodiscard]] std::unique_ptr<core::Sample>
  getSampleByBarcode(const std::string &barcode);
  [[nodiscard]] std::vector<std::unique_ptr<core::Sample>> getAllSamples();
  [[nodiscard]] std::vector<std::unique_ptr<core::Sample>>
  getSamplesByFilter(const SampleFilter &filter);
  [[nodiscard]] bool updateSample(const core::Sample &sample,
                                  const std::string &actor = "");
  [[nodiscard]] bool deleteSample(int id, const std::string &actor = "");
  [[nodiscard]] bool exportSamplesToCsv(const std::string &filePath);

  // Order-Operationen (CRUD)
  [[nodiscard]] bool createOrder(const core::Order &order,
                                 const std::string &actor = "");
  [[nodiscard]] std::unique_ptr<core::Order> getOrder(int id);
  [[nodiscard]] std::unique_ptr<core::Order>
  getOrderByOrderId(const std::string &orderId);
  [[nodiscard]] std::vector<std::unique_ptr<core::Order>>
  getOrdersBySampleId(const std::string &sampleId);
  [[nodiscard]] std::vector<std::unique_ptr<core::Order>> getAllOrders();
  [[nodiscard]] std::vector<std::unique_ptr<core::Order>>
  getOrdersByFilter(const OrderFilter &filter);
  [[nodiscard]] bool updateOrder(const core::Order &order,
                                 const std::string &actor = "");
  [[nodiscard]] bool deleteOrder(int id, const std::string &actor = "");

  // TestResult-Operationen (CRUD)
  [[nodiscard]] bool createTestResult(const core::TestResult &result,
                                      const std::string &actor = "");
  [[nodiscard]] std::unique_ptr<core::TestResult> getTestResult(int id);
  [[nodiscard]] std::unique_ptr<core::TestResult>
  getTestResultByResultId(const std::string &resultId);
  [[nodiscard]] std::vector<std::unique_ptr<core::TestResult>>
  getTestResultsByOrderId(int orderId);
  [[nodiscard]] std::vector<std::unique_ptr<core::TestResult>>
  getAllTestResults();
  [[nodiscard]] bool updateTestResult(const core::TestResult &result,
                                      const std::string &actor = "");
  [[nodiscard]] bool updateTestResultWithAudit(const core::TestResult &result,
                                               const std::string &user);
  [[nodiscard]] bool exportValidatedResultsToCsv(
      const std::string &filePath, const std::string &user,
      std::optional<int> orderId = std::nullopt);
  [[nodiscard]] bool validateTestResult(const std::string &resultId,
                                        const std::string &user);
  [[nodiscard]] bool deleteTestResult(int id, const std::string &actor = "");

  // Statistik-Operationen
  [[nodiscard]] EntityStats
  getSampleStats(const StatsFilter &filter = StatsFilter{});
  [[nodiscard]] EntityStats getOrderStats(const StatsFilter &filter = StatsFilter{});
  [[nodiscard]] EntityStats
  getResultStats(const StatsFilter &filter = StatsFilter{});
  [[nodiscard]] bool exportStatsReportToCsv(
      const std::string &filePath, const StatsFilter &sampleFilter,
      const StatsFilter &orderFilter, const StatsFilter &resultFilter,
      const std::string &actor);

  // Audit-Operationen
  [[nodiscard]] bool logAudit(const core::AuditEntry &entry);
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLog(int limit = 100);
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLogByEntity(core::AuditEntry::EntityType entity,
                      const std::string &entityId);
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLogFiltered(const AuditLogFilter &filter);
  [[nodiscard]] bool exportAuditLogToCsv(const std::string &filePath,
                                         const AuditLogFilter &filter,
                                         const std::string &actor,
                                         int &exportedCount);
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getDiagnosticsLogs(const DiagnosticsFilter &filter);
  [[nodiscard]] bool exportDiagnosticsLogsToCsv(const std::string &filePath,
                                                const DiagnosticsFilter &filter,
                                                const std::string &actor,
                                                int &exportedCount);

  // API Keys
  [[nodiscard]] bool upsertApiKey(const std::string &key, bool active = true);
  [[nodiscard]] bool isApiKeyValid(const std::string &key);

  // Retention
  [[nodiscard]] int getRetentionDays();
  [[nodiscard]] bool setRetentionDays(int days);
  [[nodiscard]] bool applyAuditRetention(const std::string &actor,
                                         int &purgedCount);

  // Audit-Hilfsmethoden
  void logSampleAction(core::AuditEntry::ActionType action,
                       const std::string &sampleId, const std::string &user,
                       const std::string &details = "");
  void logOrderAction(core::AuditEntry::ActionType action,
                      const std::string &orderId, const std::string &user,
                      const std::string &details = "");
  void logResultAction(core::AuditEntry::ActionType action,
                       const std::string &resultId, const std::string &user,
                       const std::string &details = "");
  void logUserAction(core::AuditEntry::ActionType action,
                     const std::string &username, const std::string &user,
                     const std::string &details = "");
  void logRoleAction(core::AuditEntry::ActionType action,
                     const std::string &roleName, const std::string &user,
                     const std::string &details = "");
  [[nodiscard]] bool logSupportAccess(core::AuditEntry::EntityType entity,
                                      const std::string &entityId,
                                      const std::string &user,
                                      const std::string &details = "");
  [[nodiscard]] bool
  logResultRetryImport(const std::vector<std::string> &resultIds,
                       const std::string &user,
                       const std::string &filePath);

  // User-Operationen (CRUD)
  [[nodiscard]] bool createUser(const core::User &user,
                                const std::string &actor = "");
  [[nodiscard]] std::unique_ptr<core::User> getUser(int id);
  [[nodiscard]] std::unique_ptr<core::User>
  getUserByUsername(const std::string &username);
  [[nodiscard]] std::vector<std::unique_ptr<core::User>> getAllUsers();
  [[nodiscard]] bool updateUser(const core::User &user,
                                const std::string &actor = "");
  [[nodiscard]] bool deleteUser(int id, const std::string &actor = "");

  // Rollen & Berechtigungen
  [[nodiscard]] bool createRole(const std::string &name,
                                const std::vector<std::string> &permissions,
                                const std::string &actor = "");
  [[nodiscard]] bool updateRole(const std::string &name,
                                const std::vector<std::string> &permissions,
                                const std::string &actor = "");
  [[nodiscard]] std::vector<std::string>
  getRolePermissions(const std::string &name);
  [[nodiscard]] std::vector<std::string> getAllRoles();
  [[nodiscard]] bool assignUserRole(int userId, const std::string &roleName,
                                    const std::string &actor = "");

  // Auth-Konfiguration, LDAP und MFA
  [[nodiscard]] bool setAuthConfig(const std::string &key,
                                   const std::string &value);
  [[nodiscard]] std::optional<std::string>
  getAuthConfig(const std::string &key);
  [[nodiscard]] bool setLdapEnabled(bool enabled);
  [[nodiscard]] bool isLdapEnabled();
  [[nodiscard]] bool upsertLdapUser(const std::string &username,
                                    const std::string &passwordHash,
                                    bool active, bool mfaRequired,
                                    const std::string &mfaSecret);
  [[nodiscard]] bool setUserMfaRequirement(const std::string &username,
                                           bool required,
                                           const std::string &mfaSecret);
  [[nodiscard]] bool setRoleMfaRequirement(const std::string &roleName,
                                           bool required);
  [[nodiscard]] bool isMfaRequiredForUser(const std::string &username,
                                          const std::string &roleName);
  [[nodiscard]] bool verifyUserMfa(const std::string &username,
                                   const std::string &code);

  // Sitzungsverfolgung
  [[nodiscard]] bool startSession(int userId, const std::string &username,
                                  AuthMethod method,
                                  const std::string &details = "");
  [[nodiscard]] bool endSession(int userId, const std::string &username,
                                const std::string &reason = "");
  [[nodiscard]] std::optional<int> getActiveSessionId(int userId);
  [[nodiscard]] int getActiveSessionCount(int userId);
  [[nodiscard]] int getSessionCount(int userId);
  [[nodiscard]] bool hasActiveSession(int userId);
  [[nodiscard]] std::optional<SessionInfo> getSessionById(int sessionId);
  [[nodiscard]] std::optional<SessionInfo> getLatestSessionForUser(int userId);

  // Authentifizierung
  [[nodiscard]] AuthResult authenticatePrimary(const std::string &username,
                                               const std::string &password);
  [[nodiscard]] std::unique_ptr<core::User>
  authenticateUser(const std::string &username, const std::string &password,
                   const std::optional<std::string> &mfaCode = std::nullopt);

  // Fehlerbehandlung
  const std::string &getLastError() const { return lastError_; }
  bool hasError() const { return !lastError_.empty(); }
  void clearError() { lastError_.clear(); }

private:
  std::string dbPath_;
  sqlite3 *db_ = nullptr;
  bool isOpen_ = false;
  std::string lastError_;
  struct PendingAuth {
    bool active = false;
    std::string username;
    AuthMethod method = AuthMethod::LOCAL;
    std::string mfaSecret;
  } pendingAuth_;

  // Hilfsfunktionen
  void setError(const std::string &error);
  [[nodiscard]] bool updateTestResultCore(const core::TestResult &result);
};

} // namespace db
} // namespace opensylab

#endif // OPENSYLAB_DATABASE_H
