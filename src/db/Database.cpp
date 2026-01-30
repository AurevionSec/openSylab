#include "db/Database.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
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

int computeMfaCode(const std::string &secret, std::time_t now) {
  const long step = static_cast<long>(now / 30);
  const std::string material = secret + ":" + std::to_string(step);

  unsigned long hash = 5381;
  for (unsigned char c : material) {
    hash = ((hash << 5) + hash) ^ c;
  }
  return static_cast<int>(hash % 1000000UL);
}

bool verifyMfaCode(const std::string &secret, const std::string &code) {
  if (secret.empty() || code.empty()) {
    return false;
  }

  int provided = -1;
  try {
    provided = std::stoi(code);
  } catch (...) {
    return false;
  }

  const std::time_t now = std::time(nullptr);
  for (int offset = -1; offset <= 1; ++offset) {
    const std::time_t candidate = now + static_cast<std::time_t>(offset * 30);
    if (computeMfaCode(secret, candidate) == provided) {
      return true;
    }
  }
  return false;
}

std::string escapeCsvField(const std::string &value) {
  if (value.find_first_of(",\"\n\r") == std::string::npos) {
    return value;
  }

  std::string escaped;
  escaped.reserve(value.size() + 2);
  for (char ch : value) {
    if (ch == '"') {
      escaped.push_back('"');
    }
    escaped.push_back(ch);
  }
  return "\"" + escaped + "\"";
}

std::string normalizeActor(const std::string &user) {
  return user.empty() ? "system" : user;
}

struct StatsBindValue {
  enum class Type { Text, Int64 };
  Type type = Type::Text;
  std::string text;
  sqlite3_int64 number = 0;
};

struct StatsFilterSql {
  std::string whereClause;
  std::vector<StatsBindValue> bindings;
};

StatsFilterSql buildStatsFilter(const opensylab::db::Database::StatsFilter &filter,
                                const std::string &dateColumn) {
  StatsFilterSql result;
  std::vector<std::string> conditions;

  if (filter.status.has_value()) {
    conditions.emplace_back("status = ?");
    StatsBindValue binding;
    binding.type = StatsBindValue::Type::Text;
    binding.text = filter.status.value();
    result.bindings.push_back(std::move(binding));
  }
  if (filter.fromDate.has_value()) {
    conditions.emplace_back(dateColumn + " >= ?");
    StatsBindValue binding;
    binding.type = StatsBindValue::Type::Int64;
    binding.number =
        static_cast<sqlite3_int64>(filter.fromDate.value());
    result.bindings.push_back(std::move(binding));
  }
  if (filter.toDate.has_value()) {
    conditions.emplace_back(dateColumn + " <= ?");
    StatsBindValue binding;
    binding.type = StatsBindValue::Type::Int64;
    binding.number =
        static_cast<sqlite3_int64>(filter.toDate.value());
    result.bindings.push_back(std::move(binding));
  }

  if (!conditions.empty()) {
    std::ostringstream where;
    where << " WHERE ";
    for (size_t i = 0; i < conditions.size(); ++i) {
      if (i > 0) {
        where << " AND ";
      }
      where << conditions[i];
    }
    result.whereClause = where.str();
  }

  return result;
}

bool bindStatsFilter(sqlite3_stmt *stmt,
                     const std::vector<StatsBindValue> &bindings) {
  int index = 1;
  for (const auto &binding : bindings) {
    int rc = SQLITE_OK;
    if (binding.type == StatsBindValue::Type::Text) {
      rc = sqlite3_bind_text(stmt, index++, binding.text.c_str(), -1,
                             SQLITE_TRANSIENT);
    } else {
      rc = sqlite3_bind_int64(stmt, index++, binding.number);
    }
    if (rc != SQLITE_OK) {
      return false;
    }
  }
  return true;
}

void appendAuditChange(std::ostringstream &details, bool &hasChanges,
                       const std::string &label, const std::string &oldValue,
                       const std::string &newValue) {
  if (oldValue == newValue) {
    return;
  }
  if (hasChanges) {
    details << "; ";
  }
  details << label << ": " << oldValue << " -> " << newValue;
  hasChanges = true;
}

std::string joinList(const std::vector<std::string> &items) {
  if (items.empty()) {
    return "";
  }
  std::ostringstream joined;
  for (size_t i = 0; i < items.size(); ++i) {
    if (i > 0) {
      joined << ", ";
    }
    joined << items[i];
  }
  return joined.str();
}

std::string formatActive(bool active) {
  return active ? "aktiv" : "inaktiv";
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
  user->setRoleName(columnText(stmt, 3));
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

        CREATE TABLE IF NOT EXISTS retention_settings (
            key TEXT PRIMARY KEY,
            value INTEGER NOT NULL
        );

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

        CREATE TABLE IF NOT EXISTS roles (
            name TEXT PRIMARY KEY,
            description TEXT
        );

        CREATE TABLE IF NOT EXISTS role_permissions (
            role_name TEXT NOT NULL,
            permission TEXT NOT NULL,
            PRIMARY KEY (role_name, permission),
            FOREIGN KEY(role_name) REFERENCES roles(name) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS auth_config (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS api_keys (
            key TEXT PRIMARY KEY,
            active INTEGER NOT NULL DEFAULT 1,
            created_date INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS ldap_directory (
            username TEXT PRIMARY KEY,
            password_hash TEXT NOT NULL,
            active INTEGER NOT NULL DEFAULT 1,
            mfa_required INTEGER NOT NULL DEFAULT 0,
            mfa_secret TEXT NOT NULL DEFAULT ''
        );

        CREATE TABLE IF NOT EXISTS user_mfa (
            username TEXT PRIMARY KEY,
            mfa_required INTEGER NOT NULL DEFAULT 0,
            mfa_secret TEXT NOT NULL DEFAULT '',
            FOREIGN KEY(username) REFERENCES users(username) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS role_mfa (
            role_name TEXT PRIMARY KEY,
            mfa_required INTEGER NOT NULL DEFAULT 0,
            FOREIGN KEY(role_name) REFERENCES roles(name) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            username TEXT NOT NULL,
            method TEXT NOT NULL,
            login_ts INTEGER NOT NULL,
            logout_ts INTEGER,
            details TEXT NOT NULL DEFAULT '',
            FOREIGN KEY(user_id) REFERENCES users(id)
        );

        CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON sessions(user_id);
        CREATE INDEX IF NOT EXISTS idx_sessions_login_ts ON sessions(login_ts);
        CREATE INDEX IF NOT EXISTS idx_sessions_active_user
            ON sessions(user_id) WHERE logout_ts IS NULL;
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

  const char *seedRolesSQL = R"(
        INSERT OR IGNORE INTO roles (name, description)
        VALUES ('Administrator', 'Vollzugriff'),
               ('Operator', 'Standardzugriff'),
               ('Betrachter', 'Lesezugriff');
    )";
  rc = sqlite3_exec(db_, seedRolesSQL, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    std::string error =
        "SQL-Fehler beim Initialisieren der Rollen: " + std::string(errMsg);
    sqlite3_free(errMsg);
    setError(error);
    return false;
  }

  const char *seedAuthConfigSQL = R"(
        INSERT OR IGNORE INTO auth_config (key, value)
        VALUES ('ldap_enabled', '0');
    )";
  rc = sqlite3_exec(db_, seedAuthConfigSQL, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    std::string error =
        "SQL-Fehler beim Initialisieren der Auth-Konfiguration: " +
        std::string(errMsg);
    sqlite3_free(errMsg);
    setError(error);
    return false;
  }

  const char *seedRetentionSQL = R"(
        INSERT OR IGNORE INTO retention_settings (key, value)
        VALUES ('audit_log_days', 180);
    )";
  rc = sqlite3_exec(db_, seedRetentionSQL, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    std::string error =
        "SQL-Fehler beim Initialisieren der Retention-Konfiguration: " +
        std::string(errMsg);
    sqlite3_free(errMsg);
    setError(error);
    return false;
  }

  std::cout << "Datenbankschema erfolgreich initialisiert" << std::endl;
  return true;
}

Database::HealthStatus Database::getHealthStatus() {
  clearError();
  HealthStatus status;

  if (!isOpen_ || db_ == nullptr) {
    setError("Datenbank ist nicht geöffnet");
    status.dbOpen = false;
    status.schemaOk = false;
    return status;
  }

  status.dbOpen = true;

  auto tableExists = [&](const std::string &table) -> bool {
    const char *sql =
        "SELECT name FROM sqlite_master WHERE type='table' AND name=? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
      setError("SQL-Fehler bei Systemstatus: " +
               std::string(sqlite3_errmsg(db_)));
      return false;
    }
    auto handle = makeStatement(stmt);
    sqlite3_bind_text(stmt, 1, table.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    return rc == SQLITE_ROW;
  };

  const std::vector<std::string> requiredTables = {
      "samples", "orders", "test_results", "audit_log", "users"};

  for (const auto &table : requiredTables) {
    if (!tableExists(table)) {
      status.missingTables.push_back(table);
    }
  }

  status.schemaOk = status.missingTables.empty();
  return status;
}

bool Database::createSample(const core::Sample &sample,
                            const std::string &actor) {
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

  std::ostringstream details;
  details << "Patient-ID: " << sample.getPatientId() << "; Patient: "
          << sample.getPatientName()
          << "; Status: " << sample.getStatusString();
  if (!sample.getDescription().empty()) {
    details << "; Beschreibung: " << sample.getDescription();
  }
  logSampleAction(core::AuditEntry::ActionType::CREATE, sample.getSampleId(),
                  normalizeActor(actor), details.str());

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

bool Database::updateSample(const core::Sample &sample,
                            const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  auto existing = getSample(sample.getId());
  if (!existing) {
    return false;
  }

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
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
  rc = sqlite3_prepare_v2(db_, updateSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
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
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Fehler beim Aktualisieren der Probe: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  std::ostringstream details;
  bool hasChanges = false;
  appendAuditChange(details, hasChanges, "Proben-ID",
                    existing->getSampleId(), sample.getSampleId());
  appendAuditChange(details, hasChanges, "Patient-ID",
                    existing->getPatientId(), sample.getPatientId());
  appendAuditChange(details, hasChanges, "Patient",
                    existing->getPatientName(), sample.getPatientName());
  appendAuditChange(details, hasChanges, "Beschreibung",
                    existing->getDescription(), sample.getDescription());
  appendAuditChange(details, hasChanges, "Status",
                    existing->getStatusString(), sample.getStatusString());
  appendAuditChange(details, hasChanges, "Registriert",
                    std::to_string(existing->getRegistrationDate()),
                    std::to_string(sample.getRegistrationDate()));

  const std::string detailText =
      hasChanges ? details.str() : "Keine Änderungen";
  core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                         core::AuditEntry::EntityType::SAMPLE,
                         sample.getSampleId(), normalizeActor(actor),
                         detailText);
  if (!logAudit(entry)) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  return true;
}

bool Database::deleteSample(int id, const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  auto existing = getSample(id);
  if (!existing) {
    return false;
  }

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  const char *deleteSQL = "DELETE FROM samples WHERE id = ?;";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Fehler beim Vorbereiten des DELETE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, id);
  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Fehler beim Löschen der Probe: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  // Prüfen ob tatsächlich eine Zeile gelöscht wurde
  int changes = sqlite3_changes(db_);
  if (changes == 0) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Probe mit ID " + std::to_string(id) + " nicht gefunden");
    return false;
  }

  std::ostringstream details;
  details << "Proben-ID: " << existing->getSampleId()
          << "; Patient-ID: " << existing->getPatientId()
          << "; Status: " << existing->getStatusString();
  core::AuditEntry entry(core::AuditEntry::ActionType::DELETE,
                         core::AuditEntry::EntityType::SAMPLE,
                         existing->getSampleId(), normalizeActor(actor),
                         details.str());
  if (!logAudit(entry)) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  return true;
}

bool Database::exportSamplesToCsv(const std::string &filePath) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  auto samples = getAllSamples();
  if (hasError()) {
    return false;
  }

  if (samples.empty()) {
    setError("Keine Proben zum Export");
    return false;
  }

  std::ofstream output(filePath);
  if (!output.is_open()) {
    setError("Exportdatei konnte nicht geschrieben werden");
    return false;
  }

  output << "sample_id,patient_id,patient_name,description,status\n";
  for (const auto &sample : samples) {
    output << escapeCsvField(sample->getSampleId()) << ","
           << escapeCsvField(sample->getPatientId()) << ","
           << escapeCsvField(sample->getPatientName()) << ","
           << escapeCsvField(sample->getDescription()) << ","
           << escapeCsvField(sample->getStatusString()) << "\n";
  }

  if (!output) {
    setError("Fehler beim Schreiben der Exportdatei");
    return false;
  }

  return true;
}

// ============================================================================
// Order-Operationen
// ============================================================================

bool Database::createOrder(const core::Order &order, const std::string &actor) {
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

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  const char *insertSQL = R"(
        INSERT INTO orders (order_id, sample_id, test_type, status, priority,
                           requested_date, completed_date, requested_by, notes)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
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
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Fehler beim Einfügen des Auftrags: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  std::ostringstream details;
  details << "Proben-ID: " << order.getSampleId()
          << "; Testtyp: " << order.getTestType()
          << "; Status: " << order.getStatusString()
          << "; Priorität: " << order.getPriorityString();
  core::AuditEntry entry(core::AuditEntry::ActionType::CREATE,
                         core::AuditEntry::EntityType::ORDER,
                         order.getOrderId(), normalizeActor(actor),
                         details.str());
  if (!logAudit(entry)) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
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

bool Database::updateOrder(const core::Order &order, const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  auto existing = getOrder(order.getId());
  if (!existing) {
    return false;
  }

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
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
  rc = sqlite3_prepare_v2(db_, updateSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
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
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Fehler beim Aktualisieren des Auftrags: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  std::ostringstream details;
  bool hasChanges = false;
  appendAuditChange(details, hasChanges, "Auftrags-ID",
                    existing->getOrderId(), order.getOrderId());
  appendAuditChange(details, hasChanges, "Proben-ID",
                    existing->getSampleId(), order.getSampleId());
  appendAuditChange(details, hasChanges, "Testtyp",
                    existing->getTestType(), order.getTestType());
  appendAuditChange(details, hasChanges, "Status",
                    existing->getStatusString(), order.getStatusString());
  appendAuditChange(details, hasChanges, "Priorität",
                    existing->getPriorityString(), order.getPriorityString());
  appendAuditChange(details, hasChanges, "Angefragt",
                    std::to_string(existing->getRequestedDate()),
                    std::to_string(order.getRequestedDate()));
  appendAuditChange(details, hasChanges, "Abgeschlossen",
                    std::to_string(existing->getCompletedDate()),
                    std::to_string(order.getCompletedDate()));
  appendAuditChange(details, hasChanges, "Angefragt von",
                    existing->getRequestedBy(), order.getRequestedBy());
  appendAuditChange(details, hasChanges, "Notizen",
                    existing->getNotes(), order.getNotes());

  const std::string detailText =
      hasChanges ? details.str() : "Keine Änderungen";
  core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                         core::AuditEntry::EntityType::ORDER,
                         order.getOrderId(), normalizeActor(actor),
                         detailText);
  if (!logAudit(entry)) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  return true;
}

bool Database::deleteOrder(int id, const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  auto existing = getOrder(id);
  if (!existing) {
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

  std::ostringstream details;
  details << "Auftrags-ID: " << existing->getOrderId()
          << "; Proben-ID: " << existing->getSampleId()
          << "; Status: " << existing->getStatusString();
  logOrderAction(core::AuditEntry::ActionType::DELETE,
                 existing->getOrderId(), normalizeActor(actor), details.str());

  return true;
}

// ============================================================================
// TestResult-Operationen
// ============================================================================

bool Database::createTestResult(const core::TestResult &result,
                                const std::string &actor) {
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

  const std::string computedFlag =
      core::TestResult::flagToString(result.evaluateFlag());

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  const char *insertSQL = R"(
        INSERT INTO test_results (result_id, order_id, test_parameter, value, unit,
                                  reference_range, reference_low, reference_high,
                                  status, flag, measured_date, measured_by, comment)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
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
  sqlite3_bind_text(stmt.get(), 10, computedFlag.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 11,
                     static_cast<sqlite3_int64>(result.getMeasuredDate()));
  sqlite3_bind_text(stmt.get(), 12, result.getMeasuredBy().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 13, result.getComment().c_str(), -1,
                    SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Fehler beim Einfügen des Ergebnisses: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  std::ostringstream details;
  details << "Auftrags-ID: " << result.getOrderId()
          << "; Parameter: " << result.getTestParameter()
          << "; Status: " << result.getStatusString();
  if (!result.getValue().empty()) {
    details << "; Wert: " << result.getValue();
  }
  if (!result.getUnit().empty()) {
    details << " " << result.getUnit();
  }
  core::AuditEntry entry(core::AuditEntry::ActionType::CREATE,
                         core::AuditEntry::EntityType::RESULT,
                         result.getResultId(), normalizeActor(actor),
                         details.str());
  if (!logAudit(entry)) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
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

bool Database::updateTestResult(const core::TestResult &result,
                                const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  auto existing = getTestResult(result.getId());
  if (!existing) {
    return false;
  }

  if (!updateTestResultCore(result)) {
    return false;
  }

  const std::string computedFlag =
      core::TestResult::flagToString(result.evaluateFlag());

  std::ostringstream details;
  bool hasChanges = false;
  appendAuditChange(details, hasChanges, "Parameter",
                    existing->getTestParameter(), result.getTestParameter());
  appendAuditChange(details, hasChanges, "Wert", existing->getValue(),
                    result.getValue());
  appendAuditChange(details, hasChanges, "Einheit", existing->getUnit(),
                    result.getUnit());
  appendAuditChange(details, hasChanges, "Referenzbereich",
                    existing->getReferenceRange(),
                    result.getReferenceRange());
  appendAuditChange(details, hasChanges, "Referenz (low)",
                    std::to_string(existing->getReferenceLow()),
                    std::to_string(result.getReferenceLow()));
  appendAuditChange(details, hasChanges, "Referenz (high)",
                    std::to_string(existing->getReferenceHigh()),
                    std::to_string(result.getReferenceHigh()));
  appendAuditChange(details, hasChanges, "Status",
                    existing->getStatusString(), result.getStatusString());
  appendAuditChange(details, hasChanges, "Flag", existing->getFlagString(),
                    computedFlag);
  appendAuditChange(details, hasChanges, "Gemessen am",
                    std::to_string(existing->getMeasuredDate()),
                    std::to_string(result.getMeasuredDate()));
  appendAuditChange(details, hasChanges, "Gemessen von",
                    existing->getMeasuredBy(), result.getMeasuredBy());
  appendAuditChange(details, hasChanges, "Kommentar",
                    existing->getComment(), result.getComment());

  const std::string detailText =
      hasChanges ? details.str() : "Keine Änderungen";
  logResultAction(core::AuditEntry::ActionType::UPDATE, result.getResultId(),
                  normalizeActor(actor), detailText);

  return true;
}

bool Database::updateTestResultCore(const core::TestResult &result) {
  const std::string computedFlag =
      core::TestResult::flagToString(result.evaluateFlag());
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
  sqlite3_bind_text(stmt.get(), 10, computedFlag.c_str(), -1,
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

bool Database::updateTestResultWithAudit(const core::TestResult &result,
                                         const std::string &user) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  auto existing = getTestResult(result.getId());
  if (!existing) {
    if (!hasError()) {
      setError("Ergebnis mit ID " + std::to_string(result.getId()) +
               " nicht gefunden");
    }
    return false;
  }

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  if (!updateTestResultCore(result)) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  const std::string computedFlag =
      core::TestResult::flagToString(result.evaluateFlag());

  std::ostringstream details;
  bool hasChanges = false;
  appendAuditChange(details, hasChanges, "Parameter",
                    existing->getTestParameter(), result.getTestParameter());
  appendAuditChange(details, hasChanges, "Wert", existing->getValue(),
                    result.getValue());
  appendAuditChange(details, hasChanges, "Einheit", existing->getUnit(),
                    result.getUnit());
  appendAuditChange(details, hasChanges, "Referenzbereich",
                    existing->getReferenceRange(),
                    result.getReferenceRange());
  appendAuditChange(details, hasChanges, "Referenz (low)",
                    std::to_string(existing->getReferenceLow()),
                    std::to_string(result.getReferenceLow()));
  appendAuditChange(details, hasChanges, "Referenz (high)",
                    std::to_string(existing->getReferenceHigh()),
                    std::to_string(result.getReferenceHigh()));
  appendAuditChange(details, hasChanges, "Status",
                    existing->getStatusString(), result.getStatusString());
  appendAuditChange(details, hasChanges, "Flag", existing->getFlagString(),
                    computedFlag);
  appendAuditChange(details, hasChanges, "Gemessen am",
                    std::to_string(existing->getMeasuredDate()),
                    std::to_string(result.getMeasuredDate()));
  appendAuditChange(details, hasChanges, "Gemessen von",
                    existing->getMeasuredBy(), result.getMeasuredBy());
  appendAuditChange(details, hasChanges, "Kommentar",
                    existing->getComment(), result.getComment());

  const std::string detailText =
      hasChanges ? details.str() : "Keine Änderungen";
  core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                         core::AuditEntry::EntityType::RESULT,
                         result.getResultId(), normalizeActor(user),
                         detailText);
  if (!logAudit(entry)) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  return true;
}

bool Database::exportValidatedResultsToCsv(const std::string &filePath,
                                           const std::string &user,
                                           std::optional<int> orderId) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  std::vector<std::unique_ptr<core::TestResult>> results;
  if (orderId.has_value()) {
    results = getTestResultsByOrderId(*orderId);
  } else {
    results = getAllTestResults();
  }

  if (hasError()) {
    return false;
  }

  std::vector<core::TestResult *> validated;
  validated.reserve(results.size());
  for (auto &result : results) {
    if (result->getStatus() == core::TestResult::Status::VALIDATED) {
      validated.push_back(result.get());
    }
  }

  if (validated.empty()) {
    setError("Keine validierten Ergebnisse zum Export");
    return false;
  }

  std::ofstream output(filePath);
  if (!output.is_open()) {
    setError("Exportdatei konnte nicht geschrieben werden");
    return false;
  }

  output << "result_id,order_id,test_parameter,value,unit,reference_low,"
            "reference_high,status,flag,measured_date,measured_by,comment\n";

  for (const auto *result : validated) {
    std::ostringstream low;
    low << result->getReferenceLow();
    std::ostringstream high;
    high << result->getReferenceHigh();

    output << escapeCsvField(result->getResultId()) << ","
           << result->getOrderId() << ","
           << escapeCsvField(result->getTestParameter()) << ","
           << escapeCsvField(result->getValue()) << ","
           << escapeCsvField(result->getUnit()) << "," << low.str() << ","
           << high.str() << "," << escapeCsvField(result->getStatusString())
           << "," << escapeCsvField(result->getFlagString()) << ","
           << static_cast<long long>(result->getMeasuredDate()) << ","
           << escapeCsvField(result->getMeasuredBy()) << ","
           << escapeCsvField(result->getComment()) << "\n";
  }

  if (!output) {
    setError("Fehler beim Schreiben der Exportdatei");
    return false;
  }

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  const std::string actor = normalizeActor(user);
  const std::string details = "Export: " + filePath + "; Anzahl: " +
                              std::to_string(validated.size());
  for (const auto *result : validated) {
    core::AuditEntry entry(core::AuditEntry::ActionType::EXPORT,
                           core::AuditEntry::EntityType::RESULT,
                           result->getResultId(), actor, details);
    if (!logAudit(entry)) {
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
      return false;
    }
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  return true;
}

bool Database::validateTestResult(const std::string &resultId,
                                  const std::string &user) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  auto result = getTestResultByResultId(resultId);
  if (!result) {
    return false;
  }

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  std::string oldStatus = result->getStatusString();
  result->setStatus(core::TestResult::Status::VALIDATED);

  if (!updateTestResultCore(*result)) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  core::AuditEntry entry(
      core::AuditEntry::ActionType::VALIDATE,
      core::AuditEntry::EntityType::RESULT, resultId, normalizeActor(user),
      "Status: " + oldStatus + " -> " + result->getStatusString());
  if (!logAudit(entry)) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  return true;
}

bool Database::deleteTestResult(int id, const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  auto existing = getTestResult(id);
  if (!existing) {
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

  std::ostringstream details;
  details << "Ergebnis-ID: " << existing->getResultId()
          << "; Parameter: " << existing->getTestParameter()
          << "; Status: " << existing->getStatusString();
  logResultAction(core::AuditEntry::ActionType::DELETE,
                  existing->getResultId(), normalizeActor(actor),
                  details.str());

  return true;
}

// ============================================================================
// Statistik-Operationen
// ============================================================================

Database::EntityStats Database::getSampleStats(const StatsFilter &filter) {
  EntityStats stats;
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return stats;
  }

  const auto filterSql = buildStatsFilter(filter, "registration_date");
  const std::string totalSQL =
      "SELECT COUNT(*) FROM samples" + filterSql.whereClause + ";";
  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, totalSQL.c_str(), -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des COUNT: " +
             std::string(sqlite3_errmsg(db_)));
    return stats;
  }
  auto stmt = makeStatement(rawStmt);
  if (!bindStatsFilter(stmt.get(), filterSql.bindings)) {
    setError("Fehler beim Binden der Statistik-Filter");
    return stats;
  }
  rc = sqlite3_step(stmt.get());
  if (rc == SQLITE_ROW) {
    stats.total = sqlite3_column_int(stmt.get(), 0);
  } else {
    setError("Fehler beim Abrufen der Statistik: " +
             std::string(sqlite3_errmsg(db_)));
    return stats;
  }

  const std::string statusSQL =
      "SELECT status, COUNT(*) FROM samples" + filterSql.whereClause +
      " GROUP BY status;";
  rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, statusSQL.c_str(), -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten der Status-Statistik: " +
             std::string(sqlite3_errmsg(db_)));
    return stats;
  }
  stmt = makeStatement(rawStmt);
  if (!bindStatsFilter(stmt.get(), filterSql.bindings)) {
    setError("Fehler beim Binden der Statistik-Filter");
    return stats;
  }
  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    StatusCount entry;
    entry.status = columnText(stmt.get(), 0);
    entry.count = sqlite3_column_int(stmt.get(), 1);
    stats.byStatus.push_back(std::move(entry));
  }
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Status-Statistiken: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return stats;
}

Database::EntityStats Database::getOrderStats(const StatsFilter &filter) {
  EntityStats stats;
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return stats;
  }

  const auto filterSql = buildStatsFilter(filter, "requested_date");
  const std::string totalSQL =
      "SELECT COUNT(*) FROM orders" + filterSql.whereClause + ";";
  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, totalSQL.c_str(), -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des COUNT: " +
             std::string(sqlite3_errmsg(db_)));
    return stats;
  }
  auto stmt = makeStatement(rawStmt);
  if (!bindStatsFilter(stmt.get(), filterSql.bindings)) {
    setError("Fehler beim Binden der Statistik-Filter");
    return stats;
  }
  rc = sqlite3_step(stmt.get());
  if (rc == SQLITE_ROW) {
    stats.total = sqlite3_column_int(stmt.get(), 0);
  } else {
    setError("Fehler beim Abrufen der Statistik: " +
             std::string(sqlite3_errmsg(db_)));
    return stats;
  }

  const std::string statusSQL =
      "SELECT status, COUNT(*) FROM orders" + filterSql.whereClause +
      " GROUP BY status;";
  rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, statusSQL.c_str(), -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten der Status-Statistik: " +
             std::string(sqlite3_errmsg(db_)));
    return stats;
  }
  stmt = makeStatement(rawStmt);
  if (!bindStatsFilter(stmt.get(), filterSql.bindings)) {
    setError("Fehler beim Binden der Statistik-Filter");
    return stats;
  }
  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    StatusCount entry;
    entry.status = columnText(stmt.get(), 0);
    entry.count = sqlite3_column_int(stmt.get(), 1);
    stats.byStatus.push_back(std::move(entry));
  }
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Status-Statistiken: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return stats;
}

Database::EntityStats Database::getResultStats(const StatsFilter &filter) {
  EntityStats stats;
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return stats;
  }

  const auto filterSql = buildStatsFilter(filter, "measured_date");
  const std::string totalSQL =
      "SELECT COUNT(*) FROM test_results" + filterSql.whereClause + ";";
  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, totalSQL.c_str(), -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des COUNT: " +
             std::string(sqlite3_errmsg(db_)));
    return stats;
  }
  auto stmt = makeStatement(rawStmt);
  if (!bindStatsFilter(stmt.get(), filterSql.bindings)) {
    setError("Fehler beim Binden der Statistik-Filter");
    return stats;
  }
  rc = sqlite3_step(stmt.get());
  if (rc == SQLITE_ROW) {
    stats.total = sqlite3_column_int(stmt.get(), 0);
  } else {
    setError("Fehler beim Abrufen der Statistik: " +
             std::string(sqlite3_errmsg(db_)));
    return stats;
  }

  const std::string statusSQL =
      "SELECT status, COUNT(*) FROM test_results" + filterSql.whereClause +
      " GROUP BY status;";
  rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, statusSQL.c_str(), -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten der Status-Statistik: " +
             std::string(sqlite3_errmsg(db_)));
    return stats;
  }
  stmt = makeStatement(rawStmt);
  if (!bindStatsFilter(stmt.get(), filterSql.bindings)) {
    setError("Fehler beim Binden der Statistik-Filter");
    return stats;
  }
  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    StatusCount entry;
    entry.status = columnText(stmt.get(), 0);
    entry.count = sqlite3_column_int(stmt.get(), 1);
    stats.byStatus.push_back(std::move(entry));
  }
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Status-Statistiken: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return stats;
}

bool Database::exportStatsReportToCsv(
    const std::string &filePath, const StatsFilter &sampleFilter,
    const StatsFilter &orderFilter, const StatsFilter &resultFilter,
    const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const auto sampleStats = getSampleStats(sampleFilter);
  if (hasError()) {
    return false;
  }
  const auto orderStats = getOrderStats(orderFilter);
  if (hasError()) {
    return false;
  }
  const auto resultStats = getResultStats(resultFilter);
  if (hasError()) {
    return false;
  }

  std::ofstream output(filePath);
  if (!output.is_open()) {
    setError("Exportdatei konnte nicht geschrieben werden");
    return false;
  }

  auto formatDate = [](const std::optional<std::time_t> &value) {
    if (!value.has_value()) {
      return std::string("none");
    }
    std::tm tm = *std::localtime(&value.value());
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d");
    return ss.str();
  };

  output << "# sample_filter status="
         << (sampleFilter.status.has_value() ? sampleFilter.status.value()
                                             : "any")
         << " from=" << formatDate(sampleFilter.fromDate)
         << " to=" << formatDate(sampleFilter.toDate) << "\n";
  output << "# order_filter status="
         << (orderFilter.status.has_value() ? orderFilter.status.value() : "any")
         << " from=" << formatDate(orderFilter.fromDate)
         << " to=" << formatDate(orderFilter.toDate) << "\n";
  output << "# result_filter status="
         << (resultFilter.status.has_value() ? resultFilter.status.value()
                                             : "any")
         << " from=" << formatDate(resultFilter.fromDate)
         << " to=" << formatDate(resultFilter.toDate) << "\n";

  output << "entity,status,count\n";

  auto countFor = [](const std::vector<StatusCount> &entries,
                     const std::string &status) {
    for (const auto &entry : entries) {
      if (entry.status == status) {
        return entry.count;
      }
    }
    return 0;
  };

  auto writeEntity = [&](const std::string &entity, int total,
                         const std::vector<std::string> &statuses,
                         const std::vector<StatusCount> &entries) {
    output << entity << ",TOTAL," << total << "\n";
    for (const auto &status : statuses) {
      output << entity << "," << escapeCsvField(status) << ","
             << countFor(entries, status) << "\n";
    }
  };

  const std::vector<std::string> sampleStatuses = {
      core::Sample::statusToString(core::Sample::Status::REGISTERED),
      core::Sample::statusToString(core::Sample::Status::IN_ANALYSIS),
      core::Sample::statusToString(core::Sample::Status::ANALYZED),
      core::Sample::statusToString(core::Sample::Status::VALIDATED),
      core::Sample::statusToString(core::Sample::Status::ARCHIVED)};
  const std::vector<std::string> orderStatuses = {
      core::Order::statusToString(core::Order::Status::REQUESTED),
      core::Order::statusToString(core::Order::Status::IN_PROGRESS),
      core::Order::statusToString(core::Order::Status::COMPLETED),
      core::Order::statusToString(core::Order::Status::VALIDATED),
      core::Order::statusToString(core::Order::Status::CANCELLED)};
  const std::vector<std::string> resultStatuses = {
      core::TestResult::statusToString(core::TestResult::Status::PENDING),
      core::TestResult::statusToString(core::TestResult::Status::ENTERED),
      core::TestResult::statusToString(core::TestResult::Status::VALIDATED),
      core::TestResult::statusToString(core::TestResult::Status::REJECTED),
      core::TestResult::statusToString(core::TestResult::Status::REPEATED)};

  writeEntity("samples", sampleStats.total, sampleStatuses,
              sampleStats.byStatus);
  writeEntity("orders", orderStats.total, orderStatuses, orderStats.byStatus);
  writeEntity("results", resultStats.total, resultStatuses,
              resultStats.byStatus);

  if (!output) {
    setError("Fehler beim Schreiben der Exportdatei");
    return false;
  }

  std::ostringstream details;
  details << "Export stats report: " << filePath
          << "; sample_status="
          << (sampleFilter.status.has_value() ? sampleFilter.status.value()
                                              : "any")
          << "; sample_from=" << formatDate(sampleFilter.fromDate)
          << "; sample_to=" << formatDate(sampleFilter.toDate)
          << "; order_status="
          << (orderFilter.status.has_value() ? orderFilter.status.value()
                                             : "any")
          << "; order_from=" << formatDate(orderFilter.fromDate)
          << "; order_to=" << formatDate(orderFilter.toDate)
          << "; result_status="
          << (resultFilter.status.has_value() ? resultFilter.status.value()
                                              : "any")
          << "; result_from=" << formatDate(resultFilter.fromDate)
          << "; result_to=" << formatDate(resultFilter.toDate);

  core::AuditEntry auditEntry(core::AuditEntry::ActionType::UPDATE,
                              core::AuditEntry::EntityType::SYSTEM,
                              "stats_report", normalizeActor(actor),
                              details.str());
  if (!logAudit(auditEntry)) {
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

std::vector<std::unique_ptr<core::AuditEntry>>
Database::getAuditLogFiltered(const AuditLogFilter &filter) {
  std::vector<std::unique_ptr<core::AuditEntry>> entries;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return entries;
  }

  const bool hasFilter =
      (filter.user && !filter.user->empty()) || filter.action.has_value() ||
      filter.entity.has_value() ||
      (filter.entityId && !filter.entityId->empty()) ||
      filter.fromTime.has_value() || filter.toTime.has_value();

  int limit = filter.limit;
  if (limit <= 0) {
    limit = 100;
  }

  if (!hasFilter) {
    return getAuditLog(limit);
  }

  std::ostringstream sql;
  sql << "SELECT id, action, entity, entity_id, user, timestamp, details "
         "FROM audit_log WHERE 1=1";

  if (filter.user && !filter.user->empty()) {
    sql << " AND user = ?";
  }
  if (filter.action.has_value()) {
    sql << " AND action = ?";
  }
  if (filter.entity.has_value()) {
    sql << " AND entity = ?";
  }
  if (filter.entityId && !filter.entityId->empty()) {
    sql << " AND entity_id = ?";
  }
  if (filter.fromTime.has_value()) {
    sql << " AND timestamp >= ?";
  }
  if (filter.toTime.has_value()) {
    sql << " AND timestamp <= ?";
  }

  sql << " ORDER BY timestamp DESC LIMIT ?;";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return entries;
  }

  auto stmt = makeStatement(rawStmt);
  int index = 1;

  if (filter.user && !filter.user->empty()) {
    sqlite3_bind_text(stmt.get(), index++, filter.user->c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  if (filter.action.has_value()) {
    const std::string action =
        core::AuditEntry::actionToString(filter.action.value());
    sqlite3_bind_text(stmt.get(), index++, action.c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  if (filter.entity.has_value()) {
    const std::string entity =
        core::AuditEntry::entityToString(filter.entity.value());
    sqlite3_bind_text(stmt.get(), index++, entity.c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  if (filter.entityId && !filter.entityId->empty()) {
    sqlite3_bind_text(stmt.get(), index++, filter.entityId->c_str(), -1,
                      SQLITE_TRANSIENT);
  }
  if (filter.fromTime.has_value()) {
    sqlite3_bind_int64(stmt.get(), index++,
                       static_cast<sqlite3_int64>(filter.fromTime.value()));
  }
  if (filter.toTime.has_value()) {
    sqlite3_bind_int64(stmt.get(), index++,
                       static_cast<sqlite3_int64>(filter.toTime.value()));
  }

  sqlite3_bind_int(stmt.get(), index++, limit);

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

bool Database::exportAuditLogToCsv(const std::string &filePath,
                                   const AuditLogFilter &filter,
                                   const std::string &actor,
                                   int &exportedCount) {
  clearError();
  exportedCount = 0;

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  auto entries = getAuditLogFiltered(filter);
  if (hasError()) {
    return false;
  }
  if (entries.empty()) {
    setError("Keine Audit-Einträge zum Export");
    return false;
  }

  std::ofstream output(filePath);
  if (!output.is_open()) {
    setError("Exportdatei konnte nicht geschrieben werden");
    return false;
  }

  output << "id,action,entity,entity_id,user,timestamp,details\n";

  for (const auto &entry : entries) {
    output << entry->getId() << ","
           << escapeCsvField(entry->getActionString()) << ","
           << escapeCsvField(entry->getEntityString()) << ","
           << escapeCsvField(entry->getEntityId()) << ","
           << escapeCsvField(entry->getUser()) << ","
           << static_cast<long long>(entry->getTimestamp()) << ","
           << escapeCsvField(entry->getDetails()) << "\n";
  }

  if (!output) {
    setError("Fehler beim Schreiben der Exportdatei");
    return false;
  }

  exportedCount = static_cast<int>(entries.size());

  const std::string details =
      "Export: " + filePath + "; Anzahl: " + std::to_string(exportedCount);
  core::AuditEntry auditEntry(core::AuditEntry::ActionType::UPDATE,
                              core::AuditEntry::EntityType::SYSTEM,
                              "audit_log", normalizeActor(actor), details);
  if (!logAudit(auditEntry)) {
    return false;
  }

  return true;
}

std::vector<std::unique_ptr<core::AuditEntry>>
Database::getDiagnosticsLogs(const DiagnosticsFilter &filter) {
  AuditLogFilter auditFilter;
  auditFilter.entity = filter.component;
  auditFilter.fromTime = filter.fromTime;
  auditFilter.toTime = filter.toTime;
  auditFilter.limit = filter.limit;
  return getAuditLogFiltered(auditFilter);
}

bool Database::exportDiagnosticsLogsToCsv(const std::string &filePath,
                                          const DiagnosticsFilter &filter,
                                          const std::string &actor,
                                          int &exportedCount) {
  AuditLogFilter auditFilter;
  auditFilter.entity = filter.component;
  auditFilter.fromTime = filter.fromTime;
  auditFilter.toTime = filter.toTime;
  auditFilter.limit = filter.limit;
  return exportAuditLogToCsv(filePath, auditFilter, actor, exportedCount);
}

int Database::getRetentionDays() {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return 180;
  }

  const char *selectSQL =
      "SELECT value FROM retention_settings WHERE key = ? LIMIT 1;";
  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return 180;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_text(stmt.get(), 1, "audit_log_days", -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());
  if (rc == SQLITE_ROW) {
    int value = sqlite3_column_int(stmt.get(), 0);
    return std::max(value, 180);
  }
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Laden der Retention-Konfiguration: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return 180;
}

bool Database::setRetentionDays(int days) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  int enforcedDays = std::max(days, 180);

  const char *upsertSQL = R"(
        INSERT INTO retention_settings (key, value) VALUES (?, ?)
        ON CONFLICT(key) DO UPDATE SET value = excluded.value;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, upsertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten der Retention-Konfiguration: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_text(stmt.get(), 1, "audit_log_days", -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 2, enforcedDays);

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Speichern der Retention-Konfiguration: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  return true;
}

bool Database::applyAuditRetention(const std::string &actor,
                                   int &purgedCount) {
  clearError();

  purgedCount = 0;

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  int retentionDays = getRetentionDays();
  if (!getLastError().empty()) {
    return false;
  }

  const std::time_t now = std::time(nullptr);
  const std::time_t cutoff =
      now - static_cast<std::time_t>(retentionDays * 24 * 60 * 60);

  const char *deleteSQL =
      "DELETE FROM audit_log WHERE timestamp < ?;";
  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des DELETE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_int64(stmt.get(), 1, static_cast<sqlite3_int64>(cutoff));

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Anwenden der Retention-Regeln: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  purgedCount = sqlite3_changes(db_);
  if (purgedCount > 0) {
    std::ostringstream details;
    details << "Retention purge: " << purgedCount
            << " audit entries older than " << cutoff << " (days="
            << retentionDays << ")";

    core::AuditEntry entry(core::AuditEntry::ActionType::DELETE,
                           core::AuditEntry::EntityType::SYSTEM, "audit_log",
                           normalizeActor(actor), details.str());
    entry.setTimestamp(now);
    if (!logAudit(entry)) {
      return false;
    }
  }

  return true;
}

// Audit-Hilfsmethoden
void Database::logSampleAction(core::AuditEntry::ActionType action,
                               const std::string &sampleId,
                               const std::string &user,
                               const std::string &details) {
  const std::string actor = normalizeActor(user);
  core::AuditEntry entry(action, core::AuditEntry::EntityType::SAMPLE, sampleId,
                         actor, details);
  (void)logAudit(entry);
}

void Database::logOrderAction(core::AuditEntry::ActionType action,
                              const std::string &orderId,
                              const std::string &user,
                              const std::string &details) {
  const std::string actor = normalizeActor(user);
  core::AuditEntry entry(action, core::AuditEntry::EntityType::ORDER, orderId,
                         actor, details);
  (void)logAudit(entry);
}

void Database::logResultAction(core::AuditEntry::ActionType action,
                               const std::string &resultId,
                               const std::string &user,
                               const std::string &details) {
  const std::string actor = normalizeActor(user);
  core::AuditEntry entry(action, core::AuditEntry::EntityType::RESULT, resultId,
                         actor, details);
  (void)logAudit(entry);
}

void Database::logUserAction(core::AuditEntry::ActionType action,
                             const std::string &username,
                             const std::string &user,
                             const std::string &details) {
  const std::string actor = normalizeActor(user);
  core::AuditEntry entry(action, core::AuditEntry::EntityType::USER, username,
                         actor, details);
  (void)logAudit(entry);
}

void Database::logRoleAction(core::AuditEntry::ActionType action,
                             const std::string &roleName,
                             const std::string &user,
                             const std::string &details) {
  const std::string actor = normalizeActor(user);
  core::AuditEntry entry(action, core::AuditEntry::EntityType::ROLE, roleName,
                         actor, details);
  (void)logAudit(entry);
}

bool Database::logSupportAccess(core::AuditEntry::EntityType entity,
                                const std::string &entityId,
                                const std::string &user,
                                const std::string &details) {
  const std::string actor = normalizeActor(user);
  const std::string info = details.empty() ? "support_access" : details;
  core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE, entity, entityId,
                         actor, info);
  return logAudit(entry);
}

bool Database::logResultRetryImport(const std::vector<std::string> &resultIds,
                                    const std::string &user,
                                    const std::string &filePath) {
  if (resultIds.empty()) {
    return true;
  }

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  const std::string actor = user.empty() ? "system" : user;
  const std::string details = "Retry-Import: " + filePath + "; Anzahl: " +
                              std::to_string(resultIds.size());

  for (const auto &resultId : resultIds) {
    core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                           core::AuditEntry::EntityType::RESULT, resultId,
                           actor, details);
    if (!logAudit(entry)) {
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
      return false;
    }
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  return true;
}

// ============================================================================
// User-Operationen
// ============================================================================

bool Database::createUser(const core::User &user, const std::string &actor) {
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

  std::ostringstream details;
  details << "Rolle: " << user.getRoleString()
          << "; Status: " << formatActive(user.isActive());
  if (!user.getFullName().empty()) {
    details << "; Name: " << user.getFullName();
  }
  if (!user.getEmail().empty()) {
    details << "; Email: " << user.getEmail();
  }
  logUserAction(core::AuditEntry::ActionType::CREATE, user.getUsername(),
                normalizeActor(actor), details.str());

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

bool Database::updateUser(const core::User &user, const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  auto existing = getUser(user.getId());
  if (!existing) {
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

  std::ostringstream details;
  bool hasChanges = false;
  appendAuditChange(details, hasChanges, "Benutzername",
                    existing->getUsername(), user.getUsername());
  appendAuditChange(details, hasChanges, "Rolle",
                    existing->getRoleString(), user.getRoleString());
  appendAuditChange(details, hasChanges, "Status",
                    formatActive(existing->isActive()),
                    formatActive(user.isActive()));
  appendAuditChange(details, hasChanges, "Letzter Login",
                    std::to_string(existing->getLastLogin()),
                    std::to_string(user.getLastLogin()));
  appendAuditChange(details, hasChanges, "Name",
                    existing->getFullName(), user.getFullName());
  appendAuditChange(details, hasChanges, "Email",
                    existing->getEmail(), user.getEmail());

  const std::string detailText =
      hasChanges ? details.str() : "Keine Änderungen";
  logUserAction(core::AuditEntry::ActionType::UPDATE, user.getUsername(),
                normalizeActor(actor), detailText);

  return true;
}

bool Database::deleteUser(int id, const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  auto existing = getUser(id);
  if (!existing) {
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

  std::ostringstream details;
  details << "Benutzername: " << existing->getUsername()
          << "; Rolle: " << existing->getRoleString()
          << "; Status: " << formatActive(existing->isActive());
  logUserAction(core::AuditEntry::ActionType::DELETE, existing->getUsername(),
                normalizeActor(actor), details.str());

  return true;
}

// ============================================================================
// Rollen & Berechtigungen
// ============================================================================

bool Database::createRole(const std::string &name,
                          const std::vector<std::string> &permissions,
                          const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (name.empty()) {
    setError("Rollenname darf nicht leer sein");
    return false;
  }

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  const char *insertRoleSQL = R"(
        INSERT INTO roles (name, description) VALUES (?, ?);
    )";
  sqlite3_stmt *roleStmtRaw = nullptr;
  rc = sqlite3_prepare_v2(db_, insertRoleSQL, -1, &roleStmtRaw, nullptr);
  if (rc != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Fehler beim Vorbereiten des Rollen-INSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto roleStmt = makeStatement(roleStmtRaw);

  sqlite3_bind_text(roleStmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(roleStmt.get(), 2, "", -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(roleStmt.get());
  if (rc != SQLITE_DONE) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Fehler beim Einfügen der Rolle: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  const char *insertPermSQL = R"(
        INSERT OR IGNORE INTO role_permissions (role_name, permission)
        VALUES (?, ?);
    )";
  sqlite3_stmt *permStmtRaw = nullptr;
  rc = sqlite3_prepare_v2(db_, insertPermSQL, -1, &permStmtRaw, nullptr);
  if (rc != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Fehler beim Vorbereiten des Berechtigungs-INSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto permStmt = makeStatement(permStmtRaw);

  for (const auto &perm : permissions) {
    if (perm.empty()) {
      continue;
    }
    sqlite3_reset(permStmt.get());
    sqlite3_clear_bindings(permStmt.get());
    sqlite3_bind_text(permStmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(permStmt.get(), 2, perm.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(permStmt.get());
    if (rc != SQLITE_DONE) {
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
      setError("Fehler beim Einfügen der Berechtigung: " +
               std::string(sqlite3_errmsg(db_)));
      return false;
    }
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  std::ostringstream details;
  details << "Berechtigungen: [" << joinList(permissions) << "]";
  logRoleAction(core::AuditEntry::ActionType::CREATE, name,
                normalizeActor(actor), details.str());

  return true;
}

bool Database::updateRole(const std::string &name,
                          const std::vector<std::string> &permissions,
                          const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (name.empty()) {
    setError("Rollenname darf nicht leer sein");
    return false;
  }

  const char *existsSQL = "SELECT 1 FROM roles WHERE name = ? LIMIT 1;";
  sqlite3_stmt *existsStmtRaw = nullptr;
  int rc = sqlite3_prepare_v2(db_, existsSQL, -1, &existsStmtRaw, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Prüfen der Rolle: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto existsStmt = makeStatement(existsStmtRaw);
  sqlite3_bind_text(existsStmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(existsStmt.get());
  if (rc == SQLITE_DONE) {
    setError("Rolle '" + name + "' nicht gefunden");
    return false;
  }
  if (rc != SQLITE_ROW) {
    setError("Fehler beim Prüfen der Rolle: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto existingPerms = getRolePermissions(name);
  if (hasError()) {
    return false;
  }

  char *errMsg = nullptr;
  rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                    &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  const char *deletePermSQL =
      "DELETE FROM role_permissions WHERE role_name = ?;";
  sqlite3_stmt *deleteStmtRaw = nullptr;
  rc = sqlite3_prepare_v2(db_, deletePermSQL, -1, &deleteStmtRaw, nullptr);
  if (rc != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Fehler beim Vorbereiten des DELETE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto deleteStmt = makeStatement(deleteStmtRaw);
  sqlite3_bind_text(deleteStmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(deleteStmt.get());
  if (rc != SQLITE_DONE) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Fehler beim Löschen der Berechtigungen: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  const char *insertPermSQL = R"(
        INSERT OR IGNORE INTO role_permissions (role_name, permission)
        VALUES (?, ?);
    )";
  sqlite3_stmt *permStmtRaw = nullptr;
  rc = sqlite3_prepare_v2(db_, insertPermSQL, -1, &permStmtRaw, nullptr);
  if (rc != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Fehler beim Vorbereiten des Berechtigungs-INSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto permStmt = makeStatement(permStmtRaw);

  for (const auto &perm : permissions) {
    if (perm.empty()) {
      continue;
    }
    sqlite3_reset(permStmt.get());
    sqlite3_clear_bindings(permStmt.get());
    sqlite3_bind_text(permStmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(permStmt.get(), 2, perm.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(permStmt.get());
    if (rc != SQLITE_DONE) {
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
      setError("Fehler beim Einfügen der Berechtigung: " +
               std::string(sqlite3_errmsg(db_)));
      return false;
    }
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  std::ostringstream details;
  details << "Berechtigungen: [" << joinList(existingPerms) << "] -> ["
          << joinList(permissions) << "]";
  logRoleAction(core::AuditEntry::ActionType::UPDATE, name,
                normalizeActor(actor), details.str());

  return true;
}

std::vector<std::string> Database::getRolePermissions(const std::string &name) {
  std::vector<std::string> permissions;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return permissions;
  }

  const char *selectSQL = R"(
        SELECT permission
        FROM role_permissions
        WHERE role_name = ?
        ORDER BY permission;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return permissions;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_text(stmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);

  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    const char *perm = reinterpret_cast<const char *>(sqlite3_column_text(
        stmt.get(), 0));
    permissions.emplace_back(perm ? perm : "");
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Berechtigungen: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return permissions;
}

std::vector<std::string> Database::getAllRoles() {
  std::vector<std::string> roles;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return roles;
  }

  const char *selectSQL = R"(
        SELECT name FROM roles ORDER BY name;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return roles;
  }

  auto stmt = makeStatement(rawStmt);
  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    const char *name =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), 0));
    roles.emplace_back(name ? name : "");
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Rollen: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return roles;
}

bool Database::assignUserRole(int userId, const std::string &roleName,
                              const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (roleName.empty()) {
    setError("Rollenname darf nicht leer sein");
    return false;
  }

  const char *existsSQL = "SELECT 1 FROM roles WHERE name = ? LIMIT 1;";
  sqlite3_stmt *existsStmtRaw = nullptr;
  int rc = sqlite3_prepare_v2(db_, existsSQL, -1, &existsStmtRaw, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Prüfen der Rolle: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto existsStmt = makeStatement(existsStmtRaw);
  sqlite3_bind_text(existsStmt.get(), 1, roleName.c_str(), -1,
                    SQLITE_TRANSIENT);
  rc = sqlite3_step(existsStmt.get());
  if (rc == SQLITE_DONE) {
    setError("Rolle '" + roleName + "' nicht gefunden");
    return false;
  }
  if (rc != SQLITE_ROW) {
    setError("Fehler beim Prüfen der Rolle: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto existing = getUser(userId);
  if (!existing) {
    return false;
  }

  const char *updateSQL = "UPDATE users SET role = ? WHERE id = ?;";
  sqlite3_stmt *updateStmtRaw = nullptr;
  rc = sqlite3_prepare_v2(db_, updateSQL, -1, &updateStmtRaw, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des UPDATE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto updateStmt = makeStatement(updateStmtRaw);

  sqlite3_bind_text(updateStmt.get(), 1, roleName.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(updateStmt.get(), 2, userId);

  rc = sqlite3_step(updateStmt.get());
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Zuweisen der Rolle: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  if (sqlite3_changes(db_) == 0) {
    setError("Benutzer mit ID " + std::to_string(userId) + " nicht gefunden");
    return false;
  }

  logUserAction(core::AuditEntry::ActionType::UPDATE,
                existing->getUsername(), normalizeActor(actor),
                "Rolle: " + existing->getRoleString() + " -> " + roleName);

  return true;
}

// ============================================================================
// Auth-Konfiguration, LDAP und MFA
// ============================================================================

bool Database::setAuthConfig(const std::string &key, const std::string &value) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *upsertSQL = R"(
        INSERT INTO auth_config (key, value) VALUES (?, ?)
        ON CONFLICT(key) DO UPDATE SET value = excluded.value;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, upsertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten der Auth-Konfiguration: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_text(stmt.get(), 1, key.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, value.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Speichern der Auth-Konfiguration: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  return true;
}

std::optional<std::string> Database::getAuthConfig(const std::string &key) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return std::nullopt;
  }

  const char *selectSQL =
      "SELECT value FROM auth_config WHERE key = ? LIMIT 1;";
  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return std::nullopt;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_text(stmt.get(), 1, key.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());
  if (rc == SQLITE_DONE) {
    return std::nullopt;
  }
  if (rc != SQLITE_ROW) {
    setError("Fehler beim Laden der Auth-Konfiguration: " +
             std::string(sqlite3_errmsg(db_)));
    return std::nullopt;
  }

  return columnText(stmt.get(), 0);
}

bool Database::setLdapEnabled(bool enabled) {
  return setAuthConfig("ldap_enabled", enabled ? "1" : "0");
}

bool Database::isLdapEnabled() {
  auto value = getAuthConfig("ldap_enabled");
  if (!value.has_value()) {
    clearError();
    return false;
  }
  return value.value() == "1";
}

bool Database::upsertApiKey(const std::string &key, bool active) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (key.empty()) {
    setError("API-Schlüssel darf nicht leer sein");
    return false;
  }

  const char *upsertSQL = R"(
        INSERT INTO api_keys (key, active, created_date)
        VALUES (?, ?, ?)
        ON CONFLICT(key) DO UPDATE SET active = excluded.active;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, upsertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des API-Key-UPSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_text(stmt.get(), 1, key.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 2, active ? 1 : 0);
  sqlite3_bind_int64(stmt.get(), 3,
                     static_cast<sqlite3_int64>(std::time(nullptr)));

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Speichern des API-Schlüssels: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  return true;
}

bool Database::isApiKeyValid(const std::string &key) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (key.empty()) {
    return false;
  }

  const char *selectSQL =
      "SELECT active FROM api_keys WHERE key = ? LIMIT 1;";
  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_text(stmt.get(), 1, key.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());
  if (rc == SQLITE_ROW) {
    const int active = sqlite3_column_int(stmt.get(), 0);
    return active == 1;
  }
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Laden des API-Schlüssels: " +
             std::string(sqlite3_errmsg(db_)));
  } else {
    clearError();
  }
  return false;
}

bool Database::upsertLdapUser(const std::string &username,
                              const std::string &passwordHash, bool active,
                              bool mfaRequired,
                              const std::string &mfaSecret) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (username.empty() || passwordHash.empty()) {
    setError("LDAP-Benutzername und Passwort-Hash sind erforderlich");
    return false;
  }

  const char *upsertSQL = R"(
        INSERT INTO ldap_directory (username, password_hash, active, mfa_required, mfa_secret)
        VALUES (?, ?, ?, ?, ?)
        ON CONFLICT(username) DO UPDATE SET
            password_hash = excluded.password_hash,
            active = excluded.active,
            mfa_required = excluded.mfa_required,
            mfa_secret = excluded.mfa_secret;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, upsertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des LDAP-UPSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 3, active ? 1 : 0);
  sqlite3_bind_int(stmt.get(), 4, mfaRequired ? 1 : 0);
  sqlite3_bind_text(stmt.get(), 5, mfaSecret.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Speichern des LDAP-Benutzers: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  return true;
}

bool Database::setUserMfaRequirement(const std::string &username,
                                     bool required,
                                     const std::string &mfaSecret) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (username.empty()) {
    setError("Benutzername darf nicht leer sein");
    return false;
  }

  const char *upsertSQL = R"(
        INSERT INTO user_mfa (username, mfa_required, mfa_secret)
        VALUES (?, ?, ?)
        ON CONFLICT(username) DO UPDATE SET
            mfa_required = excluded.mfa_required,
            mfa_secret = excluded.mfa_secret;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, upsertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des MFA-UPSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 2, required ? 1 : 0);
  sqlite3_bind_text(stmt.get(), 3, mfaSecret.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Speichern der MFA-Anforderung: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  return true;
}

bool Database::setRoleMfaRequirement(const std::string &roleName,
                                     bool required) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (roleName.empty()) {
    setError("Rollenname darf nicht leer sein");
    return false;
  }

  const char *upsertSQL = R"(
        INSERT INTO role_mfa (role_name, mfa_required)
        VALUES (?, ?)
        ON CONFLICT(role_name) DO UPDATE SET
            mfa_required = excluded.mfa_required;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, upsertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des Rollen-MFA-UPSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_text(stmt.get(), 1, roleName.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 2, required ? 1 : 0);

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Speichern der Rollen-MFA-Anforderung: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  return true;
}

bool Database::isMfaRequiredForUser(const std::string &username,
                                    const std::string &roleName) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *userSQL = R"(
        SELECT mfa_required
        FROM user_mfa
        WHERE username = ?
        LIMIT 1;
    )";
  sqlite3_stmt *userStmtRaw = nullptr;
  int rc = sqlite3_prepare_v2(db_, userSQL, -1, &userStmtRaw, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des MFA-SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto userStmt = makeStatement(userStmtRaw);
  sqlite3_bind_text(userStmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(userStmt.get());
  if (rc == SQLITE_ROW) {
    return sqlite3_column_int(userStmt.get(), 0) != 0;
  }
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Laden der Benutzer-MFA-Anforderung: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  const char *roleSQL = R"(
        SELECT mfa_required
        FROM role_mfa
        WHERE role_name = ?
        LIMIT 1;
    )";
  sqlite3_stmt *roleStmtRaw = nullptr;
  rc = sqlite3_prepare_v2(db_, roleSQL, -1, &roleStmtRaw, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des Rollen-MFA-SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto roleStmt = makeStatement(roleStmtRaw);
  sqlite3_bind_text(roleStmt.get(), 1, roleName.c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(roleStmt.get());
  if (rc == SQLITE_ROW) {
    return sqlite3_column_int(roleStmt.get(), 0) != 0;
  }
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Laden der Rollen-MFA-Anforderung: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  clearError();
  return false;
}

bool Database::verifyUserMfa(const std::string &username,
                             const std::string &code) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  const char *selectSQL = R"(
        SELECT mfa_secret
        FROM user_mfa
        WHERE username = ?
        LIMIT 1;
    )";
  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des MFA-SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }
  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());
  if (rc == SQLITE_DONE) {
    setError("Keine MFA-Konfiguration für Benutzer '" + username + "'");
    return false;
  }
  if (rc != SQLITE_ROW) {
    setError("Fehler beim Laden des MFA-Secrets: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  const std::string secret = columnText(stmt.get(), 0);
  if (!verifyMfaCode(secret, code)) {
    setError("Ungültiger oder abgelaufener MFA-Code");
    return false;
  }

  return true;
}

// ============================================================================
// Sitzungsverfolgung
// ============================================================================

std::optional<int> Database::getActiveSessionId(int userId) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return std::nullopt;
  }

  const char *selectSQL = R"(
        SELECT id
        FROM sessions
        WHERE user_id = ? AND logout_ts IS NULL
        ORDER BY login_ts DESC
        LIMIT 1;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des Session-SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return std::nullopt;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_int(stmt.get(), 1, userId);

  rc = sqlite3_step(stmt.get());
  if (rc == SQLITE_DONE) {
    clearError();
    return std::nullopt;
  }
  if (rc != SQLITE_ROW) {
    setError("Fehler beim Laden der aktiven Sitzung: " +
             std::string(sqlite3_errmsg(db_)));
    return std::nullopt;
  }

  return sqlite3_column_int(stmt.get(), 0);
}

int Database::getActiveSessionCount(int userId) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return 0;
  }

  const char *countSQL = R"(
        SELECT COUNT(*)
        FROM sessions
        WHERE user_id = ? AND logout_ts IS NULL;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, countSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des Session-COUNT: " +
             std::string(sqlite3_errmsg(db_)));
    return 0;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_int(stmt.get(), 1, userId);

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_ROW) {
    setError("Fehler beim Zählen aktiver Sitzungen: " +
             std::string(sqlite3_errmsg(db_)));
    return 0;
  }

  return sqlite3_column_int(stmt.get(), 0);
}

int Database::getSessionCount(int userId) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return 0;
  }

  const char *countSQL = R"(
        SELECT COUNT(*)
        FROM sessions
        WHERE user_id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, countSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des Session-COUNT: " +
             std::string(sqlite3_errmsg(db_)));
    return 0;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_int(stmt.get(), 1, userId);

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_ROW) {
    setError("Fehler beim Zählen der Sitzungen: " +
             std::string(sqlite3_errmsg(db_)));
    return 0;
  }

  return sqlite3_column_int(stmt.get(), 0);
}

bool Database::hasActiveSession(int userId) {
  return getActiveSessionCount(userId) > 0;
}

bool Database::endSession(int userId, const std::string &username,
                          const std::string &reason) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (userId <= 0) {
    setError("Ungültige Benutzer-ID für Sitzungsende");
    return false;
  }

  const std::time_t now = std::time(nullptr);
  const char *updateSQL = R"(
        UPDATE sessions
        SET logout_ts = ?
        WHERE user_id = ? AND logout_ts IS NULL;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, updateSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des Session-UPDATE: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_int64(stmt.get(), 1, static_cast<sqlite3_int64>(now));
  sqlite3_bind_int(stmt.get(), 2, userId);

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Beenden der Sitzung: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  const int closedCount = sqlite3_changes(db_);
  const std::string details =
      closedCount > 0 ? "Sitzung beendet" : "Keine aktive Sitzung gefunden";

  // Audit-Logging soll keine erfolgreichen Operationen in Fehler verwandeln.
  const std::string prevError = lastError_;
  logUserAction(core::AuditEntry::ActionType::LOGOUT, username, username,
                details + (reason.empty() ? "" : " (" + reason + ")"));
  if (!prevError.empty()) {
    lastError_ = prevError;
  } else if (!lastError_.empty()) {
    clearError();
  }

  return true;
}

bool Database::startSession(int userId, const std::string &username,
                            AuthMethod method, const std::string &details) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (userId <= 0 || username.empty()) {
    setError("Ungültige Benutzerdaten für Sitzungsstart");
    return false;
  }

  // Bestehende aktive Sitzungen schließen, ohne den Aufrufer zu blockieren.
  (void)endSession(userId, username, "relogin");
  clearError();

  const std::time_t now = std::time(nullptr);
  const std::string methodStr = method == AuthMethod::LDAP ? "LDAP" : "Lokal";

  const char *insertSQL = R"(
        INSERT INTO sessions (user_id, username, method, login_ts, details)
        VALUES (?, ?, ?, ?, ?);
    )";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des Session-INSERT: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_int(stmt.get(), 1, userId);
  sqlite3_bind_text(stmt.get(), 2, username.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, methodStr.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 4, static_cast<sqlite3_int64>(now));
  sqlite3_bind_text(stmt.get(), 5, details.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Starten der Sitzung: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  const int sessionId = static_cast<int>(sqlite3_last_insert_rowid(db_));
  const std::string auditDetails =
      "Login erfolgreich (Methode: " + methodStr +
      ", Session: " + std::to_string(sessionId) + ")";

  const std::string prevError = lastError_;
  logUserAction(core::AuditEntry::ActionType::LOGIN, username, username,
                auditDetails);
  if (!prevError.empty()) {
    lastError_ = prevError;
  } else if (!lastError_.empty()) {
    clearError();
  }

  return true;
}

Database::AuthResult Database::authenticatePrimary(const std::string &username,
                                                   const std::string &password) {
  clearError();
  AuthResult result;

  auto localUser = getUserByUsername(username);
  const bool ldapEnabled = isLdapEnabled();
  const std::string actor = username.empty() ? "unbekannt" : username;

  // LDAP-Pfad zuerst prüfen, wenn aktiviert und ein LDAP-Benutzer existiert.
  if (ldapEnabled) {
    const char *ldapSQL = R"(
          SELECT password_hash, active, mfa_required, mfa_secret
          FROM ldap_directory
          WHERE username = ?
          LIMIT 1;
      )";
    sqlite3_stmt *ldapStmtRaw = nullptr;
    int rc = sqlite3_prepare_v2(db_, ldapSQL, -1, &ldapStmtRaw, nullptr);
    if (rc == SQLITE_OK) {
      auto ldapStmt = makeStatement(ldapStmtRaw);
      sqlite3_bind_text(ldapStmt.get(), 1, username.c_str(), -1,
                        SQLITE_TRANSIENT);
      rc = sqlite3_step(ldapStmt.get());
      if (rc == SQLITE_ROW) {
        const std::string storedHash = columnText(ldapStmt.get(), 0);
        const bool active = sqlite3_column_int(ldapStmt.get(), 1) != 0;
        const bool ldapMfaRequired = sqlite3_column_int(ldapStmt.get(), 2) != 0;
        const std::string ldapSecret = columnText(ldapStmt.get(), 3);

        result.method = AuthMethod::LDAP;

        if (!active) {
          const std::string msg = "LDAP-Benutzer ist deaktiviert";
          setError(msg);
          logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                        "Login fehlgeschlagen (LDAP, deaktiviert)");
          setError(msg);
          return result;
        }

        if (core::User::hashPassword(password) != storedHash) {
          const std::string msg = "Ungültiger Benutzername oder Passwort";
          setError(msg);
          logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                        "Login fehlgeschlagen (LDAP, falsches Passwort)");
          setError(msg);
          return result;
        }

        // Sicherstellen, dass es einen lokalen Benutzer für Rollen/ACL gibt.
        if (!localUser) {
          core::User newUser(username, core::User::hashPassword(password),
                             core::User::Role::OPERATOR);
          newUser.setRoleName("Operator");
          (void)createUser(newUser, actor);
          localUser = getUserByUsername(username);
        }

        if (!localUser) {
          const std::string msg =
              "LDAP-Anmeldung erfolgreich, aber lokaler Benutzer fehlt";
          setError(msg);
          logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                        "Login fehlgeschlagen (LDAP, lokaler Benutzer fehlt)");
          setError(msg);
          return result;
        }

        // LDAP-MFA-Vorgaben in die lokale MFA-Konfiguration spiegeln.
        if (ldapMfaRequired && !ldapSecret.empty()) {
          (void)setUserMfaRequirement(username, true, ldapSecret);
        }

        result.user = std::move(localUser);
        result.requiresMfa =
            ldapMfaRequired || isMfaRequiredForUser(username, result.user->getRoleString());
        result.mfaSecret = ldapSecret;
        return result;
      }
    }
    clearError();
  }

  // Lokaler Passwort-Pfad als Default.
  result.method = AuthMethod::LOCAL;

  if (!localUser) {
    const std::string msg = "Ungültiger Benutzername oder Passwort";
    setError(msg);
    logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                  "Login fehlgeschlagen (lokal, Benutzer unbekannt)");
    setError(msg);
    return result;
  }

  if (!localUser->isActive()) {
    const std::string msg = "Benutzer ist deaktiviert";
    setError(msg);
    logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                  "Login fehlgeschlagen (lokal, deaktiviert)");
    setError(msg);
    return result;
  }

  if (!localUser->verifyPassword(password)) {
    const std::string msg = "Ungültiger Benutzername oder Passwort";
    setError(msg);
    logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                  "Login fehlgeschlagen (lokal, falsches Passwort)");
    setError(msg);
    return result;
  }

  result.user = std::move(localUser);
  result.requiresMfa =
      isMfaRequiredForUser(username, result.user->getRoleString());
  return result;
}

std::unique_ptr<core::User>
Database::authenticateUser(const std::string &username,
                           const std::string &password,
                           const std::optional<std::string> &mfaCode) {
  clearError();
  pendingAuth_.active = false;
  pendingAuth_.username.clear();
  pendingAuth_.mfaSecret.clear();

  auto result = authenticatePrimary(username, password);
  if (!result.user) {
    return nullptr;
  }

  const std::string methodStr =
      result.method == AuthMethod::LDAP ? "LDAP" : "Lokal";
  const std::string actor = result.user->getUsername();

  if (result.requiresMfa) {
    // MFA-Secret aus user_mfa bevorzugen, sonst aus LDAP übernehmen.
    const char *secretSQL = R"(
          SELECT mfa_secret
          FROM user_mfa
          WHERE username = ?
          LIMIT 1;
      )";
    sqlite3_stmt *secretStmtRaw = nullptr;
    int rc = sqlite3_prepare_v2(db_, secretSQL, -1, &secretStmtRaw, nullptr);
    if (rc == SQLITE_OK) {
      auto secretStmt = makeStatement(secretStmtRaw);
      sqlite3_bind_text(secretStmt.get(), 1, actor.c_str(), -1,
                        SQLITE_TRANSIENT);
      rc = sqlite3_step(secretStmt.get());
      if (rc == SQLITE_ROW) {
        result.mfaSecret = columnText(secretStmt.get(), 0);
      }
    }
    clearError();

    if (result.mfaSecret.empty()) {
      const std::string msg =
          "MFA ist erforderlich, aber kein MFA-Secret ist konfiguriert";
      setError(msg);
      logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                    "Login fehlgeschlagen (" + methodStr +
                        ", MFA-Secret fehlt)");
      setError(msg);
      return nullptr;
    }

    if (!mfaCode.has_value()) {
      pendingAuth_.active = true;
      pendingAuth_.username = actor;
      pendingAuth_.method = result.method;
      pendingAuth_.mfaSecret = result.mfaSecret;
      setError("MFA erforderlich. Bitte MFA-Code eingeben.");
      return nullptr;
    }

    if (!verifyMfaCode(result.mfaSecret, mfaCode.value())) {
      const std::string msg = "Ungültiger oder abgelaufener MFA-Code";
      setError(msg);
      logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                    "Login fehlgeschlagen (" + methodStr +
                        ", MFA ungültig)");
      setError(msg);
      return nullptr;
    }
  }

  // Login-Timestamp aktualisieren
  result.user->updateLastLogin();
  (void)updateUser(*result.user, actor);

  if (!startSession(result.user->getId(), actor, result.method,
                    "Authentifiziert")) {
    const std::string msg =
        "Sitzung konnte nicht gestartet werden: " + getLastError();
    setError(msg);
    return nullptr;
  }
  return std::move(result.user);
}

void Database::setError(const std::string &error) {
  lastError_ = error;
  std::cerr << "Datenbankfehler: " << error << std::endl;
}

} // namespace db
} // namespace opensylab
