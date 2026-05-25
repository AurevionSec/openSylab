#pragma once

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

namespace opensylab {
namespace db {

/**
 * @brief Abstract database interface for OpenSylab LIMS
 *
 * Pure virtual interface that decouples all consumers from the concrete
 * database backend (SQLite, PostgreSQL, …). Any backend must implement
 * this interface; callers depend only on IDatabase.
 *
 * v0.9.0: SQLiteDatabase (via Database) is the production backend.
 *         PostgreSQLDatabase is a stub scheduled for full implementation
 *         in v1.1.
 */
class IDatabase {
public:
  // -----------------------------------------------------------------------
  // Nested types (shared between all implementations)
  // -----------------------------------------------------------------------

  struct SampleFilter {
    std::string query;
    std::string status;
    std::optional<std::time_t> fromDate;
    std::optional<std::time_t> toDate;
    bool excludeArchived = false;
    std::optional<int> limit;
    std::optional<int> offset;
  };

  struct OrderFilter {
    std::string status;
    std::string sampleId;
    std::string priority;
    std::optional<int> limit;
    std::optional<int> offset;
  };

  struct AuditLogFilter {
    std::optional<std::string> user;
    std::optional<core::AuditEntry::ActionType> action;
    std::optional<core::AuditEntry::EntityType> entity;
    std::optional<std::string> entityId;
    std::optional<std::time_t> fromTime;
    std::optional<std::time_t> toTime;
    int limit = 1000;
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

  struct BatchInsertError {
    size_t index = 0;
    std::string message;
  };

  struct BatchInsertResult {
    size_t inserted = 0;
    std::vector<BatchInsertError> failures;
  };

  // -----------------------------------------------------------------------
  // Lifecycle
  // -----------------------------------------------------------------------

  virtual ~IDatabase() = default;

  [[nodiscard]] virtual bool open() = 0;
  virtual bool close() = 0;
  [[nodiscard]] virtual bool isOpen() const = 0;

  // -----------------------------------------------------------------------
  // Schema
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual bool initializeSchema() = 0;

  // -----------------------------------------------------------------------
  // System health
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual HealthStatus getHealthStatus() = 0;

  // -----------------------------------------------------------------------
  // Samples
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual bool createSample(const core::Sample &sample,
                                          const std::string &actor = "") = 0;
  [[nodiscard]] virtual std::unique_ptr<core::Sample> getSample(int id) = 0;
  [[nodiscard]] virtual std::unique_ptr<core::Sample>
  getSampleByBarcode(const std::string &barcode) = 0;
  [[nodiscard]] virtual std::vector<std::unique_ptr<core::Sample>>
  getAllSamples() = 0;
  [[nodiscard]] virtual std::vector<std::unique_ptr<core::Sample>>
  getSamplesByFilter(const SampleFilter &filter) = 0;
  [[nodiscard]] virtual int getSamplesCount(const SampleFilter &filter) = 0;
  [[nodiscard]] virtual bool updateSample(const core::Sample &sample,
                                          const std::string &actor = "") = 0;
  [[nodiscard]] virtual bool deleteSample(int id,
                                          const std::string &actor = "") = 0;
  [[nodiscard]] virtual bool
  exportSamplesToCsv(const std::string &filePath) = 0;

  [[nodiscard]] virtual BatchInsertResult
  createSamplesBatch(const std::vector<core::Sample> &samples,
                     const std::string &actor = "") = 0;

  // -----------------------------------------------------------------------
  // Orders
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual bool createOrder(const core::Order &order,
                                         const std::string &actor = "") = 0;
  [[nodiscard]] virtual std::unique_ptr<core::Order> getOrder(int id) = 0;
  [[nodiscard]] virtual std::unique_ptr<core::Order>
  getOrderByOrderId(const std::string &orderId) = 0;
  [[nodiscard]] virtual std::vector<std::unique_ptr<core::Order>>
  getOrdersBySampleId(const std::string &sampleId) = 0;
  [[nodiscard]] virtual std::vector<std::unique_ptr<core::Order>>
  getAllOrders() = 0;
  [[nodiscard]] virtual std::vector<std::unique_ptr<core::Order>>
  getOrdersByFilter(const OrderFilter &filter) = 0;
  [[nodiscard]] virtual int getOrdersCount(const OrderFilter &filter) = 0;
  [[nodiscard]] virtual bool updateOrder(const core::Order &order,
                                         const std::string &actor = "") = 0;
  [[nodiscard]] virtual bool deleteOrder(int id,
                                         const std::string &actor = "") = 0;

  // -----------------------------------------------------------------------
  // TestResults
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual bool
  createTestResult(const core::TestResult &result,
                   const std::string &actor = "") = 0;
  [[nodiscard]] virtual std::unique_ptr<core::TestResult>
  getTestResult(int id) = 0;
  [[nodiscard]] virtual std::unique_ptr<core::TestResult>
  getTestResultByResultId(const std::string &resultId) = 0;
  [[nodiscard]] virtual std::vector<std::unique_ptr<core::TestResult>>
  getTestResultsByOrderId(int orderId,
                          std::optional<int> limit = std::nullopt,
                          std::optional<int> offset = std::nullopt) = 0;
  [[nodiscard]] virtual int
  getTestResultsCount(std::optional<int> orderIdFilter = std::nullopt) = 0;
  [[nodiscard]] virtual std::vector<std::unique_ptr<core::TestResult>>
  getAllTestResults(std::optional<int> limit = std::nullopt,
                   std::optional<int> offset = std::nullopt) = 0;

  [[nodiscard]] virtual BatchInsertResult
  createTestResultsBatch(const std::vector<core::TestResult> &results,
                         const std::string &actor = "") = 0;
  [[nodiscard]] virtual bool
  updateTestResult(const core::TestResult &result,
                   const std::string &actor = "") = 0;
  [[nodiscard]] virtual bool
  updateTestResultWithAudit(const core::TestResult &result,
                            const std::string &user) = 0;
  [[nodiscard]] virtual bool
  exportValidatedResultsToCsv(const std::string &filePath,
                               const std::string &user,
                               std::optional<int> orderId = std::nullopt) = 0;
  [[nodiscard]] virtual bool validateTestResult(const std::string &resultId,
                                                const std::string &user) = 0;
  [[nodiscard]] virtual bool deleteTestResult(int id,
                                              const std::string &actor = "") = 0;

  // -----------------------------------------------------------------------
  // Statistics
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual EntityStats
  getSampleStats(const StatsFilter &filter = StatsFilter{}) = 0;
  [[nodiscard]] virtual EntityStats
  getOrderStats(const StatsFilter &filter = StatsFilter{}) = 0;
  [[nodiscard]] virtual EntityStats
  getResultStats(const StatsFilter &filter = StatsFilter{}) = 0;
  [[nodiscard]] virtual std::vector<StatusCount> getOrderPriorityStats() = 0;
  [[nodiscard]] virtual int getCriticalResultCount() = 0;
  [[nodiscard]] virtual bool
  exportStatsReportToCsv(const std::string &filePath,
                          const StatsFilter &sampleFilter,
                          const StatsFilter &orderFilter,
                          const StatsFilter &resultFilter,
                          const std::string &actor) = 0;

  // -----------------------------------------------------------------------
  // Audit log
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual bool logAudit(const core::AuditEntry &entry) = 0;
  [[nodiscard]] virtual std::string getLastAuditHash() = 0;
  [[nodiscard]] virtual bool verifyAuditChain(std::string &firstBrokenAt) = 0;
  virtual void setAuditHmacKey(const std::string &key) = 0;
  [[nodiscard]] virtual std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLog(int limit = 1000) = 0;
  [[nodiscard]] virtual std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLogByEntity(core::AuditEntry::EntityType entity,
                      const std::string &entityId) = 0;
  [[nodiscard]] virtual std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLogFiltered(const AuditLogFilter &filter) = 0;
  [[nodiscard]] virtual bool
  exportAuditLogToCsv(const std::string &filePath, const AuditLogFilter &filter,
                      const std::string &actor, int &exportedCount) = 0;
  [[nodiscard]] virtual std::vector<std::unique_ptr<core::AuditEntry>>
  getDiagnosticsLogs(const DiagnosticsFilter &filter) = 0;
  [[nodiscard]] virtual bool
  exportDiagnosticsLogsToCsv(const std::string &filePath,
                              const DiagnosticsFilter &filter,
                              const std::string &actor,
                              int &exportedCount) = 0;

  // -----------------------------------------------------------------------
  // Audit helper actions
  // -----------------------------------------------------------------------

  virtual void logSampleAction(core::AuditEntry::ActionType action,
                                const std::string &sampleId,
                                const std::string &user,
                                const std::string &details = "") = 0;
  virtual void logOrderAction(core::AuditEntry::ActionType action,
                               const std::string &orderId,
                               const std::string &user,
                               const std::string &details = "") = 0;
  virtual void logResultAction(core::AuditEntry::ActionType action,
                                const std::string &resultId,
                                const std::string &user,
                                const std::string &details = "") = 0;
  virtual void logUserAction(core::AuditEntry::ActionType action,
                              const std::string &username,
                              const std::string &user,
                              const std::string &details = "") = 0;
  virtual void logRoleAction(core::AuditEntry::ActionType action,
                              const std::string &roleName,
                              const std::string &user,
                              const std::string &details = "") = 0;
  [[nodiscard]] virtual bool
  logSupportAccess(core::AuditEntry::EntityType entity,
                   const std::string &entityId, const std::string &user,
                   const std::string &details = "") = 0;
  [[nodiscard]] virtual bool
  logResultRetryImport(const std::vector<std::string> &resultIds,
                       const std::string &user,
                       const std::string &filePath) = 0;

  // -----------------------------------------------------------------------
  // API keys
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual bool upsertApiKey(const std::string &key,
                                          bool active = true,
                                          const std::string &role = "OPERATOR",
                                          const std::string &actor = "") = 0;
  [[nodiscard]] virtual std::optional<std::string>
  isApiKeyValid(const std::string &key) = 0;
  virtual bool persistBlacklistedToken(const std::string &token,
                                       std::time_t expiresAt) = 0;
  [[nodiscard]] virtual std::vector<std::pair<std::string, std::time_t>>
  loadActiveBlacklistedTokens() = 0;
  virtual void pruneExpiredBlacklistedTokens() = 0;

  // -----------------------------------------------------------------------
  // Retention
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual int getRetentionDays() = 0;
  [[nodiscard]] virtual bool setRetentionDays(int days) = 0;
  [[nodiscard]] virtual bool applyAuditRetention(const std::string &actor,
                                                  int &purgedCount) = 0;

  // -----------------------------------------------------------------------
  // Users
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual bool createUser(const core::User &user,
                                        const std::string &actor = "") = 0;
  [[nodiscard]] virtual std::unique_ptr<core::User> getUser(int id) = 0;
  [[nodiscard]] virtual std::unique_ptr<core::User>
  getUserByUsername(const std::string &username) = 0;
  [[nodiscard]] virtual std::vector<std::unique_ptr<core::User>>
  getAllUsers() = 0;
  [[nodiscard]] virtual bool updateUser(const core::User &user,
                                        const std::string &actor = "") = 0;
  [[nodiscard]] virtual bool deleteUser(int id,
                                        const std::string &actor = "") = 0;

  // -----------------------------------------------------------------------
  // Roles & permissions
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual bool
  createRole(const std::string &name,
             const std::vector<std::string> &permissions,
             const std::string &actor = "") = 0;
  [[nodiscard]] virtual bool
  updateRole(const std::string &name,
             const std::vector<std::string> &permissions,
             const std::string &actor = "") = 0;
  [[nodiscard]] virtual std::vector<std::string>
  getRolePermissions(const std::string &name) = 0;
  [[nodiscard]] virtual std::vector<std::string> getAllRoles() = 0;
  [[nodiscard]] virtual bool assignUserRole(int userId,
                                            const std::string &roleName,
                                            const std::string &actor = "") = 0;

  // -----------------------------------------------------------------------
  // Auth config, LDAP, MFA
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual bool setAuthConfig(const std::string &key,
                                           const std::string &value) = 0;
  [[nodiscard]] virtual std::optional<std::string>
  getAuthConfig(const std::string &key) = 0;
  [[nodiscard]] virtual bool setLdapEnabled(bool enabled) = 0;
  [[nodiscard]] virtual bool isLdapEnabled() = 0;
  [[nodiscard]] virtual bool upsertLdapUser(const std::string &username,
                                            const std::string &passwordHash,
                                            bool active, bool mfaRequired,
                                            const std::string &mfaSecret) = 0;
  [[nodiscard]] virtual bool
  setUserMfaRequirement(const std::string &username, bool required,
                         const std::string &mfaSecret) = 0;
  [[nodiscard]] virtual bool
  setRoleMfaRequirement(const std::string &roleName, bool required) = 0;
  [[nodiscard]] virtual bool
  isMfaRequiredForUser(const std::string &username,
                        const std::string &roleName) = 0;
  [[nodiscard]] virtual bool verifyUserMfa(const std::string &username,
                                           const std::string &code) = 0;

  // MFA enrollment
  [[nodiscard]] virtual std::string generateMfaSecret() = 0;
  [[nodiscard]] virtual std::string
  getMfaEnrollmentUri(const std::string &username,
                      const std::string &base32Secret) = 0;
  [[nodiscard]] virtual bool setUserMfaSecret(
      int userId, const std::string &base32Secret,
      int64_t initialUsedStep = -1) = 0;
  [[nodiscard]] virtual bool disableUserMfa(int userId,
                                           const std::string &actor = "") = 0;
  // Verify a TOTP code against a raw Base32 secret (for enrollment).
  // Sets matchedStep to the consumed time-step on success so the caller can
  // pass it to setUserMfaSecret, preventing replay of the enrollment code.
  [[nodiscard]] virtual bool
  verifyMfaCodeForEnrollment(const std::string &base32Secret,
                              const std::string &code,
                              int64_t &matchedStep) = 0;
  // Verify TOTP code with replay prevention (for login — updates last-used step)
  [[nodiscard]] virtual bool
  verifyAndConsumeMfaCode(const std::string &username,
                           const std::string &secret,
                           const std::string &code) = 0;

  // -----------------------------------------------------------------------
  // Session tracking
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual bool startSession(int userId,
                                          const std::string &username,
                                          AuthMethod method,
                                          const std::string &details = "") = 0;
  [[nodiscard]] virtual bool endSession(int userId, const std::string &username,
                                        const std::string &reason = "") = 0;
  [[nodiscard]] virtual std::optional<int> getActiveSessionId(int userId) = 0;
  [[nodiscard]] virtual int getActiveSessionCount(int userId) = 0;
  [[nodiscard]] virtual int getSessionCount(int userId) = 0;
  [[nodiscard]] virtual bool hasActiveSession(int userId) = 0;
  [[nodiscard]] virtual std::optional<SessionInfo>
  getSessionById(int sessionId) = 0;
  [[nodiscard]] virtual std::optional<SessionInfo>
  getLatestSessionForUser(int userId) = 0;
  virtual int expireStaleSessionsOlderThan(int maxLifetimeSeconds) = 0;

  // -----------------------------------------------------------------------
  // Authentication
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual AuthResult
  authenticatePrimary(const std::string &username,
                      const std::string &password) = 0;
  [[nodiscard]] virtual std::unique_ptr<core::User>
  authenticateUser(const std::string &username, const std::string &password,
                   const std::optional<std::string> &mfaCode = std::nullopt) = 0;

  // -----------------------------------------------------------------------
  // Error handling
  // -----------------------------------------------------------------------

  [[nodiscard]] virtual std::time_t getPasswordChangedAt(int userId) = 0;

  [[nodiscard]] virtual std::string getLastError() const = 0;
  [[nodiscard]] virtual bool hasError() const = 0;
  virtual void clearError() = 0;
};

} // namespace db
} // namespace opensylab
