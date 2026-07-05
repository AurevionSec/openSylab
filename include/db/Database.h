#ifndef OPENSYLAB_DATABASE_H
#define OPENSYLAB_DATABASE_H

#include "db/IDatabase.h"
#include <mutex>

// Forward declaration für SQLite
struct sqlite3;

namespace opensylab {
namespace db {

/**
 * @brief SQLite-backed implementation of IDatabase for OpenSylab LIMS.
 *
 * This class is the canonical production database backend. It implements
 * all IDatabase methods using SQLite via prepared statements.
 *
 * Consumers should depend on IDatabase, not on this concrete class.
 */
class Database : public IDatabase {
public:
  /**
   * @brief Konstruktor
   * @param dbPath Pfad zur SQLite-Datenbankdatei
   */
  explicit Database(const std::string &dbPath);

  /**
   * @brief Destruktor - schließt die Datenbankverbindung
   */
  ~Database() override;

  // Nicht kopierbar oder bewegbar (hat SQLite-Handle)
  Database(const Database &) = delete;
  Database &operator=(const Database &) = delete;
  Database(Database &&) = delete;
  Database &operator=(Database &&) = delete;

  // Datenbankoperationen
  [[nodiscard]] bool open() override;
  bool close() override; // Kein nodiscard - wird im Destruktor ohne Check aufgerufen
  [[nodiscard]] bool isOpen() const override { return isOpen_; }

  /**
   * @brief Initialisiert das Datenbankschema
   * @return true bei Erfolg
   */
  [[nodiscard]] bool initializeSchema() override;

  // Systemstatus
  [[nodiscard]] HealthStatus getHealthStatus() override;

  // Sample-Operationen (CRUD)
  [[nodiscard]] bool createSample(const core::Sample &sample,
                                  const std::string &actor = "") override;
  [[nodiscard]] std::unique_ptr<core::Sample> getSample(int id) override;
  [[nodiscard]] std::unique_ptr<core::Sample>
  getSampleByBarcode(const std::string &barcode) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::Sample>>
  getAllSamples() override;
  [[nodiscard]] std::vector<std::unique_ptr<core::Sample>>
  getSamplesByFilter(const SampleFilter &filter) override;
  [[nodiscard]] int getSamplesCount(const SampleFilter &filter) override;
  [[nodiscard]] bool updateSample(const core::Sample &sample,
                                  const std::string &actor = "") override;
  [[nodiscard]] bool deleteSample(int id,
                                  const std::string &actor = "") override;
  [[nodiscard]] bool exportSamplesToCsv(const std::string &filePath) override;

  // Order-Operationen (CRUD)
  [[nodiscard]] bool createOrder(const core::Order &order,
                                 const std::string &actor = "") override;
  [[nodiscard]] std::unique_ptr<core::Order> getOrder(int id) override;
  [[nodiscard]] std::unique_ptr<core::Order>
  getOrderByOrderId(const std::string &orderId) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::Order>>
  getOrdersBySampleId(const std::string &sampleId) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::Order>>
  getAllOrders() override;
  [[nodiscard]] std::vector<std::unique_ptr<core::Order>>
  getOrdersByFilter(const OrderFilter &filter) override;
  [[nodiscard]] int getOrdersCount(const OrderFilter &filter) override;
  [[nodiscard]] bool updateOrder(const core::Order &order,
                                 const std::string &actor = "") override;
  [[nodiscard]] bool deleteOrder(int id,
                                 const std::string &actor = "") override;

  // TestResult-Operationen (CRUD)
  [[nodiscard]] bool createTestResult(const core::TestResult &result,
                                      const std::string &actor = "") override;
  [[nodiscard]] std::unique_ptr<core::TestResult> getTestResult(int id) override;
  [[nodiscard]] std::unique_ptr<core::TestResult>
  getTestResultByResultId(const std::string &resultId) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::TestResult>>
  getTestResultsByOrderId(int orderId,
                          std::optional<int> limit = std::nullopt,
                          std::optional<int> offset = std::nullopt) override;
  [[nodiscard]] int
  getTestResultsCount(std::optional<int> orderIdFilter = std::nullopt) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::TestResult>>
  getAllTestResults(std::optional<int> limit = std::nullopt,
                   std::optional<int> offset = std::nullopt) override;

  [[nodiscard]] BatchInsertResult
  createSamplesBatch(const std::vector<core::Sample> &samples,
                     const std::string &actor = "") override;

  [[nodiscard]] BatchInsertResult
  createTestResultsBatch(const std::vector<core::TestResult> &results,
                         const std::string &actor = "") override;
  [[nodiscard]] bool updateTestResult(const core::TestResult &result,
                                      const std::string &actor = "") override;
  [[nodiscard]] bool
  updateTestResultWithAudit(const core::TestResult &result,
                            const std::string &user) override;
  [[nodiscard]] bool exportValidatedResultsToCsv(
      const std::string &filePath, const std::string &user,
      std::optional<int> orderId = std::nullopt) override;
  [[nodiscard]] bool validateTestResult(const std::string &resultId,
                                        const std::string &user) override;
  [[nodiscard]] bool deleteTestResult(int id,
                                      const std::string &actor = "") override;

  // Statistik-Operationen
  [[nodiscard]] EntityStats
  getSampleStats(const StatsFilter &filter = StatsFilter{}) override;
  [[nodiscard]] EntityStats
  getOrderStats(const StatsFilter &filter = StatsFilter{}) override;
  [[nodiscard]] EntityStats
  getResultStats(const StatsFilter &filter = StatsFilter{}) override;
  [[nodiscard]] std::vector<StatusCount> getOrderPriorityStats() override;
  [[nodiscard]] int getCriticalResultCount() override;
  [[nodiscard]] bool exportStatsReportToCsv(
      const std::string &filePath, const StatsFilter &sampleFilter,
      const StatsFilter &orderFilter, const StatsFilter &resultFilter,
      const std::string &actor) override;

  // Audit-Operationen
  [[nodiscard]] bool logAudit(const core::AuditEntry &entry) override;
  [[nodiscard]] std::string getLastAuditHash() override;
  [[nodiscard]] bool verifyAuditChain(std::string &firstBrokenAt) override;
  void setAuditHmacKey(const std::string &key) override { auditHmacKey_ = key; }
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLog(int limit = 100) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLogByEntity(core::AuditEntry::EntityType entity,
                      const std::string &entityId) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLogFiltered(const AuditLogFilter &filter) override;
  [[nodiscard]] bool exportAuditLogToCsv(const std::string &filePath,
                                         const AuditLogFilter &filter,
                                         const std::string &actor,
                                         int &exportedCount) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getDiagnosticsLogs(const DiagnosticsFilter &filter) override;
  [[nodiscard]] bool
  exportDiagnosticsLogsToCsv(const std::string &filePath,
                              const DiagnosticsFilter &filter,
                              const std::string &actor,
                              int &exportedCount) override;

  // API Keys
  [[nodiscard]] bool upsertApiKey(const std::string &key, bool active = true,
                                  const std::string &role = "OPERATOR",
                                  const std::string &actor = "") override;
  [[nodiscard]] std::optional<std::string>
  isApiKeyValid(const std::string &key) override;
  bool persistBlacklistedToken(const std::string &token,
                               std::time_t expiresAt) override;
  [[nodiscard]] std::vector<std::pair<std::string, std::time_t>>
  loadActiveBlacklistedTokens() override;
  void pruneExpiredBlacklistedTokens() override;

  // Retention
  [[nodiscard]] int getRetentionDays() override;
  [[nodiscard]] bool setRetentionDays(int days) override;
  [[nodiscard]] bool applyAuditRetention(const std::string &actor,
                                         int &purgedCount) override;

  // Audit-Hilfsmethoden
  void logSampleAction(core::AuditEntry::ActionType action,
                       const std::string &sampleId, const std::string &user,
                       const std::string &details = "") override;
  void logOrderAction(core::AuditEntry::ActionType action,
                      const std::string &orderId, const std::string &user,
                      const std::string &details = "") override;
  void logResultAction(core::AuditEntry::ActionType action,
                       const std::string &resultId, const std::string &user,
                       const std::string &details = "") override;
  void logUserAction(core::AuditEntry::ActionType action,
                     const std::string &username, const std::string &user,
                     const std::string &details = "") override;
  void logRoleAction(core::AuditEntry::ActionType action,
                     const std::string &roleName, const std::string &user,
                     const std::string &details = "") override;
  [[nodiscard]] bool logSupportAccess(core::AuditEntry::EntityType entity,
                                      const std::string &entityId,
                                      const std::string &user,
                                      const std::string &details = "") override;
  [[nodiscard]] bool
  logResultRetryImport(const std::vector<std::string> &resultIds,
                       const std::string &user,
                       const std::string &filePath) override;

  // User-Operationen (CRUD)
  [[nodiscard]] bool createUser(const core::User &user,
                                const std::string &actor = "") override;
  [[nodiscard]] std::unique_ptr<core::User> getUser(int id) override;
  [[nodiscard]] std::unique_ptr<core::User>
  getUserByUsername(const std::string &username) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::User>> getAllUsers() override;
  [[nodiscard]] bool updateUser(const core::User &user,
                                const std::string &actor = "") override;
  [[nodiscard]] bool deleteUser(int id, const std::string &actor = "") override;

  // Rollen & Berechtigungen
  [[nodiscard]] bool createRole(const std::string &name,
                                const std::vector<std::string> &permissions,
                                const std::string &actor = "") override;
  [[nodiscard]] bool updateRole(const std::string &name,
                                const std::vector<std::string> &permissions,
                                const std::string &actor = "") override;
  [[nodiscard]] std::vector<std::string>
  getRolePermissions(const std::string &name) override;
  [[nodiscard]] std::vector<std::string> getAllRoles() override;
  [[nodiscard]] bool assignUserRole(int userId, const std::string &roleName,
                                    const std::string &actor = "") override;

  // Auth-Konfiguration, LDAP und MFA
  [[nodiscard]] bool setAuthConfig(const std::string &key,
                                   const std::string &value) override;
  [[nodiscard]] std::optional<std::string>
  getAuthConfig(const std::string &key) override;
  [[nodiscard]] bool setLdapEnabled(bool enabled) override;
  [[nodiscard]] bool isLdapEnabled() override;
  [[nodiscard]] bool upsertLdapUser(const std::string &username,
                                    const std::string &passwordHash,
                                    bool active, bool mfaRequired,
                                    const std::string &mfaSecret) override;
  [[nodiscard]] bool
  setUserMfaRequirement(const std::string &username, bool required,
                         const std::string &mfaSecret) override;
  [[nodiscard]] bool setRoleMfaRequirement(const std::string &roleName,
                                           bool required) override;
  [[nodiscard]] bool isMfaRequiredForUser(const std::string &username,
                                          const std::string &roleName) override;
  [[nodiscard]] bool verifyUserMfa(const std::string &username,
                                   const std::string &code) override;

  // MFA Enrollment — Base32-compatible secrets for authenticator apps
  [[nodiscard]] std::string generateMfaSecret() override;
  [[nodiscard]] std::string getMfaEnrollmentUri(const std::string &username,
                                                const std::string &base32Secret) override;
  [[nodiscard]] bool setUserMfaSecret(int userId, const std::string &base32Secret,
                                      int64_t initialUsedStep = -1) override;
  [[nodiscard]] bool disableUserMfa(int userId,
                                    const std::string &actor = "") override;
  [[nodiscard]] bool verifyMfaCodeForEnrollment(const std::string &base32Secret,
                                                const std::string &code,
                                                int64_t &matchedStep) override;
  [[nodiscard]] bool verifyAndConsumeMfaCode(const std::string &username,
                                             const std::string &secret,
                                             const std::string &code) override;

  // Sitzungsverfolgung
  [[nodiscard]] bool startSession(int userId, const std::string &username,
                                  AuthMethod method,
                                  const std::string &details = "") override;
  [[nodiscard]] bool endSession(int userId, const std::string &username,
                                const std::string &reason = "") override;
  [[nodiscard]] std::optional<int> getActiveSessionId(int userId) override;
  [[nodiscard]] int getActiveSessionCount(int userId) override;
  [[nodiscard]] int getSessionCount(int userId) override;
  [[nodiscard]] bool hasActiveSession(int userId) override;
  [[nodiscard]] std::optional<SessionInfo> getSessionById(int sessionId) override;
  [[nodiscard]] std::optional<SessionInfo>
  getLatestSessionForUser(int userId) override;
  int expireStaleSessionsOlderThan(int maxLifetimeSeconds) override;

  // Authentifizierung
  [[nodiscard]] AuthResult
  authenticatePrimary(const std::string &username,
                      const std::string &password) override;
  [[nodiscard]] std::unique_ptr<core::User>
  authenticateUser(const std::string &username, const std::string &password,
                   const std::optional<std::string> &mfaCode = std::nullopt) override;

  [[nodiscard]] std::time_t getPasswordChangedAt(int userId) override;

  // Fehlerbehandlung
  std::string getLastError() const override {
    std::lock_guard<std::mutex> lg(errorMutex_);
    return lastError_;
  }
  bool hasError() const override {
    std::lock_guard<std::mutex> lg(errorMutex_);
    return !lastError_.empty();
  }
  void clearError() override {
    std::lock_guard<std::mutex> lg(errorMutex_);
    lastError_.clear();
  }

private:
  std::string dbPath_;
  sqlite3 *db_ = nullptr;
  bool isOpen_ = false;
  // Serializes ALL public Database access. The single sqlite3 connection is
  // shared across the server's per-connection threads; without this two
  // concurrent transactions would collide (nested BEGIN) and could corrupt the
  // audit hash chain. Recursive because some methods call others (e.g. a write
  // path calls logAudit).
  mutable std::recursive_mutex dbMutex_;
  mutable std::mutex errorMutex_;
  std::string lastError_;
  std::string auditHmacKey_;

  // Migration
  struct Migration {
    int version;
    std::string description;
    std::string sql;
  };

  [[nodiscard]] bool applyMigration(const Migration &migration);
  [[nodiscard]] bool runMigrations();
  [[nodiscard]] int getCurrentSchemaVersion();
  static const std::vector<Migration> &getMigrations();

  // Hilfsfunktionen
  void setError(const std::string &error);
  [[nodiscard]] bool updateTestResultCore(const core::TestResult &result);
};

} // namespace db
} // namespace opensylab

#endif // OPENSYLAB_DATABASE_H
