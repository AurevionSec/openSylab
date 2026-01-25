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

  // Sample-Operationen (CRUD)
  [[nodiscard]] bool createSample(const core::Sample &sample);
  [[nodiscard]] std::unique_ptr<core::Sample> getSample(int id);
  [[nodiscard]] std::unique_ptr<core::Sample>
  getSampleByBarcode(const std::string &barcode);
  [[nodiscard]] std::vector<std::unique_ptr<core::Sample>> getAllSamples();
  [[nodiscard]] std::vector<std::unique_ptr<core::Sample>>
  getSamplesByFilter(const SampleFilter &filter);
  [[nodiscard]] bool updateSample(const core::Sample &sample);
  [[nodiscard]] bool deleteSample(int id);

  // Order-Operationen (CRUD)
  [[nodiscard]] bool createOrder(const core::Order &order);
  [[nodiscard]] std::unique_ptr<core::Order> getOrder(int id);
  [[nodiscard]] std::unique_ptr<core::Order>
  getOrderByOrderId(const std::string &orderId);
  [[nodiscard]] std::vector<std::unique_ptr<core::Order>>
  getOrdersBySampleId(const std::string &sampleId);
  [[nodiscard]] std::vector<std::unique_ptr<core::Order>> getAllOrders();
  [[nodiscard]] std::vector<std::unique_ptr<core::Order>>
  getOrdersByFilter(const OrderFilter &filter);
  [[nodiscard]] bool updateOrder(const core::Order &order);
  [[nodiscard]] bool deleteOrder(int id);

  // TestResult-Operationen (CRUD)
  [[nodiscard]] bool createTestResult(const core::TestResult &result);
  [[nodiscard]] std::unique_ptr<core::TestResult> getTestResult(int id);
  [[nodiscard]] std::unique_ptr<core::TestResult>
  getTestResultByResultId(const std::string &resultId);
  [[nodiscard]] std::vector<std::unique_ptr<core::TestResult>>
  getTestResultsByOrderId(int orderId);
  [[nodiscard]] std::vector<std::unique_ptr<core::TestResult>>
  getAllTestResults();
  [[nodiscard]] bool updateTestResult(const core::TestResult &result);
  [[nodiscard]] bool validateTestResult(const std::string &resultId,
                                        const std::string &user);
  [[nodiscard]] bool deleteTestResult(int id);

  // Audit-Operationen
  [[nodiscard]] bool logAudit(const core::AuditEntry &entry);
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLog(int limit = 100);
  [[nodiscard]] std::vector<std::unique_ptr<core::AuditEntry>>
  getAuditLogByEntity(core::AuditEntry::EntityType entity,
                      const std::string &entityId);

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

  // User-Operationen (CRUD)
  [[nodiscard]] bool createUser(const core::User &user);
  [[nodiscard]] std::unique_ptr<core::User> getUser(int id);
  [[nodiscard]] std::unique_ptr<core::User>
  getUserByUsername(const std::string &username);
  [[nodiscard]] std::vector<std::unique_ptr<core::User>> getAllUsers();
  [[nodiscard]] bool updateUser(const core::User &user);
  [[nodiscard]] bool deleteUser(int id);

  // Authentifizierung
  [[nodiscard]] std::unique_ptr<core::User>
  authenticateUser(const std::string &username, const std::string &password);

  // Fehlerbehandlung
  const std::string &getLastError() const { return lastError_; }
  bool hasError() const { return !lastError_.empty(); }
  void clearError() { lastError_.clear(); }

private:
  std::string dbPath_;
  sqlite3 *db_ = nullptr;
  bool isOpen_ = false;
  std::string lastError_;

  // Hilfsfunktionen
  void setError(const std::string &error);
};

} // namespace db
} // namespace opensylab

#endif // OPENSYLAB_DATABASE_H
