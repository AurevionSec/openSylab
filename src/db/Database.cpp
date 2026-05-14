#include "db/Database.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sqlite3.h>
#include <sstream>
#include <vector>
#include <openssl/hmac.h>
#include <cstring>

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

std::string maskPathForAudit(const std::string &path) {
  try {
    std::filesystem::path p(path);
    const std::string name = p.filename().string();
    return name.empty() ? std::string("<redacted>") : name;
  } catch (const std::exception &) {
    return "<redacted>";
  }
}

// RFC 6238 TOTP (Time-Based One-Time Password) using HMAC-SHA1
// This is the INDUSTRY STANDARD for 2FA/MFA tokens (used by Google Authenticator, etc.)
int computeMfaCode(const std::string &secret, std::time_t now) {
  if (secret.empty()) {
    return -1;
  }

  // Step 1: Calculate time counter (T)
  // T = (current_time - T0) / X, where T0 = 0 (Unix epoch) and X = 30 seconds
  uint64_t timeStep = static_cast<uint64_t>(now) / 30;

  // Step 2: Convert time counter to 8-byte big-endian array
  unsigned char timeBytes[8];
  uint64_t timeStepCopy = timeStep; // Mutable copy for bit shifting
  for (int i = 7; i >= 0; i--) {
    timeBytes[i] = static_cast<unsigned char>(timeStepCopy & 0xFF);
    timeStepCopy >>= 8;
  }

  // Step 3: Compute HMAC-SHA1(secret, timeBytes)
  unsigned char hmac[20]; // SHA1 produces 20 bytes
  unsigned int hmacLen = 20;

  HMAC(EVP_sha1(),
       secret.c_str(), secret.length(),
       timeBytes, sizeof(timeBytes),
       hmac, &hmacLen);

  // Step 4: Dynamic Truncation (RFC 6238 Section 5.3)
  // Offset = last nibble of HMAC
  int offset = hmac[19] & 0x0F;

  // Step 5: Extract 4 bytes starting at offset
  uint32_t truncatedHash =
    ((hmac[offset] & 0x7F) << 24) |  // Mask most significant bit
    ((hmac[offset + 1] & 0xFF) << 16) |
    ((hmac[offset + 2] & 0xFF) << 8) |
    (hmac[offset + 3] & 0xFF);

  // Step 6: Generate 6-digit code
  int code = static_cast<int>(truncatedHash % 1000000);

  return code;
}

// RFC 6238 TOTP verification with ±1 time window tolerance (90 seconds total)
// This accounts for clock drift and network latency
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

  // Check current time window and ±1 adjacent windows (30 seconds each)
  // This provides 90-second total acceptance window for better usability
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
  sample->setUpdatedAt(
      static_cast<std::time_t>(sqlite3_column_int64(stmt, 7)));
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
  user->setMustChangePassword(sqlite3_column_int(stmt, 5) != 0);
  user->setLastLogin(static_cast<std::time_t>(sqlite3_column_int64(stmt, 6)));
  user->setCreatedDate(static_cast<std::time_t>(sqlite3_column_int64(stmt, 7)));
  user->setFullName(columnText(stmt, 8));
  user->setEmail(columnText(stmt, 9));
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
  std::cerr << "Datenbank erfolgreich geöffnet: " << dbPath_ << std::endl;
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
            registration_date INTEGER NOT NULL,
            updated_at INTEGER NOT NULL DEFAULT 0
        );

        CREATE INDEX IF NOT EXISTS idx_sample_id ON samples(sample_id);
        CREATE INDEX IF NOT EXISTS idx_patient_id ON samples(patient_id);
        CREATE INDEX IF NOT EXISTS idx_sample_registration_date ON samples(registration_date);
        CREATE INDEX IF NOT EXISTS idx_sample_status_regdate ON samples(status, registration_date);

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
        CREATE INDEX IF NOT EXISTS idx_order_requested_date ON orders(requested_date);
        CREATE INDEX IF NOT EXISTS idx_order_status_requested_date ON orders(status, requested_date);
        CREATE INDEX IF NOT EXISTS idx_order_priority_requested_date ON orders(priority, requested_date);

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
        CREATE INDEX IF NOT EXISTS idx_result_measured_date ON test_results(measured_date);
        CREATE INDEX IF NOT EXISTS idx_result_status_measured_date ON test_results(status, measured_date);

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
            must_change_password INTEGER NOT NULL DEFAULT 0,
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
            created_date INTEGER NOT NULL,
            role TEXT NOT NULL DEFAULT 'OPERATOR'
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

  // Migration: add must_change_password column to existing databases
  {
    const char *alterSQL = "ALTER TABLE users ADD COLUMN must_change_password "
                           "INTEGER NOT NULL DEFAULT 0;";
    char *alterErr = nullptr;
    (void)sqlite3_exec(db_, alterSQL, nullptr, nullptr, &alterErr);
    sqlite3_free(alterErr);
  }

  // Migration: add updated_at column to samples (v0.8)
  {
    const char *alterSQL = "ALTER TABLE samples ADD COLUMN "
                           "updated_at INTEGER NOT NULL DEFAULT 0;";
    char *alterErr = nullptr;
    (void)sqlite3_exec(db_, alterSQL, nullptr, nullptr, &alterErr);
    sqlite3_free(alterErr);
  }
  // Back-fill: use registration_date for rows created before v0.8
  {
    const char *backfillSQL =
        "UPDATE samples SET updated_at = registration_date WHERE updated_at = 0;";
    char *bfErr = nullptr;
    (void)sqlite3_exec(db_, backfillSQL, nullptr, nullptr, &bfErr);
    sqlite3_free(bfErr);
  }

  // Migration: add role column to api_keys (v0.8)
  {
    const char *alterSQL = "ALTER TABLE api_keys ADD COLUMN "
                           "role TEXT NOT NULL DEFAULT 'OPERATOR';";
    char *alterErr = nullptr;
    (void)sqlite3_exec(db_, alterSQL, nullptr, nullptr, &alterErr);
    sqlite3_free(alterErr);
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

  // Seed default admin on fresh DB — wrapped in try/catch for RNG failures.
  try {
    const char *countSQL = "SELECT COUNT(*) FROM users;";
    sqlite3_stmt *cRaw = nullptr;
    if (sqlite3_prepare_v2(db_, countSQL, -1, &cRaw, nullptr) != SQLITE_OK) {
      std::cerr << "[WARNING] Default admin check failed: " << sqlite3_errmsg(db_)
                << " — run create_admin tool manually.\n";
    } else {
      auto cStmt = makeStatement(cRaw);
      if (sqlite3_step(cStmt.get()) == SQLITE_ROW &&
          sqlite3_column_int(cStmt.get(), 0) == 0) {
        const std::string hash = core::User::hashPassword("admin");
        const std::string role = core::User::roleToString(core::User::Role::ADMIN);
        const std::time_t now = std::time(nullptr);
        const char *insertSQL = R"(
            INSERT OR IGNORE INTO users
                (username, password_hash, role, active, created_date, last_login, full_name, email, must_change_password)
            VALUES (?, ?, ?, 1, ?, 0, '', '', 1);
        )";
        sqlite3_stmt *iRaw = nullptr;
        if (sqlite3_prepare_v2(db_, insertSQL, -1, &iRaw, nullptr) != SQLITE_OK) {
          std::cerr << "[WARNING] Default admin INSERT failed: " << sqlite3_errmsg(db_)
                    << " — run create_admin tool manually.\n";
        } else {
          auto iStmt = makeStatement(iRaw);
          sqlite3_bind_text(iStmt.get(), 1, "admin", -1, SQLITE_TRANSIENT);
          sqlite3_bind_text(iStmt.get(), 2, hash.c_str(), -1, SQLITE_TRANSIENT);
          sqlite3_bind_text(iStmt.get(), 3, role.c_str(), -1, SQLITE_TRANSIENT);
          sqlite3_bind_int64(iStmt.get(), 4, static_cast<sqlite3_int64>(now));
          if (sqlite3_step(iStmt.get()) == SQLITE_DONE) {
            std::cerr << "[WARNING] Default admin created (admin/admin). "
                         "Change password immediately!\n";
          } else {
            std::cerr << "[WARNING] Default admin INSERT step failed — run "
                         "create_admin tool manually.\n";
          }
        }
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "[WARNING] Default admin seeding failed: " << e.what()
              << " — run create_admin tool manually.\n";
  }

  std::cerr << "Datenbankschema erfolgreich initialisiert" << std::endl;
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
      "samples", "orders", "test_results", "audit_log", "users",
      "retention_settings"};

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
        INSERT INTO samples (sample_id, patient_id, patient_name, description, status, registration_date, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des INSERT: " +
             std::string(_rbErr));}
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
  const std::time_t nowTs = std::time(nullptr);
  sqlite3_bind_int64(stmt.get(), 7, static_cast<sqlite3_int64>(nowTs));

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Einfügen der Probe: " +
             std::string(_rbErr));}
    return false;
  }

  std::ostringstream details;
  details << "Patient-ID: " << sample.getPatientId() << "; Patient: "
          << sample.getPatientName()
          << "; Status: " << sample.getStatusString();
  if (!sample.getDescription().empty()) {
    details << "; Beschreibung: " << sample.getDescription();
  }
  core::AuditEntry entry(core::AuditEntry::ActionType::CREATE,
                         core::AuditEntry::EntityType::SAMPLE,
                         sample.getSampleId(), normalizeActor(actor),
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

Database::BatchInsertResult
Database::createSamplesBatch(const std::vector<core::Sample> &samples,
                             const std::string &actor) {
  BatchInsertResult result;
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return result;
  }
  if (samples.empty()) {
    return result;
  }

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return result;
  }

  const char *insertSQL = R"(
        INSERT INTO samples (sample_id, patient_id, patient_name, description, status, registration_date, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des INSERT: " +
             std::string(_rbErr));}
    return result;
  }
  auto stmt = makeStatement(rawStmt);

  const std::string actorName = normalizeActor(actor);

  for (size_t i = 0; i < samples.size(); ++i) {
    const auto &sample = samples[i];
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
    sqlite3_bind_int64(
        stmt.get(), 6,
        static_cast<sqlite3_int64>(sample.getRegistrationDate()));
    sqlite3_bind_int64(stmt.get(), 7,
        static_cast<sqlite3_int64>(std::time(nullptr)));

    rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
      BatchInsertError error;
      error.index = i;
      error.message = sqlite3_errmsg(db_);
      result.failures.push_back(error);
      sqlite3_reset(stmt.get());
      sqlite3_clear_bindings(stmt.get());
      continue;
    }

    std::ostringstream details;
    details << "Patient-ID: " << sample.getPatientId() << "; Patient: "
            << sample.getPatientName()
            << "; Status: " << sample.getStatusString();
    if (!sample.getDescription().empty()) {
      details << "; Beschreibung: " << sample.getDescription();
    }
    core::AuditEntry entry(core::AuditEntry::ActionType::CREATE,
                           core::AuditEntry::EntityType::SAMPLE,
                           sample.getSampleId(), actorName, details.str());
    if (!logAudit(entry)) {
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
      result.inserted = 0;
      BatchInsertError error;
      error.index = i;
      error.message = "Audit log failed: " + getLastError();
      result.failures.push_back(error);
      return result;
    }

    sqlite3_reset(stmt.get());
    sqlite3_clear_bindings(stmt.get());
    result.inserted++;
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    result.inserted = 0;
    return result;
  }

  return result;
}

std::unique_ptr<core::Sample> Database::getSample(int id) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return nullptr;
  }

  const char *selectSQL = R"(
        SELECT id, sample_id, patient_id, patient_name, description, status, registration_date, updated_at
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
        SELECT id, sample_id, patient_id, patient_name, description, status, registration_date, updated_at
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
        SELECT id, sample_id, patient_id, patient_name, description, status, registration_date, updated_at
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
         "registration_date, updated_at FROM samples";

  if (!conditions.empty()) {
    sql << " WHERE ";
    for (size_t i = 0; i < conditions.size(); ++i) {
      if (i > 0) {
        sql << " AND ";
      }
      sql << conditions[i];
    }
  }

  sql << " ORDER BY registration_date DESC";

  int limitValue = -1;
  int offsetValue = 0;
  if (filter.limit.has_value() && filter.limit.value() > 0) {
    limitValue = filter.limit.value();
    if (filter.offset.has_value() && filter.offset.value() > 0) {
      offsetValue = filter.offset.value();
    }
    sql << " LIMIT ? OFFSET ?";
  }
  sql << ";";

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

  if (limitValue > 0) {
    sqlite3_bind_int(stmt.get(), bindIndex++, limitValue);
    sqlite3_bind_int(stmt.get(), bindIndex++, offsetValue);
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

int Database::getSamplesCount(const SampleFilter &filter) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return -1;
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
  sql << "SELECT COUNT(*) FROM samples";

  if (!conditions.empty()) {
    sql << " WHERE ";
    for (size_t i = 0; i < conditions.size(); ++i) {
      if (i > 0) {
        sql << " AND ";
      }
      sql << conditions[i];
    }
  }

  sql << ";";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des COUNT: " +
             std::string(sqlite3_errmsg(db_)));
    return -1;
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

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_ROW) {
    setError("Fehler beim Zählen der Proben: " +
             std::string(sqlite3_errmsg(db_)));
    return -1;
  }

  int count = static_cast<int>(sqlite3_column_int64(stmt.get(), 0));
  return count;
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
            registration_date = ?,
            updated_at = strftime('%s','now')
        WHERE id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, updateSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des UPDATE: " +
             std::string(_rbErr));}
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
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Aktualisieren der Probe: " +
             std::string(_rbErr));}
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

  // Check for active orders before archiving — prevents orphaned orders
  {
    auto orders = getOrdersBySampleId(existing->getSampleId());
    for (const auto &ord : orders) {
      const auto s = ord->getStatus();
      if (s != core::Order::Status::CANCELLED) {
        setError("Probe hat aktive Auftraege (ID: " + ord->getOrderId() +
                 "); Auftraege zuerst stornieren.");
        return false;
      }
    }
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

  const char *deleteSQL = "UPDATE samples SET status = ? WHERE id = ?;";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des UPDATE: " +
             std::string(_rbErr));}
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  std::string statusStr = core::Sample::statusToString(core::Sample::Status::ARCHIVED);
  sqlite3_bind_text(stmt.get(), 1, statusStr.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 2, id);
  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Löschen der Probe: " +
             std::string(_rbErr));}
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

  std::ofstream output(filePath);
  if (!output.is_open()) {
    setError("Exportdatei konnte nicht geschrieben werden");
    output.close();
    std::remove(filePath.c_str());
    return false;
  }

  output << "sample_id,patient_id,patient_name,description,status\n";

  const char *selectSQL =
      "SELECT sample_id, patient_id, patient_name, description, status "
      "FROM samples ORDER BY registration_date DESC;";
  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    output.close();
    std::remove(filePath.c_str());
    return false;
  }
  auto stmt = makeStatement(rawStmt);

  int exported = 0;
  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    output << escapeCsvField(columnText(stmt.get(), 0)) << ","
           << escapeCsvField(columnText(stmt.get(), 1)) << ","
           << escapeCsvField(columnText(stmt.get(), 2)) << ","
           << escapeCsvField(columnText(stmt.get(), 3)) << ","
           << escapeCsvField(columnText(stmt.get(), 4)) << "\n";
    exported++;
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Proben: " +
             std::string(sqlite3_errmsg(db_)));
    output.close();
    std::remove(filePath.c_str());
    return false;
  }

  if (exported == 0) {
    output.close();
    std::remove(filePath.c_str());
    setError("Keine Proben zum Export");
    return false;
  }

  if (!output) {
    setError("Fehler beim Schreiben der Exportdatei");
    output.close();
    std::remove(filePath.c_str());
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
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des INSERT: " +
             std::string(_rbErr));}
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
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Einfügen des Auftrags: " +
             std::string(_rbErr));}
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

  int limitValue = -1;
  int offsetValue = 0;
  if (filter.limit.has_value() && filter.limit.value() > 0) {
    limitValue = filter.limit.value();
    if (filter.offset.has_value() && filter.offset.value() > 0) {
      offsetValue = filter.offset.value();
    }
    query << " LIMIT ? OFFSET ?";
  }

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

  if (limitValue > 0) {
    sqlite3_bind_int(stmt.get(), index++, limitValue);
    sqlite3_bind_int(stmt.get(), index++, offsetValue);
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

int Database::getOrdersCount(const OrderFilter &filter) {
  clearError();
  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return -1;
  }

  std::ostringstream sql;
  sql << "SELECT COUNT(*) FROM orders WHERE 1=1";
  if (!filter.status.empty())   sql << " AND status = ?";
  if (!filter.sampleId.empty()) sql << " AND sample_id = ?";
  if (!filter.priority.empty()) sql << " AND priority = ?";
  sql << ";";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Zaehlen der Auftraege: " + std::string(sqlite3_errmsg(db_)));
    return -1;
  }
  auto stmt = makeStatement(rawStmt);

  int idx = 1;
  if (!filter.status.empty())
    sqlite3_bind_text(stmt.get(), idx++, filter.status.c_str(), -1, SQLITE_TRANSIENT);
  if (!filter.sampleId.empty())
    sqlite3_bind_text(stmt.get(), idx++, filter.sampleId.c_str(), -1, SQLITE_TRANSIENT);
  if (!filter.priority.empty())
    sqlite3_bind_text(stmt.get(), idx++, filter.priority.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_ROW) {
    setError("Fehler beim Zaehlen der Auftraege: " + std::string(sqlite3_errmsg(db_)));
    return -1;
  }
  return static_cast<int>(sqlite3_column_int64(stmt.get(), 0));
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
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des UPDATE: " +
             std::string(_rbErr));}
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
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Aktualisieren des Auftrags: " +
             std::string(_rbErr));}
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

  // Check for active results before cancelling order — prevents orphaned results
  {
    auto results = getTestResultsByOrderId(existing->getId());
    for (const auto &res : results) {
      const auto s = res->getStatus();
      if (s != core::TestResult::Status::VALIDATED &&
          s != core::TestResult::Status::REJECTED) {
        setError("Auftrag hat aktive Ergebnisse (ID: " + res->getResultId() +
                 "); Ergebnisse zuerst abschliessen oder ablehnen.");
        return false;
      }
    }
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

  const char *deleteSQL = "UPDATE orders SET status = ? WHERE id = ?;";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des UPDATE: " +
             std::string(_rbErr));}
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  std::string statusStr = core::Order::statusToString(core::Order::Status::CANCELLED);
  sqlite3_bind_text(stmt.get(), 1, statusStr.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 2, id);
  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Löschen des Auftrags: " +
             std::string(_rbErr));}
    return false;
  }

  int changes = sqlite3_changes(db_);
  if (changes == 0) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Auftrag mit ID " + std::to_string(id) + " nicht gefunden");
    return false;
  }

  std::ostringstream details;
  details << "Auftrags-ID: " << existing->getOrderId()
          << "; Proben-ID: " << existing->getSampleId()
          << "; Status: " << existing->getStatusString();
  core::AuditEntry entry(core::AuditEntry::ActionType::DELETE,
                         core::AuditEntry::EntityType::ORDER,
                         existing->getOrderId(), normalizeActor(actor),
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
      core::TestResult::flagToString(result.getFlag());

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
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des INSERT: " +
             std::string(_rbErr));}
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
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Einfügen des Ergebnisses: " +
             std::string(_rbErr));}
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

Database::BatchInsertResult
Database::createTestResultsBatch(const std::vector<core::TestResult> &results,
                                 const std::string &actor) {
  BatchInsertResult result;
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return result;
  }
  if (results.empty()) {
    return result;
  }

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return result;
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
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des INSERT: " +
             std::string(_rbErr));}
    return result;
  }
  auto stmt = makeStatement(rawStmt);

  const std::string actorName = normalizeActor(actor);

  for (size_t i = 0; i < results.size(); ++i) {
    const auto &item = results[i];
    auto failWith = [&](const std::string &message) {
      BatchInsertError error;
      error.index = i;
      error.message = message;
      result.failures.push_back(error);
    };

    if (item.getResultId().empty()) {
      failWith("Ergebnis-ID darf nicht leer sein");
      continue;
    }
    if (item.getOrderId() <= 0) {
      failWith("Auftrags-ID ist ungültig");
      continue;
    }
    if (item.getTestParameter().empty()) {
      failWith("Testparameter darf nicht leer sein");
      continue;
    }
    if (item.getValue().empty()) {
      failWith("Messwert darf nicht leer sein");
      continue;
    }
    if (item.getUnit().empty()) {
      failWith("Einheit darf nicht leer sein");
      continue;
    }

    auto order = getOrder(item.getOrderId());
    if (!order) {
      failWith(getLastError());
      continue;
    }

    const std::string computedFlag =
        core::TestResult::flagToString(item.getFlag());

    sqlite3_bind_text(stmt.get(), 1, item.getResultId().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 2, item.getOrderId());
    sqlite3_bind_text(stmt.get(), 3, item.getTestParameter().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 4, item.getValue().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 5, item.getUnit().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 6, item.getReferenceRange().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt.get(), 7, item.getReferenceLow());
    sqlite3_bind_double(stmt.get(), 8, item.getReferenceHigh());
    sqlite3_bind_text(stmt.get(), 9, item.getStatusString().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 10, computedFlag.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int64(
        stmt.get(), 11,
        static_cast<sqlite3_int64>(item.getMeasuredDate()));
    sqlite3_bind_text(stmt.get(), 12, item.getMeasuredBy().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 13, item.getComment().c_str(), -1,
                      SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
      failWith(sqlite3_errmsg(db_));
      sqlite3_reset(stmt.get());
      sqlite3_clear_bindings(stmt.get());
      continue;
    }

    std::ostringstream details;
    details << "Auftrags-ID: " << item.getOrderId()
            << "; Parameter: " << item.getTestParameter()
            << "; Status: " << item.getStatusString();
    if (!item.getValue().empty()) {
      details << "; Wert: " << item.getValue();
    }
    if (!item.getUnit().empty()) {
      details << " " << item.getUnit();
    }
    core::AuditEntry entry(core::AuditEntry::ActionType::CREATE,
                           core::AuditEntry::EntityType::RESULT,
                           item.getResultId(), actorName, details.str());
    if (!logAudit(entry)) {
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
      result.inserted = 0;
      BatchInsertError error;
      error.index = i;
      error.message = "Audit log failed: " + getLastError();
      result.failures.push_back(error);
      return result;
    }

    sqlite3_reset(stmt.get());
    sqlite3_clear_bindings(stmt.get());
    result.inserted++;
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    result.inserted = 0;
    return result;
  }

  return result;
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
Database::getTestResultsByOrderId(int orderId, std::optional<int> limit,
                                  std::optional<int> offset) {
  std::vector<std::unique_ptr<core::TestResult>> results;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return results;
  }

  std::ostringstream sql;
  sql << "SELECT id, result_id, order_id, test_parameter, value, unit, "
         "reference_range, reference_low, reference_high, "
         "status, flag, measured_date, measured_by, comment "
         "FROM test_results WHERE order_id = ? ORDER BY id";

  int limitValue = -1;
  int offsetValue = 0;
  if (limit.has_value() && limit.value() > 0) {
    limitValue = limit.value();
    if (offset.has_value() && offset.value() > 0) {
      offsetValue = offset.value();
    }
    sql << " LIMIT ? OFFSET ?";
  }
  sql << ";";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return results;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, orderId);
  int bindIndex = 2;
  if (limitValue > 0) {
    sqlite3_bind_int(stmt.get(), bindIndex++, limitValue);
    sqlite3_bind_int(stmt.get(), bindIndex++, offsetValue);
  }

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

int Database::getTestResultsCount(std::optional<int> orderIdFilter) {
  clearError();
  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return -1;
  }

  std::string sql = "SELECT COUNT(*) FROM test_results";
  if (orderIdFilter.has_value()) {
    sql += " WHERE order_id = ?";
  }
  sql += ";";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Zaehlen der Ergebnisse: " + std::string(sqlite3_errmsg(db_)));
    return -1;
  }
  auto stmt = makeStatement(rawStmt);

  if (orderIdFilter.has_value()) {
    sqlite3_bind_int(stmt.get(), 1, orderIdFilter.value());
  }

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_ROW) {
    setError("Fehler beim Zaehlen der Ergebnisse: " + std::string(sqlite3_errmsg(db_)));
    return -1;
  }
  return static_cast<int>(sqlite3_column_int64(stmt.get(), 0));
}

std::vector<std::unique_ptr<core::TestResult>>
Database::getAllTestResults(std::optional<int> limit,
                            std::optional<int> offset) {
  std::vector<std::unique_ptr<core::TestResult>> results;

  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return results;
  }

  std::ostringstream sql;
  sql << "SELECT id, result_id, order_id, test_parameter, value, unit, "
         "reference_range, reference_low, reference_high, "
         "status, flag, measured_date, measured_by, comment "
         "FROM test_results ORDER BY id DESC";

  int limitValue = -1;
  int offsetValue = 0;
  if (limit.has_value() && limit.value() > 0) {
    limitValue = limit.value();
    if (offset.has_value() && offset.value() > 0) {
      offsetValue = offset.value();
    }
    sql << " LIMIT ? OFFSET ?";
  }
  sql << ";";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    return results;
  }

  auto stmt = makeStatement(rawStmt);

  if (limitValue > 0) {
    sqlite3_bind_int(stmt.get(), 1, limitValue);
    sqlite3_bind_int(stmt.get(), 2, offsetValue);
  }

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

  if (existing->getStatus() == core::TestResult::Status::REJECTED) {
    setError("REJECTED-Ergebnisse koennen nicht mehr geaendert werden (Terminalzustand).");
    return false;
  }

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, &errMsg);
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

  core::AuditEntry auditEntry(core::AuditEntry::ActionType::UPDATE,
                              core::AuditEntry::EntityType::RESULT,
                              result.getResultId(), normalizeActor(actor),
                              detailText);
  if (!logAudit(auditEntry)) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

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

  if (existing->getStatus() == core::TestResult::Status::REJECTED) {
    setError("REJECTED-Ergebnisse koennen nicht mehr geaendert werden (Terminalzustand).");
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

  std::ofstream output(filePath);
  if (!output.is_open()) {
    setError("Exportdatei konnte nicht geschrieben werden");
    output.close();
    std::remove(filePath.c_str());
    return false;
  }

  output << "result_id,order_id,test_parameter,value,unit,reference_low,"
            "reference_high,status,flag,measured_date,measured_by,comment\n";

  std::ostringstream selectSql;
  selectSql
      << "SELECT result_id, order_id, test_parameter, value, unit, "
         "reference_low, reference_high, status, flag, measured_date, "
         "measured_by, comment FROM test_results WHERE status = ?";
  if (orderId.has_value()) {
    selectSql << " AND order_id = ?";
  }
  selectSql << " ORDER BY id;";

  sqlite3_stmt *rawStmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, selectSql.str().c_str(), -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Vorbereiten des SELECT: " +
             std::string(sqlite3_errmsg(db_)));
    output.close();
    std::remove(filePath.c_str());
    return false;
  }
  auto stmt = makeStatement(rawStmt);

  int bindIndex = 1;
  const std::string validatedStatus =
      core::TestResult::statusToString(core::TestResult::Status::VALIDATED);
  sqlite3_bind_text(stmt.get(), bindIndex++, validatedStatus.c_str(), -1,
                    SQLITE_TRANSIENT);
  if (orderId.has_value()) {
    sqlite3_bind_int(stmt.get(), bindIndex++, *orderId);
  }

  int exported = 0;
  std::vector<std::string> exportedResultIds;
  while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    exportedResultIds.push_back(columnText(stmt.get(), 0));
    output << escapeCsvField(columnText(stmt.get(), 0)) << ","
           << columnText(stmt.get(), 1) << ","
           << escapeCsvField(columnText(stmt.get(), 2)) << ","
           << escapeCsvField(columnText(stmt.get(), 3)) << ","
           << escapeCsvField(columnText(stmt.get(), 4)) << ","
           << columnText(stmt.get(), 5) << ","
           << columnText(stmt.get(), 6) << ","
           << escapeCsvField(columnText(stmt.get(), 7)) << ","
           << escapeCsvField(columnText(stmt.get(), 8)) << ","
           << columnText(stmt.get(), 9) << ","
           << escapeCsvField(columnText(stmt.get(), 10)) << ","
           << escapeCsvField(columnText(stmt.get(), 11)) << "\n";
    exported++;
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Ergebnisse: " +
             std::string(sqlite3_errmsg(db_)));
    output.close();
    std::remove(filePath.c_str());
    return false;
  }

  if (exported == 0) {
    output.close();
    std::remove(filePath.c_str());
    setError("Keine validierten Ergebnisse zum Export");
    return false;
  }

  if (!output) {
    setError("Fehler beim Schreiben der Exportdatei");
    output.close();
    std::remove(filePath.c_str());
    return false;
  }

  char *errMsg = nullptr;
  rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                    &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    output.close();
    std::remove(filePath.c_str());
    return false;
  }

  const std::string actor = normalizeActor(user);
  const std::string details = "Export: " + maskPathForAudit(filePath) +
                              "; Anzahl: " + std::to_string(exported);

  for (const auto &resultId : exportedResultIds) {
    core::AuditEntry entry(core::AuditEntry::ActionType::EXPORT,
                           core::AuditEntry::EntityType::RESULT, resultId,
                           actor, details);
    if (!logAudit(entry)) {
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
      std::remove(filePath.c_str());
      return false;
    }
  }

  rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Commit der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    std::remove(filePath.c_str());
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

  // Terminal-state guard: REJECTED results cannot be promoted to VALIDATED
  if (result->getStatus() == core::TestResult::Status::REJECTED) {
    setError("REJECTED-Ergebnisse koennen nicht validiert werden (Terminalzustand).");
    return false;
  }
  if (result->getStatus() == core::TestResult::Status::VALIDATED) {
    setError("Ergebnis ist bereits validiert.");
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

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  const char *deleteSQL = "UPDATE test_results SET status = ? WHERE id = ?;";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des UPDATE: " +
             std::string(_rbErr));}
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  std::string statusStr = core::TestResult::statusToString(core::TestResult::Status::REJECTED);
  sqlite3_bind_text(stmt.get(), 1, statusStr.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 2, id);
  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Löschen des Ergebnisses: " +
             std::string(_rbErr));}
    return false;
  }

  int changes = sqlite3_changes(db_);
  if (changes == 0) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Ergebnis mit ID " + std::to_string(id) + " nicht gefunden");
    return false;
  }

  std::ostringstream details;
  details << "Ergebnis-ID: " << existing->getResultId()
          << "; Parameter: " << existing->getTestParameter()
          << "; Status: " << existing->getStatusString();
  core::AuditEntry entry(core::AuditEntry::ActionType::DELETE,
                         core::AuditEntry::EntityType::RESULT,
                         existing->getResultId(), normalizeActor(actor),
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

  const std::vector<std::string> sampleStatuses = {
      core::Sample::statusToString(core::Sample::Status::REGISTERED),
      core::Sample::statusToString(core::Sample::Status::IN_ANALYSIS),
      core::Sample::statusToString(core::Sample::Status::ANALYZED),
      core::Sample::statusToString(core::Sample::Status::VALIDATED),
      core::Sample::statusToString(core::Sample::Status::ARCHIVED)};
  stats.byStatus.reserve(sampleStatuses.size());
  for (const auto &status : sampleStatuses) {
    stats.byStatus.push_back(StatusCount{status, 0});
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
    const std::string status = columnText(stmt.get(), 0);
    const int count = sqlite3_column_int(stmt.get(), 1);
    bool updated = false;
    for (auto &entry : stats.byStatus) {
      if (entry.status == status) {
        entry.count = count;
        updated = true;
        break;
      }
    }
    if (!updated) {
      stats.byStatus.push_back(StatusCount{status, count});
    }
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

  const std::vector<std::string> orderStatuses = {
      core::Order::statusToString(core::Order::Status::REQUESTED),
      core::Order::statusToString(core::Order::Status::IN_PROGRESS),
      core::Order::statusToString(core::Order::Status::COMPLETED),
      core::Order::statusToString(core::Order::Status::VALIDATED),
      core::Order::statusToString(core::Order::Status::CANCELLED)};
  stats.byStatus.reserve(orderStatuses.size());
  for (const auto &status : orderStatuses) {
    stats.byStatus.push_back(StatusCount{status, 0});
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
    const std::string status = columnText(stmt.get(), 0);
    const int count = sqlite3_column_int(stmt.get(), 1);
    bool updated = false;
    for (auto &entry : stats.byStatus) {
      if (entry.status == status) {
        entry.count = count;
        updated = true;
        break;
      }
    }
    if (!updated) {
      stats.byStatus.push_back(StatusCount{status, count});
    }
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

  const std::vector<std::string> resultStatuses = {
      core::TestResult::statusToString(core::TestResult::Status::PENDING),
      core::TestResult::statusToString(core::TestResult::Status::ENTERED),
      core::TestResult::statusToString(core::TestResult::Status::VALIDATED),
      core::TestResult::statusToString(core::TestResult::Status::REJECTED),
      core::TestResult::statusToString(core::TestResult::Status::REPEATED)};
  stats.byStatus.reserve(resultStatuses.size());
  for (const auto &status : resultStatuses) {
    stats.byStatus.push_back(StatusCount{status, 0});
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
    const std::string status = columnText(stmt.get(), 0);
    const int count = sqlite3_column_int(stmt.get(), 1);
    bool updated = false;
    for (auto &entry : stats.byStatus) {
      if (entry.status == status) {
        entry.count = count;
        updated = true;
        break;
      }
    }
    if (!updated) {
      stats.byStatus.push_back(StatusCount{status, count});
    }
  }
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Status-Statistiken: " +
             std::string(sqlite3_errmsg(db_)));
  }

  return stats;
}

std::vector<Database::StatusCount> Database::getOrderPriorityStats() {
  std::vector<StatusCount> result;
  if (!isOpen_) return result;
  const char *sql =
      "SELECT priority, COUNT(*) as cnt FROM orders "
      "WHERE status != 'CANCELLED' GROUP BY priority ORDER BY cnt DESC";
  sqlite3_stmt *rawStmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &rawStmt, nullptr) != SQLITE_OK) {
    return result;
  }
  auto stmt = makeStatement(rawStmt);
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    StatusCount sc;
    sc.status = columnText(stmt.get(), 0);
    sc.count = sqlite3_column_int(stmt.get(), 1);
    result.push_back(sc);
  }
  return result;
}

int Database::getCriticalResultCount() {
  if (!isOpen_) return 0;
  const char *sql =
      "SELECT COUNT(*) FROM test_results "
      "WHERE flag = 'CRITICAL' AND status != 'REJECTED'";
  sqlite3_stmt *rawStmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &rawStmt, nullptr) != SQLITE_OK) {
    return 0;
  }
  auto stmt = makeStatement(rawStmt);
  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    return sqlite3_column_int(stmt.get(), 0);
  }
  return 0;
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
    output.close();
    std::remove(filePath.c_str());
    return false;
  }

  auto formatDate = [](const std::optional<std::time_t> &value) {
    if (!value.has_value()) {
      return std::string("none");
    }
    std::tm tm{}; localtime_r(&value.value(), &tm);
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
    output.close();
    std::remove(filePath.c_str());
    return false;
  }

  std::ostringstream details;
  details << "Export stats report: " << maskPathForAudit(filePath)
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

  core::AuditEntry auditEntry(core::AuditEntry::ActionType::EXPORT,
                              core::AuditEntry::EntityType::SYSTEM,
                              "stats_report", normalizeActor(actor),
                              details.str());
  if (!logAudit(auditEntry)) {
    std::remove(filePath.c_str());
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

  std::ofstream output(filePath);
  if (!output.is_open()) {
    setError("Exportdatei konnte nicht geschrieben werden");
    output.close();
    std::remove(filePath.c_str());
    return false;
  }

  output << "id,action,entity,entity_id,user,timestamp,details\n";

  int limit = filter.limit;
  if (limit <= 0) {
    limit = 100;
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
    output.close();
    std::remove(filePath.c_str());
    return false;
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
    output << sqlite3_column_int(stmt.get(), 0) << ","
           << escapeCsvField(columnText(stmt.get(), 1)) << ","
           << escapeCsvField(columnText(stmt.get(), 2)) << ","
           << escapeCsvField(columnText(stmt.get(), 3)) << ","
           << escapeCsvField(columnText(stmt.get(), 4)) << ","
           << static_cast<long long>(sqlite3_column_int64(stmt.get(), 5)) << ","
           << escapeCsvField(columnText(stmt.get(), 6)) << "\n";
    exportedCount++;
  }

  if (rc != SQLITE_DONE) {
    setError("Fehler beim Abrufen der Audit-Einträge: " +
             std::string(sqlite3_errmsg(db_)));
    output.close();
    std::remove(filePath.c_str());
    return false;
  }

  if (exportedCount == 0) {
    output.close();
    std::remove(filePath.c_str());
    setError("Keine Audit-Einträge zum Export");
    return false;
  }

  if (!output) {
    setError("Fehler beim Schreiben der Exportdatei");
    output.close();
    std::remove(filePath.c_str());
    return false;
  }

  output.close();

  const std::string details = "Export: " + maskPathForAudit(filePath) +
                              "; Anzahl: " + std::to_string(exportedCount);
  core::AuditEntry auditEntry(core::AuditEntry::ActionType::EXPORT,
                              core::AuditEntry::EntityType::SYSTEM,
                              "audit_log", normalizeActor(actor), details);
  if (!logAudit(auditEntry)) {
    std::remove(filePath.c_str());
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
      now - static_cast<std::time_t>(retentionDays) * 24 * 60 * 60;

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  const char *deleteSQL =
      "DELETE FROM audit_log WHERE timestamp < ?;";
  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des DELETE: " +
             std::string(_rbErr));}
    return false;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_int64(stmt.get(), 1, static_cast<sqlite3_int64>(cutoff));

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Anwenden der Retention-Regeln: " +
             std::string(_rbErr));}
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
  core::AuditEntry entry(core::AuditEntry::ActionType::ACCESS, entity, entityId,
                         actor, info);
  return logAudit(entry);
}

bool Database::logResultRetryImport(const std::vector<std::string> &resultIds,
                                    const std::string &user,
                                    const std::string &filePath) {
  clearError();
  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }
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
  const std::string details = "Retry-Import: " + maskPathForAudit(filePath) +
                              "; Anzahl: " + std::to_string(resultIds.size());

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
        INSERT INTO users (username, password_hash, role, active, last_login,
                          created_date, full_name, email, must_change_password)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des INSERT: " +
             std::string(_rbErr));}
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
  sqlite3_bind_int(stmt.get(), 9, user.mustChangePassword() ? 1 : 0);

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Einfügen des Benutzers: " +
             std::string(_rbErr));}
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
  core::AuditEntry entry(core::AuditEntry::ActionType::CREATE,
                         core::AuditEntry::EntityType::USER,
                         user.getUsername(), normalizeActor(actor),
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

std::unique_ptr<core::User> Database::getUser(int id) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return nullptr;
  }

  const char *selectSQL = R"(
        SELECT id, username, password_hash, role, active,
               must_change_password, last_login, created_date, full_name, email
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
        SELECT id, username, password_hash, role, active,
               must_change_password, last_login, created_date, full_name, email
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
        SELECT id, username, password_hash, role, active,
               must_change_password, last_login, created_date, full_name, email
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

  // Open transaction BEFORE reading existing state — prevents TOCTOU where
  // concurrent writers could change role/active between the read and the write.
  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  auto existing = getUser(user.getId());
  if (!existing) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  // Protect against removing the last active admin (inside transaction for atomicity)
  const bool wasAdmin = (existing->getRole() == core::User::Role::ADMIN);
  const bool stillAdmin = (user.getRole() == core::User::Role::ADMIN);
  const bool beingDeactivated = existing->isActive() && !user.isActive(); // state delta
  if (wasAdmin && (!stillAdmin || beingDeactivated)) {
    const char *countSQL =
        "SELECT COUNT(*) FROM users WHERE role = ? AND active = 1 AND id != ?;";
    sqlite3_stmt *rawCount = nullptr;
    const std::string adminRoleStr = core::User::roleToString(core::User::Role::ADMIN);
    if (sqlite3_prepare_v2(db_, countSQL, -1, &rawCount, nullptr) != SQLITE_OK) {
      {const std::string _rbErr = sqlite3_errmsg(db_);
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            setError("Fehler beim Prüfen der Admin-Anzahl: " + std::string(_rbErr));}
      return false;
    }
    {
      auto countStmt = makeStatement(rawCount);
      sqlite3_bind_text(countStmt.get(), 1, adminRoleStr.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_int(countStmt.get(), 2, user.getId());
      if (sqlite3_step(countStmt.get()) != SQLITE_ROW) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Lesen der Admin-Anzahl");
        return false;
      }
      if (sqlite3_column_int(countStmt.get(), 0) == 0) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Letzten aktiven Administrator kann nicht entfernt oder deaktiviert werden");
        return false;
      }
    }
  }

  // Auto-clear must_change_password when password is being changed
  const bool passwordChanged =
      (user.getPasswordHash() != existing->getPasswordHash());

  const char *updateSQL = R"(
        UPDATE users SET
            username = ?,
            password_hash = ?,
            role = ?,
            active = ?,
            last_login = ?,
            created_date = ?,
            full_name = ?,
            email = ?,
            must_change_password = ?
        WHERE id = ?;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, updateSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des UPDATE: " +
             std::string(_rbErr));}
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
  sqlite3_bind_int(stmt.get(), 9, passwordChanged ? 0 : (user.mustChangePassword() ? 1 : 0));
  sqlite3_bind_int(stmt.get(), 10, user.getId());

  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Aktualisieren des Benutzers: " +
             std::string(_rbErr));}
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

  core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                         core::AuditEntry::EntityType::USER,
                         user.getUsername(), normalizeActor(actor),
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

bool Database::deleteUser(int id, const std::string &actor) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  // Open transaction BEFORE reading existing state — prevents TOCTOU where
  // a concurrent writer could change role/active between read and write.
  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  auto existing = getUser(id);
  if (!existing) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  if (!existing->isActive()) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Benutzer mit ID " + std::to_string(id) +
             " ist bereits deaktiviert");
    return false;
  }

  // Prevent deactivating the last active admin (inside transaction for atomicity)
  if (existing->getRole() == core::User::Role::ADMIN) {
    const char *countSQL =
        "SELECT COUNT(*) FROM users WHERE role = ? AND active = 1 AND id != ?;";
    sqlite3_stmt *rawCount = nullptr;
    const std::string adminRoleStr = core::User::roleToString(core::User::Role::ADMIN);
    if (sqlite3_prepare_v2(db_, countSQL, -1, &rawCount, nullptr) != SQLITE_OK) {
      {const std::string _rbErr = sqlite3_errmsg(db_);
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            setError("Fehler beim Prüfen der Admin-Anzahl: " + std::string(_rbErr));}
      return false;
    }
    {
      auto countStmt = makeStatement(rawCount);
      sqlite3_bind_text(countStmt.get(), 1, adminRoleStr.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_int(countStmt.get(), 2, id);
      if (sqlite3_step(countStmt.get()) != SQLITE_ROW) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Lesen der Admin-Anzahl");
        return false;
      }
      int remaining = sqlite3_column_int(countStmt.get(), 0);
      if (remaining == 0) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Letzten aktiven Administrator kann nicht deaktiviert werden");
        return false;
      }
    }
  }

  const char *deleteSQL = "UPDATE users SET active = 0 WHERE id = ?;";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &rawStmt, nullptr);

  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des DELETE: " +
             std::string(_rbErr));}
    return false;
  }

  auto stmt = makeStatement(rawStmt);

  sqlite3_bind_int(stmt.get(), 1, id);
  rc = sqlite3_step(stmt.get());

  if (rc != SQLITE_DONE) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Deaktivieren des Benutzers: " +
             std::string(_rbErr));}
    return false;
  }

  int changes = sqlite3_changes(db_);
  if (changes == 0) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Benutzer mit ID " + std::to_string(id) + " nicht gefunden");
    return false;
  }

  std::ostringstream details;
  details << "Benutzername: " << existing->getUsername()
          << "; Rolle: " << existing->getRoleString()
          << "; Status: " << formatActive(existing->isActive()) << " -> "
          << formatActive(false);
  core::AuditEntry entry(core::AuditEntry::ActionType::DELETE,
                         core::AuditEntry::EntityType::USER,
                         existing->getUsername(), normalizeActor(actor),
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
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des Rollen-INSERT: " +
             std::string(_rbErr));}
    return false;
  }
  auto roleStmt = makeStatement(roleStmtRaw);

  sqlite3_bind_text(roleStmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(roleStmt.get(), 2, "", -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(roleStmt.get());
  if (rc != SQLITE_DONE) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Einfügen der Rolle: " +
             std::string(_rbErr));}
    return false;
  }

  const char *insertPermSQL = R"(
        INSERT OR IGNORE INTO role_permissions (role_name, permission)
        VALUES (?, ?);
    )";
  sqlite3_stmt *permStmtRaw = nullptr;
  rc = sqlite3_prepare_v2(db_, insertPermSQL, -1, &permStmtRaw, nullptr);
  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des Berechtigungs-INSERT: " +
             std::string(_rbErr));}
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
      {const std::string _rbErr = sqlite3_errmsg(db_);
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            setError("Fehler beim Einfügen der Berechtigung: " +
               std::string(_rbErr));}
      return false;
    }
  }

  std::ostringstream details;
  details << "Berechtigungen: [" << joinList(permissions) << "]";
  core::AuditEntry entry(core::AuditEntry::ActionType::CREATE,
                         core::AuditEntry::EntityType::ROLE, name,
                         normalizeActor(actor), details.str());
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
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des DELETE: " +
             std::string(_rbErr));}
    return false;
  }
  auto deleteStmt = makeStatement(deleteStmtRaw);
  sqlite3_bind_text(deleteStmt.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(deleteStmt.get());
  if (rc != SQLITE_DONE) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Löschen der Berechtigungen: " +
             std::string(_rbErr));}
    return false;
  }

  const char *insertPermSQL = R"(
        INSERT OR IGNORE INTO role_permissions (role_name, permission)
        VALUES (?, ?);
    )";
  sqlite3_stmt *permStmtRaw = nullptr;
  rc = sqlite3_prepare_v2(db_, insertPermSQL, -1, &permStmtRaw, nullptr);
  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des Berechtigungs-INSERT: " +
             std::string(_rbErr));}
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
      {const std::string _rbErr = sqlite3_errmsg(db_);
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            setError("Fehler beim Einfügen der Berechtigung: " +
               std::string(_rbErr));}
      return false;
    }
  }

  std::ostringstream details;
  details << "Berechtigungen: [" << joinList(existingPerms) << "] -> ["
          << joinList(permissions) << "]";
  core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                         core::AuditEntry::EntityType::ROLE, name,
                         normalizeActor(actor), details.str());
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

  // Open transaction BEFORE reads to prevent TOCTOU on security-critical user state
  char *errMsg = nullptr;
  rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                    &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  auto existing = getUser(userId);
  if (!existing) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  // Protect against demoting the last active admin via this function
  const bool wasAdmin = (existing->getRole() == core::User::Role::ADMIN);
  const bool stillAdmin = (core::User::stringToRole(roleName) == core::User::Role::ADMIN);
  if (wasAdmin && !stillAdmin) {
    const char *countSQL =
        "SELECT COUNT(*) FROM users WHERE role = ? AND active = 1 AND id != ?;";
    sqlite3_stmt *rawCount = nullptr;
    const std::string adminRoleStr = core::User::roleToString(core::User::Role::ADMIN);
    if (sqlite3_prepare_v2(db_, countSQL, -1, &rawCount, nullptr) != SQLITE_OK) {
      {const std::string _rbErr = sqlite3_errmsg(db_);
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            setError("Fehler beim Prüfen der Admin-Anzahl: " + std::string(_rbErr));}
      return false;
    }
    {
      auto countStmt = makeStatement(rawCount);
      sqlite3_bind_text(countStmt.get(), 1, adminRoleStr.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_int(countStmt.get(), 2, userId);
      if (sqlite3_step(countStmt.get()) != SQLITE_ROW) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Lesen der Admin-Anzahl");
        return false;
      }
      if (sqlite3_column_int(countStmt.get(), 0) == 0) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Letzten aktiven Administrator kann nicht entfernt werden");
        return false;
      }
    }
  }

  const char *updateSQL = "UPDATE users SET role = ? WHERE id = ?;";
  sqlite3_stmt *updateStmtRaw = nullptr;
  rc = sqlite3_prepare_v2(db_, updateSQL, -1, &updateStmtRaw, nullptr);
  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des UPDATE: " +
             std::string(_rbErr));}
    return false;
  }
  auto updateStmt = makeStatement(updateStmtRaw);

  sqlite3_bind_text(updateStmt.get(), 1, roleName.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(updateStmt.get(), 2, userId);

  rc = sqlite3_step(updateStmt.get());
  if (rc != SQLITE_DONE) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Zuweisen der Rolle: " +
             std::string(_rbErr));}
    return false;
  }

  if (sqlite3_changes(db_) == 0) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    setError("Benutzer mit ID " + std::to_string(userId) + " nicht gefunden");
    return false;
  }

  core::AuditEntry entry(
      core::AuditEntry::ActionType::UPDATE,
      core::AuditEntry::EntityType::USER, existing->getUsername(),
      normalizeActor(actor),
      "Rolle: " + existing->getRoleString() + " -> " + roleName);
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

bool Database::upsertApiKey(const std::string &key, bool active,
                             const std::string &role) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return false;
  }

  if (key.empty()) {
    setError("API-Schlüssel darf nicht leer sein");
    return false;
  }

  const std::string effectiveRole = role.empty() ? "OPERATOR" : role;
  const char *upsertSQL = R"(
        INSERT INTO api_keys (key, active, created_date, role)
        VALUES (?, ?, ?, ?)
        ON CONFLICT(key) DO UPDATE SET active = excluded.active, role = excluded.role;
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
  sqlite3_bind_text(stmt.get(), 4, effectiveRole.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Speichern des API-Schlüssels: " +
             std::string(sqlite3_errmsg(db_)));
    return false;
  }

  return true;
}

std::optional<std::string> Database::isApiKeyValid(const std::string &key) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return std::nullopt;
  }

  if (key.empty()) {
    return std::nullopt;
  }

  const char *selectSQL =
      "SELECT active, role FROM api_keys WHERE key = ? LIMIT 1;";
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
  if (rc == SQLITE_ROW) {
    const int active = sqlite3_column_int(stmt.get(), 0);
    if (active == 1) {
      std::string role = columnText(stmt.get(), 1);
      if (role.empty()) role = "OPERATOR";
      return role;
    }
    clearError();
    return std::nullopt;
  }
  if (rc != SQLITE_DONE) {
    setError("Fehler beim Laden des API-Schlüssels: " +
             std::string(sqlite3_errmsg(db_)));
  } else {
    clearError();
  }
  return std::nullopt;
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

  char *errMsg = nullptr;
  int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr,
                        &errMsg);
  if (rc != SQLITE_OK) {
    setError("Fehler beim Starten der Transaktion: " +
             std::string(errMsg ? errMsg : sqlite3_errmsg(db_)));
    sqlite3_free(errMsg);
    return false;
  }

  const std::time_t now = std::time(nullptr);
  const char *updateSQL = R"(
        UPDATE sessions
        SET logout_ts = ?
        WHERE user_id = ? AND logout_ts IS NULL;
    )";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, updateSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des Session-UPDATE: " +
             std::string(_rbErr));}
    return false;
  }

  auto stmt = makeStatement(rawStmt);
  sqlite3_bind_int64(stmt.get(), 1, static_cast<sqlite3_int64>(now));
  sqlite3_bind_int(stmt.get(), 2, userId);

  rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Beenden der Sitzung: " +
             std::string(_rbErr));}
    return false;
  }

  const int closedCount = sqlite3_changes(db_);
  const std::string details =
      closedCount > 0 ? "Sitzung beendet" : "Keine aktive Sitzung gefunden";

  core::AuditEntry entry(
      core::AuditEntry::ActionType::LOGOUT, core::AuditEntry::EntityType::USER,
      username, username, details + (reason.empty() ? "" : " (" + reason + ")"));
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
  const bool closed = endSession(userId, username, "relogin");
  if (!closed) {
    // Fehler behalten, aber Login nicht blockieren.
  } else {
    clearError();
  }

  const std::time_t now = std::time(nullptr);
  const std::string methodStr = method == AuthMethod::LDAP ? "LDAP" : "Lokal";

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
        INSERT INTO sessions (user_id, username, method, login_ts, details)
        VALUES (?, ?, ?, ?, ?);
    )";

  sqlite3_stmt *rawStmt = nullptr;
  rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
  if (rc != SQLITE_OK) {
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Vorbereiten des Session-INSERT: " +
             std::string(_rbErr));}
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
    {const std::string _rbErr = sqlite3_errmsg(db_);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        setError("Fehler beim Starten der Sitzung: " +
             std::string(_rbErr));}
    return false;
  }

  const int sessionId = static_cast<int>(sqlite3_last_insert_rowid(db_));
  const std::string auditDetails =
      "Login erfolgreich (Methode: " + methodStr +
      ", Session: " + std::to_string(sessionId) + ")";

  core::AuditEntry entry(core::AuditEntry::ActionType::LOGIN,
                         core::AuditEntry::EntityType::USER, username,
                         username, auditDetails);
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

std::optional<Database::SessionInfo> Database::getSessionById(int sessionId) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return std::nullopt;
  }

  const char *selectSQL = R"(
        SELECT id, user_id, username, method, login_ts, logout_ts, details
        FROM sessions
        WHERE id = ?
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
  sqlite3_bind_int(stmt.get(), 1, sessionId);
  rc = sqlite3_step(stmt.get());
  if (rc == SQLITE_DONE) {
    setError("Sitzung mit ID " + std::to_string(sessionId) + " nicht gefunden");
    return std::nullopt;
  }
  if (rc != SQLITE_ROW) {
    setError("Fehler beim Laden der Sitzung: " +
             std::string(sqlite3_errmsg(db_)));
    return std::nullopt;
  }

  SessionInfo info;
  info.id = sqlite3_column_int(stmt.get(), 0);
  info.userId = sqlite3_column_int(stmt.get(), 1);
  info.username = columnText(stmt.get(), 2);
  info.method = columnText(stmt.get(), 3);
  info.loginTs = static_cast<std::time_t>(sqlite3_column_int64(stmt.get(), 4));
  if (sqlite3_column_type(stmt.get(), 5) != SQLITE_NULL) {
    info.logoutTs =
        static_cast<std::time_t>(sqlite3_column_int64(stmt.get(), 5));
  }
  info.details = columnText(stmt.get(), 6);
  return info;
}

std::optional<Database::SessionInfo>
Database::getLatestSessionForUser(int userId) {
  clearError();

  if (!isOpen_) {
    setError("Datenbank ist nicht geöffnet");
    return std::nullopt;
  }

  const char *selectSQL = R"(
        SELECT id
        FROM sessions
        WHERE user_id = ?
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
    setError("Keine Sitzung für Benutzer " + std::to_string(userId));
    return std::nullopt;
  }
  if (rc != SQLITE_ROW) {
    setError("Fehler beim Laden der Sitzung: " +
             std::string(sqlite3_errmsg(db_)));
    return std::nullopt;
  }

  const int sessionId = sqlite3_column_int(stmt.get(), 0);
  return getSessionById(sessionId);
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

        {
          core::User tempUser("", storedHash, core::User::Role::OPERATOR);
          if (!tempUser.verifyPassword(password)) {
            const std::string msg = "Ungültiger Benutzername oder Passwort";
            setError(msg);
            logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                          "Login fehlgeschlagen (LDAP, falsches Passwort)");
            setError(msg);
            return result;
          }
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

        if (!localUser->isActive()) {
          const std::string msg = "Benutzer ist deaktiviert";
          setError(msg);
          logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                        "Login fehlgeschlagen (LDAP, deaktiviert)");
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
      if (rc != SQLITE_DONE) {
        const std::string msg = "LDAP-Abfrage fehlgeschlagen";
        setError(msg);
        logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                      "Login fehlgeschlagen (LDAP, Abfragefehler)");
        setError(msg);
        return result;
      }
    } else {
      const std::string msg = "LDAP-Abfrage konnte nicht vorbereitet werden";
      setError(msg);
      logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                    "Login fehlgeschlagen (LDAP, Abfragefehler)");
      setError(msg);
      return result;
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

      const std::string msg = "MFA erforderlich. Bitte MFA-Code eingeben.";
      const std::string prevError = lastError_;
      logUserAction(core::AuditEntry::ActionType::UPDATE, actor, actor,
                    "Login fehlgeschlagen (" + methodStr +
                        ", MFA erforderlich)");
      if (!prevError.empty()) {
        lastError_ = prevError;
      } else if (!lastError_.empty()) {
        clearError();
      }
      setError(msg);
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
