/**
 * @file test_audit_chain.cpp
 * @brief Unit-Tests fuer den kryptographischen Hash-Chain Audit Trail
 *
 * Tests verify that:
 * - A fresh DB returns the genesis hash (64 zeros) from getLastAuditHash()
 * - After addAuditEntry the hash is a valid 64-char hex string (non-zero)
 * - Sequential entries produce distinct hashes (chain is progressive)
 * - verifyAuditChain() returns true on an untampered chain
 * - verifyAuditChain() returns false and reports firstBrokenAt after tampering
 */

#include "db/Database.h"
#include "test_macros.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <sqlite3.h>

using namespace opensylab::db;
using namespace opensylab::core;

namespace {

std::string uniqueAuditDbPath() {
  std::ostringstream ss;
  ss << "test_audit_chain_" << std::rand() << "_" << std::time(nullptr) << ".db";
  return ss.str();
}

// Returns true if s is exactly 64 lowercase hex characters.
bool isHex64(const std::string &s) {
  if (s.size() != 64) return false;
  for (char c : s) {
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  }
  return true;
}

static const std::string kGenesisHash(64, '0');

} // namespace

void registerAuditChainTests() {

  // Test 1: fresh DB -> getLastAuditHash() returns 64 zeros (genesis)
  registerTest("audit_chain_genesis_hash", []() -> bool {
    const std::string path = uniqueAuditDbPath();
    {
      Database db(path);
      ASSERT_TRUE(db.open());
      ASSERT_TRUE(db.initializeSchema());

      const std::string h = db.getLastAuditHash();
      ASSERT_EQ(h, kGenesisHash);
    }
    std::remove(path.c_str());
    return true;
  });

  // Test 2: after one entry -> hash is a 64-char non-zero hex string
  registerTest("audit_chain_first_entry_hash", []() -> bool {
    const std::string path = uniqueAuditDbPath();
    {
      Database db(path);
      ASSERT_TRUE(db.open());
      ASSERT_TRUE(db.initializeSchema());

      AuditEntry e(AuditEntry::ActionType::CREATE,
                   AuditEntry::EntityType::SAMPLE,
                   "S001", "testuser", "first audit entry");
      ASSERT_TRUE(db.logAudit(e));

      const std::string h = db.getLastAuditHash();
      ASSERT_TRUE(isHex64(h));
      ASSERT_NE(h, kGenesisHash);
    }
    std::remove(path.c_str());
    return true;
  });

  // Test 3: two entries -> hashes are different from each other
  registerTest("audit_chain_two_entries_distinct_hashes", []() -> bool {
    const std::string path = uniqueAuditDbPath();
    {
      Database db(path);
      ASSERT_TRUE(db.open());
      ASSERT_TRUE(db.initializeSchema());

      AuditEntry e1(AuditEntry::ActionType::CREATE,
                    AuditEntry::EntityType::SAMPLE,
                    "S001", "alice", "entry one");
      ASSERT_TRUE(db.logAudit(e1));
      const std::string h1 = db.getLastAuditHash();

      AuditEntry e2(AuditEntry::ActionType::UPDATE,
                    AuditEntry::EntityType::SAMPLE,
                    "S001", "bob", "entry two");
      ASSERT_TRUE(db.logAudit(e2));
      const std::string h2 = db.getLastAuditHash();

      ASSERT_TRUE(isHex64(h1));
      ASSERT_TRUE(isHex64(h2));
      ASSERT_NE(h1, h2);
    }
    std::remove(path.c_str());
    return true;
  });

  // Test 4: verifyAuditChain() on an untampered chain -> true
  registerTest("audit_chain_verify_valid_chain", []() -> bool {
    const std::string path = uniqueAuditDbPath();
    {
      Database db(path);
      ASSERT_TRUE(db.open());
      ASSERT_TRUE(db.initializeSchema());

      for (int i = 0; i < 5; ++i) {
        AuditEntry e(AuditEntry::ActionType::CREATE,
                     AuditEntry::EntityType::SAMPLE,
                     "S" + std::to_string(i), "sys",
                     "details " + std::to_string(i));
        ASSERT_TRUE(db.logAudit(e));
      }

      std::string broken;
      const bool ok = db.verifyAuditChain(broken);
      ASSERT_TRUE(ok);
      ASSERT_EQ(broken, std::string(""));
    }
    std::remove(path.c_str());
    return true;
  });

  // Test 5: verifyAuditChain() after tampering -> false + correct firstBrokenAt
  registerTest("audit_chain_detect_tampering", []() -> bool {
    const std::string path = uniqueAuditDbPath();
    {
      Database db(path);
      ASSERT_TRUE(db.open());
      ASSERT_TRUE(db.initializeSchema());

      for (int i = 0; i < 3; ++i) {
        AuditEntry e(AuditEntry::ActionType::UPDATE,
                     AuditEntry::EntityType::ORDER,
                     "O" + std::to_string(i), "hacker",
                     "details");
        ASSERT_TRUE(db.logAudit(e));
      }

      // Sanity: chain valid before tamper
      std::string broken;
      ASSERT_TRUE(db.verifyAuditChain(broken));

      // Tamper: directly modify the second entry's details via raw SQLite
      sqlite3 *rawDb = nullptr;
      ASSERT_EQ(sqlite3_open(path.c_str(), &rawDb), SQLITE_OK);
      const char *tamperSql =
          "UPDATE audit_log SET details = 'TAMPERED' "
          "WHERE id = (SELECT id FROM audit_log ORDER BY id ASC LIMIT 1 OFFSET 1);";
      char *errMsg = nullptr;
      const int rc = sqlite3_exec(rawDb, tamperSql, nullptr, nullptr, &errMsg);
      sqlite3_free(errMsg);
      sqlite3_close(rawDb);
      ASSERT_EQ(rc, SQLITE_OK);

      // Now chain should fail
      broken.clear();
      const bool ok = db.verifyAuditChain(broken);
      ASSERT_FALSE(ok);
      ASSERT_FALSE(broken.empty());
      const bool hasPrefix = (broken.find("entry id=") != std::string::npos);
      ASSERT_TRUE(hasPrefix);
    }
    std::remove(path.c_str());
    return true;
  });

  // Test 6: empty DB -> verifyAuditChain() returns true (no entries = valid)
  registerTest("audit_chain_verify_empty_db", []() -> bool {
    const std::string path = uniqueAuditDbPath();
    {
      Database db(path);
      ASSERT_TRUE(db.open());
      ASSERT_TRUE(db.initializeSchema());

      std::string broken;
      ASSERT_TRUE(db.verifyAuditChain(broken));
    }
    std::remove(path.c_str());
    return true;
  });
}
