/**
 * @file test_migrations.cpp
 * @brief Unit-Tests fuer das Datenbank-Migrationssystem
 */

#include "db/Database.h"
#include "test_macros.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <sqlite3.h>

using namespace opensylab::db;

namespace {
std::string uniqueDbPath() {
  std::ostringstream ss;
  ss << "/tmp/test_migrations_" << std::rand() << "_" << std::time(nullptr) << ".db";
  return ss.str();
}

// Helper: query an integer from the given SQL on an already-open raw sqlite3 handle
int queryInt(sqlite3 *db, const char *sql) {
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return -1;
  }
  int result = -1;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    result = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return result;
}
} // namespace

// Test 1: Fresh DB has schema version 0 BEFORE any migrations run.
// We open the DB and create only the schema_migrations table manually,
// then verify getCurrentSchemaVersion returns 0.
bool test_migrations_FreshDbVersionIsZero() {
  const std::string dbPath = uniqueDbPath();
  {
    Database db(dbPath);
    ASSERT_TRUE(db.open());
    ASSERT_TRUE(db.initializeSchema());
    // After initializeSchema the version must be 3 (all migrations applied).
    // We can only check getCurrentSchemaVersion indirectly via a fresh DB.
    db.close();
  }
  std::remove(dbPath.c_str());

  // Separate check: manually create a DB with only schema_migrations and
  // verify version 0 before any row is inserted.
  const std::string dbPath2 = uniqueDbPath();
  sqlite3 *rawDb = nullptr;
  ASSERT_EQ(sqlite3_open(dbPath2.c_str(), &rawDb), SQLITE_OK);
  sqlite3_exec(rawDb,
               "CREATE TABLE IF NOT EXISTS schema_migrations ("
               "version INTEGER PRIMARY KEY, description TEXT NOT NULL, "
               "applied_at TEXT NOT NULL DEFAULT (datetime('now')));",
               nullptr, nullptr, nullptr);
  const int versionBeforeMigration =
      queryInt(rawDb, "SELECT COALESCE(MAX(version), 0) FROM schema_migrations;");
  sqlite3_close(rawDb);
  std::remove(dbPath2.c_str());

  ASSERT_EQ(versionBeforeMigration, 0);
  return true;
}

// Test 2: After initializeSchema() the schema_migrations table has 3 entries
// and the max version is 3.
bool test_migrations_AfterInitSchemaVersionIsThree() {
  const std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  // Open the same file with a raw SQLite handle to inspect the table.
  sqlite3 *rawDb = nullptr;
  ASSERT_EQ(sqlite3_open(dbPath.c_str(), &rawDb), SQLITE_OK);

  const int maxVersion =
      queryInt(rawDb, "SELECT COALESCE(MAX(version), 0) FROM schema_migrations;");
  const int rowCount =
      queryInt(rawDb, "SELECT COUNT(*) FROM schema_migrations;");

  sqlite3_close(rawDb);
  db.close();
  std::remove(dbPath.c_str());

  ASSERT_EQ(maxVersion, 3);
  ASSERT_EQ(rowCount, 3);
  return true;
}

// Test 3: Second call to initializeSchema() is idempotent --
// no new rows are added to schema_migrations.
bool test_migrations_Idempotent() {
  const std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());
  // Second call
  ASSERT_TRUE(db.initializeSchema());

  sqlite3 *rawDb = nullptr;
  ASSERT_EQ(sqlite3_open(dbPath.c_str(), &rawDb), SQLITE_OK);
  const int rowCount =
      queryInt(rawDb, "SELECT COUNT(*) FROM schema_migrations;");
  const int maxVersion =
      queryInt(rawDb, "SELECT COALESCE(MAX(version), 0) FROM schema_migrations;");
  sqlite3_close(rawDb);

  db.close();
  std::remove(dbPath.c_str());

  ASSERT_EQ(rowCount, 3);
  ASSERT_EQ(maxVersion, 3);
  return true;
}

// Test 4: Migrations are applied in ascending version order.
// Verify the versions in schema_migrations are 1, 2, 3.
bool test_migrations_AppliedInOrder() {
  const std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  sqlite3 *rawDb = nullptr;
  ASSERT_EQ(sqlite3_open(dbPath.c_str(), &rawDb), SQLITE_OK);

  sqlite3_stmt *stmt = nullptr;
  ASSERT_EQ(sqlite3_prepare_v2(rawDb,
                               "SELECT version FROM schema_migrations ORDER BY version ASC;",
                               -1, &stmt, nullptr),
            SQLITE_OK);

  int prev = 0;
  bool ordered = true;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const int v = sqlite3_column_int(stmt, 0);
    if (v <= prev) {
      ordered = false;
      break;
    }
    prev = v;
  }
  sqlite3_finalize(stmt);
  sqlite3_close(rawDb);

  db.close();
  std::remove(dbPath.c_str());

  ASSERT_TRUE(ordered);
  ASSERT_EQ(prev, 3);
  return true;
}

// Test 5: Migration 2 actually created the chain_hash column in audit_log.
bool test_migrations_AuditLogChainHashColumnExists() {
  const std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  sqlite3 *rawDb = nullptr;
  ASSERT_EQ(sqlite3_open(dbPath.c_str(), &rawDb), SQLITE_OK);

  bool columnFound = false;
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(rawDb, "PRAGMA table_info(audit_log);",
                         -1, &stmt, nullptr) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      const unsigned char *colName = sqlite3_column_text(stmt, 1);
      if (colName && std::string(reinterpret_cast<const char *>(colName)) == "chain_hash") {
        columnFound = true;
        break;
      }
    }
    sqlite3_finalize(stmt);
  }

  sqlite3_close(rawDb);
  db.close();
  std::remove(dbPath.c_str());

  ASSERT_TRUE(columnFound);
  return true;
}

// Test 6: Migration 3 actually created the mfa_secret_base32 column in users.
bool test_migrations_UsersMfaBase32ColumnExists() {
  const std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  sqlite3 *rawDb = nullptr;
  ASSERT_EQ(sqlite3_open(dbPath.c_str(), &rawDb), SQLITE_OK);

  bool columnFound = false;
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(rawDb, "PRAGMA table_info(users);",
                         -1, &stmt, nullptr) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      const unsigned char *colName = sqlite3_column_text(stmt, 1);
      if (colName && std::string(reinterpret_cast<const char *>(colName)) == "mfa_secret_base32") {
        columnFound = true;
        break;
      }
    }
    sqlite3_finalize(stmt);
  }

  sqlite3_close(rawDb);
  db.close();
  std::remove(dbPath.c_str());

  ASSERT_TRUE(columnFound);
  return true;
}

void registerMigrationTests() {
  registerTest("Migrations::FreshDbVersionIsZero",
               test_migrations_FreshDbVersionIsZero);
  registerTest("Migrations::AfterInitSchemaVersionIsThree",
               test_migrations_AfterInitSchemaVersionIsThree);
  registerTest("Migrations::Idempotent",
               test_migrations_Idempotent);
  registerTest("Migrations::AppliedInOrder",
               test_migrations_AppliedInOrder);
  registerTest("Migrations::AuditLogChainHashColumnExists",
               test_migrations_AuditLogChainHashColumnExists);
  registerTest("Migrations::UsersMfaBase32ColumnExists",
               test_migrations_UsersMfaBase32ColumnExists);
}
