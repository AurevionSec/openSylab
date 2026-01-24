#include "db/Database.h"
#include <iostream>
#include <memory>
#include <sqlite3.h>
#include <sstream>
#include <vector>

namespace {
// Stellt sicher, dass vorbereitete Statements immer finalisiert werden.
using StatementPtr = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

StatementPtr makeStatement(sqlite3_stmt *stmt) {
  return StatementPtr(stmt, sqlite3_finalize);
}

std::string columnText(sqlite3_stmt *stmt, int index) {
  const unsigned char *text = sqlite3_column_text(stmt, index);
  return text ? reinterpret_cast<const char *>(text) : "";
}

std::unique_ptr<opensylab::core::Sample> sampleFromRow(sqlite3_stmt *stmt) {
  auto sample = std::make_unique<opensylab::core::Sample>();
  sample->setId(sqlite3_column_int(stmt, 0));
  sample->setSampleId(columnText(stmt, 1));
  sample->setPatientId(columnText(stmt, 2));
  sample->setPatientName(columnText(stmt, 3));
  sample->setDescription(columnText(stmt, 4));
  sample->setStatus(
      opensylab::core::Sample::stringToStatus(columnText(stmt, 5)));
  sample->setRegistrationDate(
      static_cast<std::time_t>(sqlite3_column_int64(stmt, 6)));
  return sample;
}

std::unique_ptr<opensylab::core::Order> orderFromRow(sqlite3_stmt *stmt) {
  auto order = std::make_unique<opensylab::core::Order>();
  order->setId(sqlite3_column_int(stmt, 0));
  order->setOrderId(columnText(stmt, 1));
  order->setSampleId(columnText(stmt, 2));
  order->setTestType(columnText(stmt, 3));
  order->setStatus(opensylab::core::Order::stringToStatus(columnText(stmt, 4)));
  order->setPriority(
      opensylab::core::Order::stringToPriority(columnText(stmt, 5)));
  order->setRequestedDate(
      static_cast<std::time_t>(sqlite3_column_int64(stmt, 6)));
  order->setCompletedDate(
      static_cast<std::time_t>(sqlite3_column_int64(stmt, 7)));
  order->setRequestedBy(columnText(stmt, 8));
  order->setNotes(columnText(stmt, 9));
  return order;
}

std::unique_ptr<opensylab::core::TestResult>
testResultFromRow(sqlite3_stmt *stmt) {
  auto result = std::make_unique<opensylab::core::TestResult>();
  result->setId(sqlite3_column_int(stmt, 0));
  result->setResultId(columnText(stmt, 1));
  result->setOrderId(sqlite3_column_int(stmt, 2));
  result->setTestParameter(columnText(stmt, 3));
  result->setValue(columnText(stmt, 4));
  result->setUnit(columnText(stmt, 5));
  result->setReferenceRange(columnText(stmt, 6));
  result->setReferenceLow(sqlite3_column_double(stmt, 7));
  result->setReferenceHigh(sqlite3_column_double(stmt, 8));
  result->setStatus(
      opensylab::core::TestResult::stringToStatus(columnText(stmt, 9)));
  result->setFlag(
      opensylab::core::TestResult::stringToFlag(columnText(stmt, 10)));
  result->setMeasuredDate(
      static_cast<std::time_t>(sqlite3_column_int64(stmt, 11)));
  result->setMeasuredBy(columnText(stmt, 12));
  result->setComment(columnText(stmt, 13));
  return result;
}

std::unique_ptr<opensylab::core::AuditEntry>
auditEntryFromRow(sqlite3_stmt *stmt) {
  auto entry = std::make_unique<opensylab::core::AuditEntry>();
  entry->setId(sqlite3_column_int(stmt, 0));
  entry->setAction(
      opensylab::core::AuditEntry::stringToAction(columnText(stmt, 1)));
  entry->setEntity(
      opensylab::core::AuditEntry::stringToEntity(columnText(stmt, 2)));
  entry->setEntityId(columnText(stmt, 3));
  entry->setUser(columnText(stmt, 4));
  entry->setTimestamp(static_cast<std::time_t>(sqlite3_column_int64(stmt, 5)));
  entry->setDetails(columnText(stmt, 6));
  return entry;
}

std::unique_ptr<opensylab::core::User> userFromRow(sqlite3_stmt *stmt) {
  auto user = std::make_unique<opensylab::core::User>();
  user->setId(sqlite3_column_int(stmt, 0));
  user->setUsername(columnText(stmt, 1));
  user->setPasswordHash(columnText(stmt, 2));
  user->setRole(opensylab::core::User::stringToRole(columnText(stmt, 3)));
  user->setActive(sqlite3_column_int(stmt, 4) != 0);
  user->setLastLogin(static_cast<std::time_t>(sqlite3_column_int64(stmt, 5)));
  user->setCreatedDate(static_cast<std::time_t>(sqlite3_column_int64(stmt, 6)));
  user->setFullName(columnText(stmt, 7));
  user->setEmail(columnText(stmt, 8));
  return user;
}
} // namespace

namespace opensylab {
namespace db {

Database::Database(const std::string &dbPath)
    : dbPath_(dbPath), db_(nullptr), isOpen_(false), lastError_("") {}

Database::~Database() { close(); }

bool Database::open() {
  clearError();

  if (isOpen_) {
    return true;
  }

  int rc = sqlite3_open(dbPath_.c_str(), &db_);
  if (rc != SQLITE_OK) {
    setError("Kann Datenbank nicht öffnen: " +
             std::string(sqlite3_errmsg(db_)));
    sqlite3_close(db_);
    db_ = nullptr;
    return false;
  }

  // Foreign Key Constraints aktivieren (SQLite hat sie standardmäßig aus)
  char *fkErrMsg = nullptr;
  rc = sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr,
                    &fkErrMsg);
  if (rc != SQLITE_OK) {
    std::cerr << "Warnung: Foreign Keys konnten nicht aktiviert werden";
    if (fkErrMsg) {
      std::cerr << ": " << fkErrMsg;
      sqlite3_free(fkErrMsg);
    }
    std::cerr << std::endl;
  }

  isOpen_ = true;
  std::cout << "Datenbank erfolgreich geöffnet: " << dbPath_ << std::endl;
  return true;
}

bool Database::close() {
  clearError();

  if (!isOpen_ || db_ == nullptr) {
    return true;
  }

  int rc = sqlite3_close(db_);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Schließen der Datenbank: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  db_ = nullptr;
  isOpen_ = false;
  return true;
}

bool Database::initializeSchema() {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS samples (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sample_id TEXT NOT NULL UNIQUE,
            patient_id TEXT NOT NULL,
            patient_name TEXT,
            description TEXT,
            status TEXT NOT NULL,
            registration_date INTEGER NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_sample_id ON samples(sample_id);
        CREATE INDEX IF NOT EXISTS idx_patient_id ON samples(patient_id);

        CREATE TABLE IF NOT EXISTS orders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id TEXT NOT NULL UNIQUE,
            sample_id TEXT NOT NULL,
            test_type TEXT NOT NULL,
            status TEXT NOT NULL,
            priority TEXT NOT NULL,
            requested_date INTEGER NOT NULL,
            completed_date INTEGER,
            requested_by TEXT,
            notes TEXT,
            FOREIGN KEY(sample_id) REFERENCES samples(sample_id)
        );

        CREATE INDEX IF NOT EXISTS idx_order_id ON orders(order_id);
        CREATE INDEX IF NOT EXISTS idx_order_sample_id ON orders(sample_id);
        CREATE INDEX IF NOT EXISTS idx_order_status ON orders(status);

        CREATE TABLE IF NOT EXISTS test_results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            result_id TEXT NOT NULL UNIQUE,
            order_id INTEGER NOT NULL,
            test_parameter TEXT NOT NULL,
            value TEXT,
            unit TEXT,
            reference_range TEXT,
            reference_low REAL,
            reference_high REAL,
            status TEXT NOT NULL,
            flag TEXT NOT NULL,
            measured_date INTEGER,
            measured_by TEXT,
            comment TEXT,
            FOREIGN KEY(order_id) REFERENCES orders(id)
        );

        CREATE INDEX IF NOT EXISTS idx_result_id ON test_results(result_id);
        CREATE INDEX IF NOT EXISTS idx_result_order_id ON test_results(order_id);
        CREATE INDEX IF NOT EXISTS idx_result_status ON test_results(status);

        CREATE TABLE IF NOT EXISTS audit_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            action TEXT NOT NULL,
            entity TEXT NOT NULL,
            entity_id TEXT,
            user TEXT,
            timestamp INTEGER NOT NULL,
            details TEXT
        );

        CREATE INDEX IF NOT EXISTS idx_audit_timestamp ON audit_log(timestamp);
        CREATE INDEX IF NOT EXISTS idx_audit_entity ON audit_log(entity, entity_id);
        CREATE INDEX IF NOT EXISTS idx_audit_user ON audit_log(user);

        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL,
            role TEXT NOT NULL,
            active INTEGER NOT NULL DEFAULT 1,
            last_login INTEGER,
            created_date INTEGER NOT NULL,
            full_name TEXT,
            email TEXT
        );

        CREATE INDEX IF NOT EXISTS idx_user_username ON users(username);
        CREATE INDEX IF NOT EXISTS idx_user_role ON users(role);
        CREATE INDEX IF NOT EXISTS idx_user_active ON users(active);
    )";

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, createTableSQL, nullptr, nullptr, &errMsg);

  if (rc != SQLITE_OK) {
    std::string error =
        "SQL-Fehler beim Erstellen des Schemas: " + std::string(errMsg);
    sqlite3_free(errMsg);
    setError(error);
    return false;
  }

  std::cout << "Datenbankschema erfolgreich initialisiert" << std::endl;
  return true;
}

bool Database::createSample(const core::Sample &sample) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *insertSQL = R"(
        INSERT INTO samples (sample_id, patient_id, patient_name, description, status, registration_date)
        VALUES (?, ?, ?, ?, ?, ?);
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des INSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, sample.getSampleId().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, sample.getPatientId().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, sample.getPatientName().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 4, sample.getDescription().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 5, sample.getStatusString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 6,
                     static_cast<sqlite3_int64>(sample.getRegistrationDate()));

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Einfügen der Probe: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  return true;
}

std::unique_ptr<core::Sample> Database::getSample(int id) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return nullptr;
  }

  const char *selectSQL = R"(
        SELECT id, sample_id, patient_id, patient_name, description, status, registration_date
        FROM samples WHERE id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, id);
  rc = sqlite3_step(stmt.get());

  if (rc == SQLITE_DONE) {
    // Kein Ergebnis - Probe existiert nicht
    setError("Probe mit ID " + std::to_string(id) + " nicht gefunden");
    return nullptr;
  } else if (rc != SQLITE_ROW) {
    // Tatsächlicher SQL-Fehler
    setError("SQL-Fehler beim Abrufen der Probe: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  try {
    return sampleFromRow(stmt.get());
  } catch (const std::exception &e) {
    setError("Fehler beim Verarbeiten der Probe: " + std::string(e.what()));
    return nullptr;
  }
}

std::unique_ptr<core::Sample>
Database::getSampleByBarcode(const std::string &barcode) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return nullptr;
  }

  const char *selectSQL = R"(
        SELECT id, sample_id, patient_id, patient_name, description, status, registration_date
        FROM samples WHERE sample_id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, barcode.c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(stmt.get());

  if (rc == SQLITE_DONE) {
    // Kein Ergebnis - Probe existiert nicht
    setError("Probe mit Barcode '" + barcode + "' nicht gefunden");
    return nullptr;
  } else if (rc != SQLITE_ROW) {
    // Tatsächlicher SQL-Fehler
    setError("SQL-Fehler beim Abrufen der Probe: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  try {
    return sampleFromRow(stmt.get());
  } catch (const std::exception &e) {
    setError("Fehler beim Verarbeiten der Probe: " + std::string(e.what()));
    return nullptr;
  }
}

std::vector<std::unique_ptr<core::Sample>> Database::getAllSamples() {
  std::vector<std::unique_ptr<core::Sample>> samples;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return samples;
  }

  const char *selectSQL = R"(
        SELECT id, sample_id, patient_id, patient_name, description, status, registration_date
        FROM samples ORDER BY registration_date DESC;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return samples;
  }

  auto stmt = makeStatement(rawStmt);

  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    try {
      samples.push_back(sampleFromRow(stmt.get()));
    } catch (const std::exception &e) {
      setError("Fehler beim Verarbeiten der Probe: " + std::string(e.what()));
      return samples;
    }
  }

  // Prüfen ob Fehler beim Iterieren aufgetreten ist
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Proben: " +
             std::string(sqlite3_errmsg(db_)));
  }

  // Kein Fehler - erfolgreich (auch wenn samples leer ist)
  return samples;
}

std::vector<std::unique_ptr<core::Sample>>
Database::getSamplesByFilter(const SampleFilter &filter) {
  std::vector<std::unique_ptr<core::Sample>> samples;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return samples;
  }

  std::vector<std::string> conditions;
  std::string queryPattern;
  if (!filter.query.empty()) {
    std::string escaped;
    escaped.reserve(filter.query.size());
    for (char c : filter.query) {
      if (c == '%' || c == '_' || c == '\\') {
        escaped.push_back('\\');
      }
      escaped.push_back(c);
    }
    queryPattern = "%" + escaped + "%";
    conditions.emplace_back(
        "(sample_id LIKE ? ESCAPE '\\' OR patient_id LIKE ? ESCAPE '\\' "
        "OR patient_name LIKE ? ESCAPE '\\')");
  }
  if (!filter.status.empty()) {
    conditions.emplace_back("status = ?");
  }
  if (filter.excludeArchived) {
    conditions.emplace_back("status != ?");
  }
  if (filter.fromDate.has_value()) {
    conditions.emplace_back("registration_date >= ?");
  }
  if (filter.toDate.has_value()) {
    conditions.emplace_back("registration_date <= ?");
  }

  std::ostringstream sql;
  sql << "SELECT id, sample_id, patient_id, patient_name, description, status, "
         "registration_date FROM samples";

  if (!conditions.empty()) {
    sql << " WHERE ";
    for (size_t i = 0; i < conditions.size(); ++i) {
      if (i > 0) {
        sql << " AND ";
      }
      sql << conditions[i];
    }
  }

  sql << " ORDER BY registration_date DESC;";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return samples;
  }

  auto stmt = makeStatement(rawStmt);

  int bindIndex = 1;
  if (!filter.query.empty()) {
    sqlite3_bind_text(stmt.get(), bindIndex++, queryPattern.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), bindIndex++, queryPattern.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), bindIndex++, queryPattern.c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  if (!filter.status.empty()) {
    sqlite3_bind_text(stmt.get(), bindIndex++, filter.status.c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  if (filter.excludeArchived) {
    std::string archived =
        core::Sample::statusToString(core::Sample::Status::ARCHIVED);
    sqlite3_bind_text(stmt.get(), bindIndex++, archived.c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  if (filter.fromDate.has_value()) {
    sqlite3_bind_int64(stmt.get(), bindIndex++,
                       static_cast<sqlite3_int64>(filter.fromDate.value()));
  }
  if (filter.toDate.has_value()) {
    sqlite3_bind_int64(stmt.get(), bindIndex++,
                       static_cast<sqlite3_int64>(filter.toDate.value()));
  }

  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    try {
      samples.push_back(sampleFromRow(stmt.get()));
    } catch (const std::exception &e) {
      setError("Fehler beim Verarbeiten der Probe: " + std::string(e.what()));
      return samples;
    }
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Proben: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return samples;
}

bool Database::updateSample(const core::Sample &sample) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *updateSQL = R"(
        UPDATE samples SET
            sample_id = ?,
            patient_id = ?,
            patient_name = ?,
            description = ?,
            status = ?,
            registration_date = ?
        WHERE id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, updateSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des UPDATE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, sample.getSampleId().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, sample.getPatientId().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, sample.getPatientName().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 4, sample.getDescription().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 5, sample.getStatusString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 6,
                     static_cast<sqlite3_int64>(sample.getRegistrationDate()));
  sqlite3_bind_int(stmt.get(), 7, sample.getId());

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Aktualisieren der Probe: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  // Prüfen, ob überhaupt ein Datensatz mit der ID existiert.
  // sqlite3_changes ist 0, wenn entweder kein Datensatz existiert ODER alle
  // Werte unverändert wären.
  if (sqlite3_changes(db_) == 0) {
    static const char *existsSQL =
        "SELECT 1 FROM samples WHERE id = ? LIMIT 1;";
    sqlite3_stmt *existsStmtRaw = nullptr;
    int existsRc =
        sqlite3_prepare_v2(db_, existsSQL, -1, &existsStmtRaw, nullptr);
    if (existsRc != SQLITE_OK) {
      setError("Fehler beim Prüfen der Probe: " +
               std::string(sqlite3_errmsg(db_)));
      return false;
    }
    auto existsStmt = makeStatement(existsStmtRaw);
    sqlite3_bind_int(existsStmt.get(), 1, sample.getId());
    existsRc = sqlite3_step(existsStmt.get());

    if (existsRc == SQLITE_ROW) {
      // Datensatz existiert, Werte waren unverändert -> trotzdem Erfolg
      return true;
    }

    if (existsRc == SQLITE_DONE) {
      setError("Probe mit ID " + std::to_string(sample.getId()) +
               " nicht gefunden");
    } else {
      setError("Fehler beim Prüfen der Probe: " +
               std::string(sqlite3_errmsg(db_)));
    }
    return false;
  }

  return true;
}

bool Database::deleteSample(int id) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *deleteSQL = "DELETE FROM samples WHERE id = ?;";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des DELETE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, id);
  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Löschen der Probe: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  // Prüfen ob tatsächlich eine Zeile gelöscht wurde
  int changes = sqlite3_changes(db_);
  if (changes == 0) {
    setError("Probe mit ID " + std::to_string(id) + " nicht gefunden");
    return false;
  }

  return true;
}

// ============================================================================
// Order-Operationen
// ============================================================================

bool Database::createOrder(const core::Order &order) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (order.getOrderId().empty()) {
    setError("Auftrags-ID darf nicht leer sein");
    return false;
  }
  if (order.getSampleId().empty()) {
    setError("Proben-ID darf nicht leer sein");
    return false;
  }
  if (order.getTestType().empty()) {
    setError("Testtyp darf nicht leer sein");
    return false;
  }

  const char *insertSQL = R"(
        INSERT INTO orders (order_id, sample_id, test_type, status, priority,
                           requested_date, completed_date, requested_by, notes)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des INSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, order.getOrderId().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, order.getSampleId().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, order.getTestType().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 4, order.getStatusString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 5, order.getPriorityString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 6,
                     static_cast<sqlite3_int64>(order.getRequestedDate()));
  sqlite3_bind_int64(stmt.get(), 7,
                     static_cast<sqlite3_int64>(order.getCompletedDate()));
  sqlite3_bind_text(stmt.get(), 8, order.getRequestedBy().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 9, order.getNotes().c_str(), -1,
                    SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Einfügen des Auftrags: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  return true;
}

std::unique_ptr<core::Order> Database::getOrder(int id) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return nullptr;
  }

  const char *selectSQL = R"(
        SELECT id, order_id, sample_id, test_type, status, priority,
               requested_date, completed_date, requested_by, notes
        FROM orders WHERE id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, id);
  rc = sqlite3_step(stmt.get());

  if (rc == SQLITE_DONE) {
    setError("Auftrag mit ID " + std::to_string(id) + " nicht gefunden");
    return nullptr;
  } else if (rc != SQLITE_ROW) {
    setError("SQL-Fehler beim Abrufen des Auftrags: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  try {
    return orderFromRow(stmt.get());
  } catch (const std::exception &e) {
    setError("Fehler beim Verarbeiten des Auftrags: " + std::string(e.what()));
    return nullptr;
  }
}

std::unique_ptr<core::Order>
Database::getOrderByOrderId(const std::string &orderId) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return nullptr;
  }

  const char *selectSQL = R"(
        SELECT id, order_id, sample_id, test_type, status, priority,
               requested_date, completed_date, requested_by, notes
        FROM orders WHERE order_id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, orderId.c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(stmt.get());

  if (rc == SQLITE_DONE) {
    setError("Auftrag mit ID '" + orderId + "' nicht gefunden");
    return nullptr;
  } else if (rc != SQLITE_ROW) {
    setError("SQL-Fehler beim Abrufen des Auftrags: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  try {
    return orderFromRow(stmt.get());
  } catch (const std::exception &e) {
    setError("Fehler beim Verarbeiten des Auftrags: " + std::string(e.what()));
    return nullptr;
  }
}

std::vector<std::unique_ptr<core::Order>>
Database::getOrdersBySampleId(const std::string &sampleId) {
  std::vector<std::unique_ptr<core::Order>> orders;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return orders;
  }

  const char *selectSQL = R"(
        SELECT id, order_id, sample_id, test_type, status, priority,
               requested_date, completed_date, requested_by, notes
        FROM orders WHERE sample_id = ? ORDER BY requested_date DESC;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return orders;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, sampleId.c_str(), -1, SQLITE_TRANSIENT);

  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    try {
      orders.push_back(orderFromRow(stmt.get()));
    } catch (const std::exception &e) {
      setError("Fehler beim Verarbeiten des Auftrags: " +
               std::string(e.what()));
      return orders;
    }
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Aufträge: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return orders;
}

std::vector<std::unique_ptr<core::Order>> Database::getAllOrders() {
  std::vector<std::unique_ptr<core::Order>> orders;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return orders;
  }

  const char *selectSQL = R"(
        SELECT id, order_id, sample_id, test_type, status, priority,
               requested_date, completed_date, requested_by, notes
        FROM orders ORDER BY requested_date DESC;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return orders;
  }

  auto stmt = makeStatement(rawStmt);

  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    try {
      orders.push_back(orderFromRow(stmt.get()));
    } catch (const std::exception &e) {
      setError("Fehler beim Verarbeiten des Auftrags: " +
               std::string(e.what()));
      return orders;
    }
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Aufträge: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return orders;
}

std::vector<std::unique_ptr<core::Order>>
Database::getOrdersByFilter(const OrderFilter &filter) {
  clearError();

  std::vector<std::unique_ptr<core::Order>> orders;

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return orders;
  }

  std::ostringstream query;
  query << R"(
        SELECT id, order_id, sample_id, test_type, status, priority,
               requested_date, completed_date, requested_by, notes
        FROM orders
        WHERE 1=1
    )";

  if (!filter.status.empty()) {
    query << " AND status = ?";
  }
  if (!filter.sampleId.empty()) {
    query << " AND sample_id = ?";
  }
  if (!filter.priority.empty()) {
    query << " AND priority = ?";
  }
  query << " ORDER BY requested_date DESC";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, query.str().c_str(), -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return orders;
  }
  auto stmt = makeStatement(rawStmt);

  int index = 1;
  if (!filter.status.empty()) {
    sqlite3_bind_text(stmt.get(), index++, filter.status.c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  if (!filter.sampleId.empty()) {
    sqlite3_bind_text(stmt.get(), index++, filter.sampleId.c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  if (!filter.priority.empty()) {
    sqlite3_bind_text(stmt.get(), index++, filter.priority.c_str(), -1,
                      SQLITE_TRANSIENT);
  }

  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    try {
      orders.push_back(orderFromRow(stmt.get()));
    } catch (const std::exception &e) {
      setError("Fehler beim Verarbeiten der Aufträge: " +
               std::string(e.what()));
      return {};
    }
  }

  if (rc != SQLITE_DONE) {
    setError("SQL-Fehler beim Abrufen der Aufträge: " +
             std::string(sqlite3_errmsg(db_)));
    return {};
  }

  return orders;
}

bool Database::updateOrder(const core::Order &order) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *updateSQL = R"(
        UPDATE orders SET
            order_id = ?,
            sample_id = ?,
            test_type = ?,
            status = ?,
            priority = ?,
            requested_date = ?,
            completed_date = ?,
            requested_by = ?,
            notes = ?
        WHERE id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, updateSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des UPDATE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, order.getOrderId().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, order.getSampleId().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, order.getTestType().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 4, order.getStatusString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 5, order.getPriorityString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 6,
                     static_cast<sqlite3_int64>(order.getRequestedDate()));
  sqlite3_bind_int64(stmt.get(), 7,
                     static_cast<sqlite3_int64>(order.getCompletedDate()));
  sqlite3_bind_text(stmt.get(), 8, order.getRequestedBy().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 9, order.getNotes().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 10, order.getId());

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Aktualisieren des Auftrags: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  if (sqlite3_changes(db_) == 0) {
    static const char *existsSQL = "SELECT 1 FROM orders WHERE id = ? LIMIT 1;";
    sqlite3_stmt *existsStmtRaw = nullptr;
    int existsRc =
        sqlite3_prepare_v2(db_, existsSQL, -1, &existsStmtRaw, nullptr);
    if (existsRc != SQLITE_OK) {
      setError("Fehler beim Prüfen des Auftrags: " +
               std::string(sqlite3_errmsg(db_)));
      return false;
    }
    auto existsStmt = makeStatement(existsStmtRaw);
    sqlite3_bind_int(existsStmt.get(), 1, order.getId());
    existsRc = sqlite3_step(existsStmt.get());

    if (existsRc == SQLITE_ROW) {
      return true;
    }

    if (existsRc == SQLITE_DONE) {
      setError("Auftrag mit ID " + std::to_string(order.getId()) +
               " nicht gefunden");
    } else {
      setError("Fehler beim Prüfen des Auftrags: " +
               std::string(sqlite3_errmsg(db_)));
    }
    return false;
  }

  return true;
}

bool Database::deleteOrder(int id) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *deleteSQL = "DELETE FROM orders WHERE id = ?;";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des DELETE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, id);
  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Löschen des Auftrags: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  int changes = sqlite3_changes(db_);
  if (changes == 0) {
    setError("Auftrag mit ID " + std::to_string(id) + " nicht gefunden");
    return false;
  }

  return true;
}

// ============================================================================
// TestResult-Operationen
// ============================================================================

bool Database::createTestResult(const core::TestResult &result) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (result.getResultId().empty()) {
    setError("Ergebnis-ID darf nicht leer sein");
    return false;
  }
  if (result.getOrderId() <= 0) {
    setError("Auftrags-ID ist ungültig");
    return false;
  }
  if (result.getTestParameter().empty()) {
    setError("Testparameter darf nicht leer sein");
    return false;
  }
  if (result.getValue().empty()) {
    setError("Messwert darf nicht leer sein");
    return false;
  }
  if (result.getUnit().empty()) {
    setError("Einheit darf nicht leer sein");
    return false;
  }

  auto order = getOrder(result.getOrderId());
  if (!order) {
    return false;
  }

  const char *insertSQL = R"(
        INSERT INTO test_results (result_id, order_id, test_parameter, value, unit,
                                  reference_range, reference_low, reference_high,
                                  status, flag, measured_date, measured_by, comment)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des INSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, result.getResultId().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 2, result.getOrderId());
  sqlite3_bind_text(stmt.get(), 3, result.getTestParameter().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 4, result.getValue().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 5, result.getUnit().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 6, result.getReferenceRange().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt.get(), 7, result.getReferenceLow());
  sqlite3_bind_double(stmt.get(), 8, result.getReferenceHigh());
  sqlite3_bind_text(stmt.get(), 9, result.getStatusString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 10, result.getFlagString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 11,
                     static_cast<sqlite3_int64>(result.getMeasuredDate()));
  sqlite3_bind_text(stmt.get(), 12, result.getMeasuredBy().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 13, result.getComment().c_str(), -1,
                    SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Einfügen des Ergebnisses: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  return true;
}

std::unique_ptr<core::TestResult> Database::getTestResult(int id) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return nullptr;
  }

  const char *selectSQL = R"(
        SELECT id, result_id, order_id, test_parameter, value, unit,
               reference_range, reference_low, reference_high,
               status, flag, measured_date, measured_by, comment
        FROM test_results WHERE id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, id);
  rc = sqlite3_step(stmt.get());

  if (rc == SQLITE_DONE) {
    setError("Ergebnis mit ID " + std::to_string(id) + " nicht gefunden");
    return nullptr;
  } else if (rc != SQLITE_ROW) {
    setError("SQL-Fehler beim Abrufen des Ergebnisses: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  try {
    return testResultFromRow(stmt.get());
  } catch (const std::exception &e) {
    setError("Fehler beim Verarbeiten des Ergebnisses: " +
             std::string(e.what()));
    return nullptr;
  }
}

std::unique_ptr<core::TestResult>
Database::getTestResultByResultId(const std::string &resultId) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return nullptr;
  }

  const char *selectSQL = R"(
        SELECT id, result_id, order_id, test_parameter, value, unit,
               reference_range, reference_low, reference_high,
               status, flag, measured_date, measured_by, comment
        FROM test_results WHERE result_id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, resultId.c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(stmt.get());

  if (rc == SQLITE_DONE) {
    setError("Ergebnis mit ID '" + resultId + "' nicht gefunden");
    return nullptr;
  } else if (rc != SQLITE_ROW) {
    setError("SQL-Fehler beim Abrufen des Ergebnisses: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  try {
    return testResultFromRow(stmt.get());
  } catch (const std::exception &e) {
    setError("Fehler beim Verarbeiten des Ergebnisses: " +
             std::string(e.what()));
    return nullptr;
  }
}

std::vector<std::unique_ptr<core::TestResult>>
Database::getTestResultsByOrderId(int orderId) {
  std::vector<std::unique_ptr<core::TestResult>> results;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return results;
  }

  const char *selectSQL = R"(
        SELECT id, result_id, order_id, test_parameter, value, unit,
               reference_range, reference_low, reference_high,
               status, flag, measured_date, measured_by, comment
        FROM test_results WHERE order_id = ? ORDER BY id;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return results;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, orderId);

  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    try {
      results.push_back(testResultFromRow(stmt.get()));
    } catch (const std::exception &e) {
      setError("Fehler beim Verarbeiten des Ergebnisses: " +
               std::string(e.what()));
      return results;
    }
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Ergebnisse: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return results;
}

std::vector<std::unique_ptr<core::TestResult>> Database::getAllTestResults() {
  std::vector<std::unique_ptr<core::TestResult>> results;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return results;
  }

  const char *selectSQL = R"(
        SELECT id, result_id, order_id, test_parameter, value, unit,
               reference_range, reference_low, reference_high,
               status, flag, measured_date, measured_by, comment
        FROM test_results ORDER BY id DESC;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return results;
  }

  auto stmt = makeStatement(rawStmt);

  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    try {
      results.push_back(testResultFromRow(stmt.get()));
    } catch (const std::exception &e) {
      setError("Fehler beim Verarbeiten des Ergebnisses: " +
               std::string(e.what()));
      return results;
    }
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Ergebnisse: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return results;
}

bool Database::updateTestResult(const core::TestResult &result) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *updateSQL = R"(
        UPDATE test_results SET
            result_id = ?,
            order_id = ?,
            test_parameter = ?,
            value = ?,
            unit = ?,
            reference_range = ?,
            reference_low = ?,
            reference_high = ?,
            status = ?,
            flag = ?,
            measured_date = ?,
            measured_by = ?,
            comment = ?
        WHERE id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, updateSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des UPDATE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, result.getResultId().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 2, result.getOrderId());
  sqlite3_bind_text(stmt.get(), 3, result.getTestParameter().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 4, result.getValue().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 5, result.getUnit().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 6, result.getReferenceRange().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt.get(), 7, result.getReferenceLow());
  sqlite3_bind_double(stmt.get(), 8, result.getReferenceHigh());
  sqlite3_bind_text(stmt.get(), 9, result.getStatusString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 10, result.getFlagString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 11,
                     static_cast<sqlite3_int64>(result.getMeasuredDate()));
  sqlite3_bind_text(stmt.get(), 12, result.getMeasuredBy().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 13, result.getComment().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 14, result.getId());

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Aktualisieren des Ergebnisses: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  if (sqlite3_changes(db_) == 0) {
    static const char *existsSQL =
        "SELECT 1 FROM test_results WHERE id = ? LIMIT 1;";
    sqlite3_stmt *existsStmtRaw = nullptr;
    int existsRc =
        sqlite3_prepare_v2(db_, existsSQL, -1, &existsStmtRaw, nullptr);
    if (existsRc != SQLITE_OK) {
      setError("Fehler beim Prüfen des Ergebnisses: " +
               std::string(sqlite3_errmsg(db_)));
      return false;
    }
    auto existsStmt = makeStatement(existsStmtRaw);
    sqlite3_bind_int(existsStmt.get(), 1, result.getId());
    existsRc = sqlite3_step(existsStmt.get());

    if (existsRc == SQLITE_ROW) {
      return true;
    }

    if (existsRc == SQLITE_DONE) {
      setError("Ergebnis mit ID " + std::to_string(result.getId()) +
               " nicht gefunden");
    } else {
      setError("Fehler beim Prüfen des Ergebnisses: " +
               std::string(sqlite3_errmsg(db_)));
    }
    return false;
  }

  return true;
}

bool Database::deleteTestResult(int id) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *deleteSQL = "DELETE FROM test_results WHERE id = ?;";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des DELETE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, id);
  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Löschen des Ergebnisses: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  int changes = sqlite3_changes(db_);
  if (changes == 0) {
    setError("Ergebnis mit ID " + std::to_string(id) + " nicht gefunden");
    return false;
  }

  return true;
}

// ============================================================================
// Audit-Operationen
// ============================================================================

bool Database::logAudit(const core::AuditEntry &entry) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *insertSQL = R"(
        INSERT INTO audit_log (action, entity, entity_id, user, timestamp, details)
        VALUES (?, ?, ?, ?, ?, ?);
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des INSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, entry.getActionString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, entry.getEntityString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, entry.getEntityId().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 4, entry.getUser().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 5,
                     static_cast<sqlite3_int64>(entry.getTimestamp()));
  sqlite3_bind_text(stmt.get(), 6, entry.getDetails().c_str(), -1,
                    SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Einfügen des Audit-Eintrags: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  return true;
}

std::vector<std::unique_ptr<core::AuditEntry>>
Database::getAuditLog(int limit) {
  std::vector<std::unique_ptr<core::AuditEntry>> entries;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return entries;
  }

  const char *selectSQL = R"(
        SELECT id, action, entity, entity_id, user, timestamp, details
        FROM audit_log ORDER BY timestamp DESC LIMIT ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return entries;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, limit);

  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    try {
      entries.push_back(auditEntryFromRow(stmt.get()));
    } catch (const std::exception &e) {
      setError("Fehler beim Verarbeiten des Audit-Eintrags: " +
               std::string(e.what()));
      return entries;
    }
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Audit-Einträge: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return entries;
}

std::vector<std::unique_ptr<core::AuditEntry>>
Database::getAuditLogByEntity(core::AuditEntry::EntityType entity,
                              const std::string &entityId) {
  std::vector<std::unique_ptr<core::AuditEntry>> entries;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return entries;
  }

  const char *selectSQL = R"(
        SELECT id, action, entity, entity_id, user, timestamp, details
        FROM audit_log WHERE entity = ? AND entity_id = ?
        ORDER BY timestamp DESC;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return entries;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1,
                    core::AuditEntry::entityToString(entity).c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, entityId.c_str(), -1, SQLITE_TRANSIENT);

  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    try {
      entries.push_back(auditEntryFromRow(stmt.get()));
    } catch (const std::exception &e) {
      setError("Fehler beim Verarbeiten des Audit-Eintrags: " +
               std::string(e.what()));
      return entries;
    }
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Audit-Einträge: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return entries;
}

// Audit-Hilfsmethoden
void Database::logSampleAction(core::AuditEntry::ActionType action,
                               const std::string &sampleId,
                               const std::string &user,
                               const std::string &details) {
  core::AuditEntry entry(action, core::AuditEntry::EntityType::SAMPLE, sampleId,
                         user, details);
  (void)logAudit(entry);
}

void Database::logOrderAction(core::AuditEntry::ActionType action,
                              const std::string &orderId,
                              const std::string &user,
                              const std::string &details) {
  core::AuditEntry entry(action, core::AuditEntry::EntityType::ORDER, orderId,
                         user, details);
  (void)logAudit(entry);
}

void Database::logResultAction(core::AuditEntry::ActionType action,
                               const std::string &resultId,
                               const std::string &user,
                               const std::string &details) {
  core::AuditEntry entry(action, core::AuditEntry::EntityType::RESULT, resultId,
                         user, details);
  (void)logAudit(entry);
}

// ============================================================================
// User-Operationen
// ============================================================================

bool Database::createUser(const core::User &user) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *insertSQL = R"(
        INSERT INTO users (username, password_hash, role, active, last_login,
                          created_date, full_name, email)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des INSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, user.getUsername().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, user.getPasswordHash().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, user.getRoleString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 4, user.isActive() ? 1 : 0);
  sqlite3_bind_int64(stmt.get(), 5,
                     static_cast<sqlite3_int64>(user.getLastLogin()));
  sqlite3_bind_int64(stmt.get(), 6,
                     static_cast<sqlite3_int64>(user.getCreatedDate()));
  sqlite3_bind_text(stmt.get(), 7, user.getFullName().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 8, user.getEmail().c_str(), -1,
                    SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Einfügen des Benutzers: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  return true;
}

std::unique_ptr<core::User> Database::getUser(int id) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return nullptr;
  }

  const char *selectSQL = R"(
        SELECT id, username, password_hash, role, active, last_login,
               created_date, full_name, email
        FROM users WHERE id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, id);
  rc = sqlite3_step(stmt.get());

  if (rc == SQLITE_DONE) {
    setError("Benutzer mit ID " + std::to_string(id) + " nicht gefunden");
    return nullptr;
  } else if (rc != SQLITE_ROW) {
    setError("SQL-Fehler beim Abrufen des Benutzers: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  try {
    return userFromRow(stmt.get());
  } catch (const std::exception &e) {
    setError("Fehler beim Verarbeiten des Benutzers: " + std::string(e.what()));
    return nullptr;
  }
}

std::unique_ptr<core::User>
Database::getUserByUsername(const std::string &username) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return nullptr;
  }

  const char *selectSQL = R"(
        SELECT id, username, password_hash, role, active, last_login,
               created_date, full_name, email
        FROM users WHERE username = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(stmt.get());

  if (rc == SQLITE_DONE) {
    setError("Benutzer '" + username + "' nicht gefunden");
    return nullptr;
  } else if (rc != SQLITE_ROW) {
    setError("SQL-Fehler beim Abrufen des Benutzers: " +
             std::string(sqlite3_errmsg(db_)));
    return nullptr;
  }

  try {
    return userFromRow(stmt.get());
  } catch (const std::exception &e) {
    setError("Fehler beim Verarbeiten des Benutzers: " + std::string(e.what()));
    return nullptr;
  }
}

std::vector<std::unique_ptr<core::User>> Database::getAllUsers() {
  std::vector<std::unique_ptr<core::User>> users;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return users;
  }

  const char *selectSQL = R"(
        SELECT id, username, password_hash, role, active, last_login,
               created_date, full_name, email
        FROM users ORDER BY username;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return users;
  }

  auto stmt = makeStatement(rawStmt);

  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    try {
      users.push_back(userFromRow(stmt.get()));
    } catch (const std::exception &e) {
      setError("Fehler beim Verarbeiten des Benutzers: " +
               std::string(e.what()));
      return users;
    }
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Benutzer: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return users;
}

bool Database::updateUser(const core::User &user) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *updateSQL = R"(
        UPDATE users SET
            username = ?,
            password_hash = ?,
            role = ?,
            active = ?,
            last_login = ?,
            created_date = ?,
            full_name = ?,
            email = ?
        WHERE id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, updateSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des UPDATE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_text(stmt.get(), 1, user.getUsername().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, user.getPasswordHash().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, user.getRoleString().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 4, user.isActive() ? 1 : 0);
  sqlite3_bind_int64(stmt.get(), 5,
                     static_cast<sqlite3_int64>(user.getLastLogin()));
  sqlite3_bind_int64(stmt.get(), 6,
                     static_cast<sqlite3_int64>(user.getCreatedDate()));
  sqlite3_bind_text(stmt.get(), 7, user.getFullName().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 8, user.getEmail().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 9, user.getId());

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Aktualisieren des Benutzers: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  if (sqlite3_changes(db_) == 0) {
    static const char *existsSQL = "SELECT 1 FROM users WHERE id = ? LIMIT 1;";
    sqlite3_stmt *existsStmtRaw = nullptr;
    int existsRc =
        sqlite3_prepare_v2(db_, existsSQL, -1, &existsStmtRaw, nullptr);
    if (existsRc != SQLITE_OK) {
      setError("Fehler beim Prüfen des Benutzers: " +
               std::string(sqlite3_errmsg(db_)));
      return false;
    }
    auto existsStmt = makeStatement(existsStmtRaw);
    sqlite3_bind_int(existsStmt.get(), 1, user.getId());
    existsRc = sqlite3_step(existsStmt.get());

    if (existsRc == SQLITE_ROW) {
      return true;
    }

    if (existsRc == SQLITE_DONE) {
      setError("Benutzer mit ID " + std::to_string(user.getId()) +
               " nicht gefunden");
    } else {
      setError("Fehler beim Prüfen des Benutzers: " +
               std::string(sqlite3_errmsg(db_)));
    }
    return false;
  }

  return true;
}

bool Database::deleteUser(int id) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *deleteSQL = "DELETE FROM users WHERE id = ?;";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des DELETE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, id);
  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Löschen des Benutzers: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  int changes = sqlite3_changes(db_);
  if (changes == 0) {
    setError("Benutzer mit ID " + std::to_string(id) + " nicht gefunden");
    return false;
  }

  return true;
}

std::unique_ptr<core::User>
Database::authenticateUser(const std::string &username,
                           const std::string &password) {
  clearError();

  auto user = getUserByUsername(username);
  if (!user) {
    setError("Ungültiger Benutzername oder Passwort");
    return nullptr;
  }

  if (!user->isActive()) {
    setError("Benutzer ist deaktiviert");
    return nullptr;
  }

  if (!user->verifyPassword(password)) {
    setError("Ungültiger Benutzername oder Passwort");
    return nullptr;
  }

  // Login-Timestamp aktualisieren
  user->updateLastLogin();
  (void)updateUser(*user);

  return user;
}

void Database::setError(const std::string &error) {
  lastError_ = error;
  std::cerr << "Datenbankfehler: " << error << std::endl;
}

} // namespace db
} // namespace opensylab
