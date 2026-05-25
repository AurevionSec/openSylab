#pragma once

#include "db/IDatabase.h"
#include <string>

namespace opensylab {
namespace db {

/**
 * @brief PostgreSQL stub implementation of IDatabase.
 *
 * This class satisfies the IDatabase contract but every method returns a
 * failure indicator and records a "not yet implemented" message in the
 * last-error slot.  It exists so that:
 *
 *   1. The binary can be built with --db-backend=postgresql without linker
 *      errors.
 *   2. The full PostgreSQL implementation can be added in v1.1 without any
 *      interface changes.
 *
 * Every public method that returns bool returns false.
 * Every method that returns a pointer or optional returns nullptr /
 * std::nullopt.  Every method that returns a collection returns an empty one.
 * Numeric methods return 0.  String methods return "".
 *
 * DO NOT use this backend in production — it does not persist any data.
 */
class PostgreSQLDatabase : public IDatabase {
public:
  /**
   * @brief Construct with a libpq-style connection string.
   *
   * Example: "postgresql://user:pass\@localhost:5432/opensylab"
   *
   * The string is stored but not used until a full implementation lands in
   * v1.1.
   *
   * @param connectionString PostgreSQL connection string (DSN).
   */
  explicit PostgreSQLDatabase(const std::string &connectionString);
  ~PostgreSQLDatabase() override;

  // Non-copyable, non-movable (mirrors Database semantics)
  PostgreSQLDatabase(const PostgreSQLDatabase &) = delete;
  PostgreSQLDatabase &operator=(const PostgreSQLDatabase &) = delete;
  PostgreSQLDatabase(PostgreSQLDatabase &&) = delete;
  PostgreSQLDatabase &operator=(PostgreSQLDatabase &&) = delete;

  // -----------------------------------------------------------------------
  // Lifecycle
  // -----------------------------------------------------------------------
  [[nodiscard]] bool open() override;
  bool close() override;
  [[nodiscard]] bool isOpen() const override;

  // -----------------------------------------------------------------------
  // Schema
  // -----------------------------------------------------------------------
  [[nodiscard]] bool initializeSchema() override;

  // -----------------------------------------------------------------------
  // System health
  // -----------------------------------------------------------------------
  [[nodiscard]] HealthStatus getHealthStatus() override;

  // -----------------------------------------------------------------------
  // Samples
  // -----------------------------------------------------------------------
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
  [[nodiscard]] BatchInsertResult
  createSamplesBatch(const std::vector<core::Sample> &samples,
                     const std::string &actor = "") override;

  // -----------------------------------------------------------------------
  // Orders
  // -----------------------------------------------------------------------
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

  // -----------------------------------------------------------------------
  // TestResults
  // -----------------------------------------------------------------------
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
  createTestResultsBatch(const std::vector<core::TestResult> &results,
                         const std::string &actor = "") override;
  [[nodiscard]] bool updateTestResult(const core::TestResult &result,
                                      const std::string &actor = "") override;
  [[nodiscard]] bool
  updateTestResultWithAudit(const core::TestResult &result,
                            const std::string &user) override;
  [[nodiscard]] bool
  exportValidatedResultsToCsv(const std::string &filePath,
                               const std::string &user,
                               std::optional<int> orderId = std::nullopt) override;
  [[nodiscard]] bool validateTestResult(const std::string &resultId,
                                        const std::string &user) override;
  [[nodiscard]] bool deleteTestResult(int id,
                                      const std::string &actor = "") override;

  // -----------------------------------------------------------------------
  // Statistics
  // -----------------------------------------------------------------------
  [[nodiscard]] EntityStats
  getSampleStats(const StatsFilter &filter = StatsFilter{}) override;
  [[nodiscard]] EntityStats
  getOrderStats(const StatsFilter &filter = StatsFilter{}) override;
  [[nodiscard]] EntityStats
  getResultStats(const StatsFilter &filter = StatsFilter{}) override;
  [[nodiscard]] std::vector<StatusCount> getOrderPriorityStats() override;
  [[nodiscard]] int getCriticalResultCount() override;
  [[nodiscard]] bool
  exportStatsReportToCsv(const std::string &filePath,
                          const StatsFilter &sampleFilter,
                          const StatsFilter &orderFilter,
                          const StatsFilter &resultFilter,
                          const std::string &actor) override;

  // -----------------------------------------------------------------------
  // Audit log
  // -----------------------------------------------------------------------
  [[nodiscard]] bool logAudit(const core::AuditEntry &entry) override;
  [[nodiscard]] std::string getLastAuditHash() override;
  [[nodiscard]] bool verifyAuditChain(std::string &firstBrokenAt) override;
  void setAuditHmacKey(const std::string & /*key*/) override {}
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLog(int limit = 100) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLogByEntity(core::AuditEntry::EntityType entity,
                      const std::string &entityId) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLogFiltered(const AuditLogFilter &filter) override;
  [[nodiscard]] bool
  exportAuditLogToCsv(const std::string &filePath,
                      const AuditLogFilter &filter, const std::string &actor,
                      int &exportedCount) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getDiagnosticsLogs(const DiagnosticsFilter &filter) override;
  [[nodiscard]] bool
  exportDiagnosticsLogsToCsv(const std::string &filePath,
                              const DiagnosticsFilter &filter,
                              const std::string &actor,
                              int &exportedCount) override;

  // -----------------------------------------------------------------------
  // Audit helper actions
  // -----------------------------------------------------------------------
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

  // -----------------------------------------------------------------------
  // API keys
  // -----------------------------------------------------------------------
  [[nodiscard]] bool upsertApiKey(const std::string &key, bool active = true,
                                  const std::string &role = "OPERATOR") override;
  [[nodiscard]] std::optional<std::string>
  isApiKeyValid(const std::string &key) override;

  // -----------------------------------------------------------------------
  // Retention
  // -----------------------------------------------------------------------
  [[nodiscard]] int getRetentionDays() override;
  [[nodiscard]] bool setRetentionDays(int days) override;
  [[nodiscard]] bool applyAuditRetention(const std::string &actor,
                                          int &purgedCount) override;

  // -----------------------------------------------------------------------
  // Users
  // -----------------------------------------------------------------------
  [[nodiscard]] bool createUser(const core::User &user,
                                const std::string &actor = "") override;
  [[nodiscard]] std::unique_ptr<core::User> getUser(int id) override;
  [[nodiscard]] std::unique_ptr<core::User>
  getUserByUsername(const std::string &username) override;
  [[nodiscard]] std::vector<std::unique_ptr<core::User>> getAllUsers() override;
  [[nodiscard]] bool updateUser(const core::User &user,
                                const std::string &actor = "") override;
  [[nodiscard]] bool deleteUser(int id, const std::string &actor = "") override;

  // -----------------------------------------------------------------------
  // Roles & permissions
  // -----------------------------------------------------------------------
  [[nodiscard]] bool
  createRole(const std::string &name,
             const std::vector<std::string> &permissions,
             const std::string &actor = "") override;
  [[nodiscard]] bool
  updateRole(const std::string &name,
             const std::vector<std::string> &permissions,
             const std::string &actor = "") override;
  [[nodiscard]] std::vector<std::string>
  getRolePermissions(const std::string &name) override;
  [[nodiscard]] std::vector<std::string> getAllRoles() override;
  [[nodiscard]] bool assignUserRole(int userId, const std::string &roleName,
                                    const std::string &actor = "") override;

  // -----------------------------------------------------------------------
  // Auth config, LDAP, MFA
  // -----------------------------------------------------------------------
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
  [[nodiscard]] std::string generateMfaSecret() override;
  [[nodiscard]] std::string
  getMfaEnrollmentUri(const std::string &username,
                      const std::string &base32Secret) override;
  [[nodiscard]] bool setUserMfaSecret(int userId,
                                      const std::string &base32Secret) override;
  [[nodiscard]] bool disableUserMfa(int userId) override;
  [[nodiscard]] bool
  verifyMfaCodeForEnrollment(const std::string &base32Secret,
                              const std::string &code) override;

  // -----------------------------------------------------------------------
  // Session tracking
  // -----------------------------------------------------------------------
  [[nodiscard]] bool startSession(int userId, const std::string &username,
                                  AuthMethod method,
                                  const std::string &details = "") override;
  [[nodiscard]] bool endSession(int userId, const std::string &username,
                                const std::string &reason = "") override;
  [[nodiscard]] std::optional<int> getActiveSessionId(int userId) override;
  [[nodiscard]] int getActiveSessionCount(int userId) override;
  [[nodiscard]] int getSessionCount(int userId) override;
  [[nodiscard]] bool hasActiveSession(int userId) override;
  [[nodiscard]] std::optional<SessionInfo>
  getSessionById(int sessionId) override;
  [[nodiscard]] std::optional<SessionInfo>
  getLatestSessionForUser(int userId) override;

  // -----------------------------------------------------------------------
  // Authentication
  // -----------------------------------------------------------------------
  [[nodiscard]] AuthResult
  authenticatePrimary(const std::string &username,
                      const std::string &password) override;
  [[nodiscard]] std::unique_ptr<core::User>
  authenticateUser(const std::string &username, const std::string &password,
                   const std::optional<std::string> &mfaCode = std::nullopt) override;

  // -----------------------------------------------------------------------
  // Error handling
  // -----------------------------------------------------------------------
  [[nodiscard]] const std::string &getLastError() const override;
  [[nodiscard]] bool hasError() const override;
  void clearError() override;

private:
  std::string connectionString_;
  std::string lastError_;

  static constexpr const char *kNotImplemented =
      "PostgreSQL backend not yet implemented (scheduled for v1.1)";

  void setNotImplemented();
};

} // namespace db
} // namespace opensylab
