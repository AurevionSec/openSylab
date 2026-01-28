/**
 * @file test_database.cpp
 * @brief Unit-Tests für die Database-Klasse
 */

#include "db/Database.h"
#include "test_macros.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>

using namespace opensylab::db;
using namespace opensylab::core;

namespace {
// Generiert eindeutigen Dateinamen für parallele Tests
std::string uniqueDbPath() {
  std::ostringstream ss;
  ss << "test_db_" << std::rand() << "_" << std::time(nullptr) << ".db";
  return ss.str();
}

int computeMfaCode(const std::string &secret, std::time_t now) {
  const long step = static_cast<long>(now / 30);
  std::string material = secret + ":" + std::to_string(step);

  unsigned long hash = 5381;
  for (unsigned char c : material) {
    hash = ((hash << 5) + hash) ^ c;
  }
  return static_cast<int>(hash % 1000000UL);
}

const AuditEntry *findAuditEntry(
    const std::vector<std::unique_ptr<AuditEntry>> &entries,
    AuditEntry::ActionType action, AuditEntry::EntityType entity,
    const std::string &entityId = "", const std::string &user = "") {
  for (const auto &entry : entries) {
    if (!entry) {
      continue;
    }
    if (entry->getAction() != action) {
      continue;
    }
    if (entry->getEntity() != entity) {
      continue;
    }
    if (!entityId.empty() && entry->getEntityId() != entityId) {
      continue;
    }
    if (!user.empty() && entry->getUser() != user) {
      continue;
    }
    return entry.get();
  }
  return nullptr;
}
} // namespace

bool test_database_OpenAndClose() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.isOpen());
  ASSERT_TRUE(db.close());
  ASSERT_FALSE(db.isOpen());
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_InitializeSchema() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());
  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_CreateSample() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("TEST001", "P001");
  sample.setPatientName("Test Patient");
  ASSERT_TRUE(db.createSample(sample));

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_GetSampleByBarcode() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("TEST002", "P002");
  ASSERT_TRUE(db.createSample(sample));

  auto retrieved = db.getSampleByBarcode("TEST002");
  ASSERT_NOT_NULL(retrieved);
  ASSERT_EQ(retrieved->getSampleId(), "TEST002");
  ASSERT_EQ(retrieved->getPatientId(), "P002");

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_GetAllSamples() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  // Mehrere Proben anlegen
  Sample s1("S001", "P001");
  Sample s2("S002", "P002");
  Sample s3("S003", "P003");

  ASSERT_TRUE(db.createSample(s1));
  ASSERT_TRUE(db.createSample(s2));
  ASSERT_TRUE(db.createSample(s3));

  auto samples = db.getAllSamples();
  ASSERT_FALSE(db.hasError()); // Kein Fehler
  ASSERT_EQ(samples.size(), static_cast<size_t>(3));

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_GetAllSamples_EmptyDatabase() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  auto samples = db.getAllSamples();
  ASSERT_FALSE(db.hasError()); // Kein Fehler, nur leer
  ASSERT_EQ(samples.size(), static_cast<size_t>(0));

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_GetSamplesByFilter() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample s1("S001", "P001");
  s1.setPatientName("Alice");
  s1.setRegistrationDate(1000);
  ASSERT_TRUE(db.createSample(s1));

  Sample s2("S002", "P002");
  s2.setPatientName("Bob");
  s2.setStatus(Sample::Status::VALIDATED);
  s2.setRegistrationDate(2000);
  ASSERT_TRUE(db.createSample(s2));

  Sample s3("XYZ", "P003");
  s3.setPatientName("Alice B");
  s3.setRegistrationDate(3000);
  ASSERT_TRUE(db.createSample(s3));

  Database::SampleFilter queryFilter;
  queryFilter.query = "Alice";
  auto byQuery = db.getSamplesByFilter(queryFilter);
  ASSERT_FALSE(db.hasError());
  ASSERT_EQ(byQuery.size(), static_cast<size_t>(2));

  Database::SampleFilter statusFilter;
  statusFilter.status = Sample::statusToString(Sample::Status::VALIDATED);
  auto byStatus = db.getSamplesByFilter(statusFilter);
  ASSERT_FALSE(db.hasError());
  ASSERT_EQ(byStatus.size(), static_cast<size_t>(1));
  ASSERT_EQ(byStatus[0]->getSampleId(), "S002");

  Database::SampleFilter dateFilter;
  dateFilter.fromDate = 1500;
  dateFilter.toDate = 2500;
  auto byDate = db.getSamplesByFilter(dateFilter);
  ASSERT_FALSE(db.hasError());
  ASSERT_EQ(byDate.size(), static_cast<size_t>(1));
  ASSERT_EQ(byDate[0]->getSampleId(), "S002");

  Sample s4("%_ID", "P004");
  ASSERT_TRUE(db.createSample(s4));

  Database::SampleFilter literalFilter;
  literalFilter.query = "%_";
  auto byLiteral = db.getSamplesByFilter(literalFilter);
  ASSERT_FALSE(db.hasError());
  ASSERT_EQ(byLiteral.size(), static_cast<size_t>(1));
  ASSERT_EQ(byLiteral[0]->getSampleId(), "%_ID");

  Sample s5("ARCH001", "P005");
  s5.setStatus(Sample::Status::ARCHIVED);
  ASSERT_TRUE(db.createSample(s5));

  Database::SampleFilter excludeArchived;
  excludeArchived.excludeArchived = true;
  auto withoutArchived = db.getSamplesByFilter(excludeArchived);
  ASSERT_FALSE(db.hasError());
  ASSERT_EQ(withoutArchived.size(), static_cast<size_t>(4));

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_UpdateSample() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("TEST003", "P003");
  ASSERT_TRUE(db.createSample(sample));

  auto retrieved = db.getSampleByBarcode("TEST003");
  retrieved->setStatus(Sample::Status::VALIDATED);
  ASSERT_TRUE(db.updateSample(*retrieved));

  auto updated = db.getSampleByBarcode("TEST003");
  ASSERT_EQ(updated->getStatus(), Sample::Status::VALIDATED);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_LogSampleStatusUpdate() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("AUDIT001", "P010");
  ASSERT_TRUE(db.createSample(sample));

  auto retrieved = db.getSampleByBarcode("AUDIT001");
  ASSERT_NOT_NULL(retrieved);
  retrieved->setStatus(Sample::Status::VALIDATED);
  ASSERT_TRUE(db.updateSample(*retrieved));

  db.logSampleAction(AuditEntry::ActionType::UPDATE, "AUDIT001", "tester",
                     "Status: Erfasst -> Validiert");

  auto entries = db.getAuditLogByEntity(AuditEntry::EntityType::SAMPLE,
                                        "AUDIT001");
  ASSERT_FALSE(db.hasError());
  ASSERT_FALSE(entries.empty());
  const AuditEntry *entry =
      findAuditEntry(entries, AuditEntry::ActionType::UPDATE,
                     AuditEntry::EntityType::SAMPLE, "AUDIT001", "tester");
  ASSERT_NOT_NULL(entry);
  ASSERT_EQ(entry->getDetails(), "Status: Erfasst -> Validiert");

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_UpdateSample_NoChangesStillSuccess() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("UNCHANGED", "P004");
  ASSERT_TRUE(db.createSample(sample));

  auto retrieved = db.getSampleByBarcode("UNCHANGED");
  ASSERT_NOT_NULL(retrieved);
  ASSERT_TRUE(db.updateSample(
      *retrieved)); // Keine Änderungen, sollte trotzdem Erfolg sein
  ASSERT_FALSE(db.hasError());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_UpdateSample_NotFound() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("MISSING", "P999");
  sample.setId(9999); // Nicht vorhandene ID
  ASSERT_FALSE(db.updateSample(sample));
  ASSERT_TRUE(db.hasError());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_DeleteSample() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  // Probe anlegen
  Sample sample("TEST004", "P004");
  ASSERT_TRUE(db.createSample(sample));

  // Probe abrufen um ID zu bekommen
  auto retrieved = db.getSampleByBarcode("TEST004");
  ASSERT_NOT_NULL(retrieved);
  int sampleId = retrieved->getId();

  // Probe löschen
  ASSERT_TRUE(db.deleteSample(sampleId));

  // Prüfen dass Probe nicht mehr existiert
  auto deleted = db.getSampleByBarcode("TEST004");
  ASSERT_NULL(deleted);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_DeleteSample_NotFound() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  // Versuche nicht existierende Probe zu löschen
  ASSERT_FALSE(db.deleteSample(999999));
  ASSERT_TRUE(db.hasError());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_LogSampleDelete() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("DEL001", "P011");
  ASSERT_TRUE(db.createSample(sample));

  db.logSampleAction(AuditEntry::ActionType::DELETE, "DEL001", "tester",
                     "Sample gelöscht");

  auto entries =
      db.getAuditLogByEntity(AuditEntry::EntityType::SAMPLE, "DEL001");
  ASSERT_FALSE(db.hasError());
  ASSERT_FALSE(entries.empty());
  const AuditEntry *entry =
      findAuditEntry(entries, AuditEntry::ActionType::DELETE,
                     AuditEntry::EntityType::SAMPLE, "DEL001");
  ASSERT_NOT_NULL(entry);
  ASSERT_EQ(entry->getDetails(), "Sample gelöscht");

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_AuditLogsSampleCrud() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("AUDIT_CRUD", "P900");
  sample.setPatientName("Audit Patient");
  ASSERT_TRUE(db.createSample(sample, "tester"));

  auto createdEntries =
      db.getAuditLogByEntity(AuditEntry::EntityType::SAMPLE, "AUDIT_CRUD");
  ASSERT_FALSE(createdEntries.empty());
  const AuditEntry *createdEntry =
      findAuditEntry(createdEntries, AuditEntry::ActionType::CREATE,
                     AuditEntry::EntityType::SAMPLE, "AUDIT_CRUD", "tester");
  ASSERT_NOT_NULL(createdEntry);

  auto retrieved = db.getSampleByBarcode("AUDIT_CRUD");
  ASSERT_NOT_NULL(retrieved);
  retrieved->setStatus(Sample::Status::VALIDATED);
  ASSERT_TRUE(db.updateSample(*retrieved, "tester"));

  auto updatedEntries =
      db.getAuditLogByEntity(AuditEntry::EntityType::SAMPLE, "AUDIT_CRUD");
  ASSERT_FALSE(updatedEntries.empty());
  const AuditEntry *updatedEntry =
      findAuditEntry(updatedEntries, AuditEntry::ActionType::UPDATE,
                     AuditEntry::EntityType::SAMPLE, "AUDIT_CRUD", "tester");
  ASSERT_NOT_NULL(updatedEntry);
  ASSERT_NE(updatedEntry->getDetails().find("Status"), std::string::npos);
  ASSERT_NE(updatedEntry->getDetails().find("->"), std::string::npos);

  ASSERT_TRUE(db.deleteSample(retrieved->getId(), "tester"));

  auto deletedEntries =
      db.getAuditLogByEntity(AuditEntry::EntityType::SAMPLE, "AUDIT_CRUD");
  ASSERT_FALSE(deletedEntries.empty());
  const AuditEntry *deletedEntry =
      findAuditEntry(deletedEntries, AuditEntry::ActionType::DELETE,
                     AuditEntry::EntityType::SAMPLE, "AUDIT_CRUD", "tester");
  ASSERT_NOT_NULL(deletedEntry);
  ASSERT_NE(deletedEntry->getDetails().find("Proben-ID"), std::string::npos);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_CreateOrder_RequestedDateStored() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("ORD_SAMPLE", "P100");
  ASSERT_TRUE(db.createSample(sample));

  Order order("ORD001", "ORD_SAMPLE", "Blutbild");
  std::tm tm = {};
  tm.tm_year = 2026 - 1900;
  tm.tm_mon = 0;
  tm.tm_mday = 15;
  std::time_t requestedDate = std::mktime(&tm);
  order.setRequestedDate(requestedDate);

  ASSERT_TRUE(db.createOrder(order));

  auto stored = db.getOrderByOrderId("ORD001");
  ASSERT_NOT_NULL(stored);
  ASSERT_EQ(stored->getRequestedDate(), requestedDate);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_CreateOrder_MissingFieldsRejected() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Order missingOrderId("", "S001", "Blutbild");
  ASSERT_FALSE(db.createOrder(missingOrderId));
  ASSERT_FALSE(db.getLastError().empty());

  Order missingSampleId("ORD002", "", "Blutbild");
  ASSERT_FALSE(db.createOrder(missingSampleId));
  ASSERT_FALSE(db.getLastError().empty());

  Order missingTestType("ORD003", "S001", "");
  ASSERT_FALSE(db.createOrder(missingTestType));
  ASSERT_FALSE(db.getLastError().empty());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_NoAuditOnFailedOrderCreate() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Order missingOrderId("", "S001", "Blutbild");
  ASSERT_FALSE(db.createOrder(missingOrderId, "tester"));

  auto entries = db.getAuditLog();
  ASSERT_TRUE(entries.empty());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_CreateTestResult_Valid() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("RES_SAMPLE", "P300");
  ASSERT_TRUE(db.createSample(sample));

  Order order("ORD_RES_1", "RES_SAMPLE", "Glucose");
  ASSERT_TRUE(db.createOrder(order));

  auto storedOrder = db.getOrderByOrderId("ORD_RES_1");
  ASSERT_NOT_NULL(storedOrder);
  int orderDbId = storedOrder->getId();

  TestResult result("RES001", orderDbId, "Glucose");
  result.setValue("98");
  result.setUnit("mg/dL");
  result.setReferenceRange("70-100");
  result.setReferenceLow(70);
  result.setReferenceHigh(100);
  result.setStatus(TestResult::Status::ENTERED);
  result.setFlag(TestResult::Flag::NORMAL);
  result.setMeasuredDate(123456789);

  ASSERT_TRUE(db.createTestResult(result));

  auto stored = db.getTestResultByResultId("RES001");
  ASSERT_NOT_NULL(stored);
  ASSERT_EQ(stored->getOrderId(), orderDbId);
  ASSERT_EQ(stored->getTestParameter(), "Glucose");
  ASSERT_EQ(stored->getValue(), "98");
  ASSERT_EQ(stored->getUnit(), "mg/dL");
  ASSERT_EQ(stored->getReferenceRange(), "70-100");
  ASSERT_EQ(stored->getReferenceLow(), 70);
  ASSERT_EQ(stored->getReferenceHigh(), 100);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_CreateTestResult_MissingFieldsRejected() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("RES_SAMPLE_2", "P301");
  ASSERT_TRUE(db.createSample(sample));

  Order order("ORD_RES_2", "RES_SAMPLE_2", "PCR");
  ASSERT_TRUE(db.createOrder(order));

  auto storedOrder = db.getOrderByOrderId("ORD_RES_2");
  ASSERT_NOT_NULL(storedOrder);
  int orderDbId = storedOrder->getId();

  TestResult missingResultId("", orderDbId, "PCR");
  missingResultId.setValue("1.2");
  missingResultId.setUnit("mg/L");
  ASSERT_FALSE(db.createTestResult(missingResultId));
  ASSERT_FALSE(db.getLastError().empty());

  TestResult missingParameter("RES_MISS_PARAM", orderDbId, "");
  missingParameter.setValue("1.2");
  missingParameter.setUnit("mg/L");
  ASSERT_FALSE(db.createTestResult(missingParameter));
  ASSERT_FALSE(db.getLastError().empty());

  TestResult missingValue("RES_MISS_VALUE", orderDbId, "PCR");
  missingValue.setValue("");
  missingValue.setUnit("mg/L");
  ASSERT_FALSE(db.createTestResult(missingValue));
  ASSERT_FALSE(db.getLastError().empty());

  TestResult missingUnit("RES_MISS_UNIT", orderDbId, "PCR");
  missingUnit.setValue("1.2");
  missingUnit.setUnit("");
  ASSERT_FALSE(db.createTestResult(missingUnit));
  ASSERT_FALSE(db.getLastError().empty());

  TestResult missingOrder("RES_BAD_ORDER", orderDbId + 999, "PCR");
  missingOrder.setValue("1.2");
  missingOrder.setUnit("mg/L");
  ASSERT_FALSE(db.createTestResult(missingOrder));
  ASSERT_FALSE(db.getLastError().empty());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_ValidateResultLogsAudit() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("RES_AUDIT", "P302");
  ASSERT_TRUE(db.createSample(sample));

  Order order("ORD_RES_AUDIT", "RES_AUDIT", "PCR");
  ASSERT_TRUE(db.createOrder(order));

  auto storedOrder = db.getOrderByOrderId("ORD_RES_AUDIT");
  ASSERT_NOT_NULL(storedOrder);
  int orderDbId = storedOrder->getId();

  TestResult result("RES_AUDIT_1", orderDbId, "PCR");
  result.setValue("1.0");
  result.setUnit("mg/L");
  result.setStatus(TestResult::Status::ENTERED);
  result.setFlag(result.evaluateFlag());
  ASSERT_TRUE(db.createTestResult(result));

  auto storedResult = db.getTestResultByResultId("RES_AUDIT_1");
  ASSERT_NOT_NULL(storedResult);
  ASSERT_TRUE(db.validateTestResult("RES_AUDIT_1", "tester"));

  auto validated = db.getTestResultByResultId("RES_AUDIT_1");
  ASSERT_NOT_NULL(validated);
  ASSERT_EQ(validated->getStatus(), TestResult::Status::VALIDATED);

  auto entries =
      db.getAuditLogByEntity(AuditEntry::EntityType::RESULT, "RES_AUDIT_1");
  ASSERT_FALSE(db.hasError());
  ASSERT_FALSE(entries.empty());
  const AuditEntry *entry =
      findAuditEntry(entries, AuditEntry::ActionType::VALIDATE,
                     AuditEntry::EntityType::RESULT, "RES_AUDIT_1", "tester");
  ASSERT_NOT_NULL(entry);
  ASSERT_EQ(entry->getDetails(), "Status: Eingegeben -> Validiert");

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_LogResultRetryImportAudit() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("RES_RETRY", "P307");
  ASSERT_TRUE(db.createSample(sample));

  Order order("ORD_RES_RETRY", "RES_RETRY", "PCR");
  ASSERT_TRUE(db.createOrder(order));

  auto storedOrder = db.getOrderByOrderId("ORD_RES_RETRY");
  ASSERT_NOT_NULL(storedOrder);
  int orderDbId = storedOrder->getId();

  TestResult r1("RES_RETRY_1", orderDbId, "PCR");
  r1.setValue("1.0");
  r1.setUnit("mg/L");
  r1.setStatus(TestResult::Status::ENTERED);
  r1.setFlag(r1.evaluateFlag());
  ASSERT_TRUE(db.createTestResult(r1));

  TestResult r2("RES_RETRY_2", orderDbId, "PCR");
  r2.setValue("1.1");
  r2.setUnit("mg/L");
  r2.setStatus(TestResult::Status::ENTERED);
  r2.setFlag(r2.evaluateFlag());
  ASSERT_TRUE(db.createTestResult(r2));

  std::vector<std::string> resultIds = {"RES_RETRY_1", "RES_RETRY_2"};
  db.logResultRetryImport(resultIds, "tester", "retry_results.csv");

  auto entries =
      db.getAuditLogByEntity(AuditEntry::EntityType::RESULT, "RES_RETRY_1");
  ASSERT_FALSE(entries.empty());
  const AuditEntry *entry =
      findAuditEntry(entries, AuditEntry::ActionType::UPDATE,
                     AuditEntry::EntityType::RESULT, "RES_RETRY_1", "tester");
  ASSERT_NOT_NULL(entry);
  ASSERT_EQ(entry->getDetails(),
            "Retry-Import: retry_results.csv; Anzahl: 2");

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_LogUserAction() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  User user("admin1", User::hashPassword("secret"),
            User::Role::ADMIN);
  ASSERT_TRUE(db.createUser(user));

  db.logUserAction(AuditEntry::ActionType::UPDATE, "admin1", "system",
                   "Benutzer aktualisiert");

  auto entries =
      db.getAuditLogByEntity(AuditEntry::EntityType::USER, "admin1");
  ASSERT_FALSE(entries.empty());
  const AuditEntry *entry =
      findAuditEntry(entries, AuditEntry::ActionType::UPDATE,
                     AuditEntry::EntityType::USER, "admin1", "system");
  ASSERT_NOT_NULL(entry);
  ASSERT_EQ(entry->getDetails(), "Benutzer aktualisiert");

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_CreateRoleAndPermissions() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  const std::string roleName = "QA";
  const std::vector<std::string> perms = {"results.validate", "samples.read"};
  ASSERT_TRUE(db.createRole(roleName, perms));

  auto storedPerms = db.getRolePermissions(roleName);
  ASSERT_FALSE(db.hasError());
  std::sort(storedPerms.begin(), storedPerms.end());

  auto expected = perms;
  std::sort(expected.begin(), expected.end());

  ASSERT_EQ(storedPerms.size(), expected.size());
  ASSERT_EQ(storedPerms[0], expected[0]);
  ASSERT_EQ(storedPerms[1], expected[1]);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_UpdateRolePermissions() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  const std::string roleName = "Supervisor";
  ASSERT_TRUE(db.createRole(roleName, {"samples.read"}));

  const std::vector<std::string> updated = {"orders.update", "results.read"};
  ASSERT_TRUE(db.updateRole(roleName, updated));

  auto storedPerms = db.getRolePermissions(roleName);
  std::sort(storedPerms.begin(), storedPerms.end());
  auto expected = updated;
  std::sort(expected.begin(), expected.end());

  ASSERT_EQ(storedPerms.size(), expected.size());
  ASSERT_EQ(storedPerms[0], expected[0]);
  ASSERT_EQ(storedPerms[1], expected[1]);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_AssignUserRoleCustom() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  const std::string roleName = "QA";
  ASSERT_TRUE(db.createRole(roleName, {"results.validate"}));

  User user("qa_user", User::hashPassword("secret"), User::Role::OPERATOR);
  ASSERT_TRUE(db.createUser(user));

  auto stored = db.getUserByUsername("qa_user");
  ASSERT_NOT_NULL(stored);

  ASSERT_TRUE(db.assignUserRole(stored->getId(), roleName));

  auto updated = db.getUser(stored->getId());
  ASSERT_NOT_NULL(updated);
  ASSERT_EQ(updated->getRoleString(), roleName);
  ASSERT_EQ(updated->getRole(), User::Role::CUSTOM);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_LogRoleAction() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  const std::string roleName = "QA";
  ASSERT_TRUE(db.createRole(roleName, {"results.validate"}));

  db.logRoleAction(AuditEntry::ActionType::UPDATE, roleName, "admin",
                   "Rolle aktualisiert");

  auto entries = db.getAuditLogByEntity(AuditEntry::EntityType::ROLE, roleName);
  ASSERT_FALSE(entries.empty());
  const AuditEntry *entry =
      findAuditEntry(entries, AuditEntry::ActionType::UPDATE,
                     AuditEntry::EntityType::ROLE, roleName, "admin");
  ASSERT_NOT_NULL(entry);
  ASSERT_EQ(entry->getDetails(), "Rolle aktualisiert");

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_AuditLogFiltering() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  AuditEntry e1(AuditEntry::ActionType::CREATE,
                AuditEntry::EntityType::SAMPLE, "S1", "userA", "one");
  e1.setTimestamp(1000);
  ASSERT_TRUE(db.logAudit(e1));

  AuditEntry e2(AuditEntry::ActionType::UPDATE, AuditEntry::EntityType::ORDER,
                "O1", "userB", "two");
  e2.setTimestamp(2000);
  ASSERT_TRUE(db.logAudit(e2));

  AuditEntry e3(AuditEntry::ActionType::DELETE, AuditEntry::EntityType::RESULT,
                "R1", "userA", "three");
  e3.setTimestamp(3000);
  ASSERT_TRUE(db.logAudit(e3));

  Database::AuditLogFilter byUser;
  byUser.user = "userA";
  auto userEntries = db.getAuditLogFiltered(byUser);
  ASSERT_EQ(userEntries.size(), static_cast<size_t>(2));

  Database::AuditLogFilter byAction;
  byAction.action = AuditEntry::ActionType::UPDATE;
  auto actionEntries = db.getAuditLogFiltered(byAction);
  ASSERT_EQ(actionEntries.size(), static_cast<size_t>(1));
  ASSERT_EQ(actionEntries[0]->getEntityId(), "O1");

  Database::AuditLogFilter byEntity;
  byEntity.entity = AuditEntry::EntityType::SAMPLE;
  auto entityEntries = db.getAuditLogFiltered(byEntity);
  ASSERT_EQ(entityEntries.size(), static_cast<size_t>(1));
  ASSERT_EQ(entityEntries[0]->getEntityId(), "S1");

  Database::AuditLogFilter byTime;
  byTime.fromTime = 1500;
  byTime.toTime = 3500;
  auto timeEntries = db.getAuditLogFiltered(byTime);
  ASSERT_EQ(timeEntries.size(), static_cast<size_t>(2));

  Database::AuditLogFilter combo;
  combo.user = "userA";
  combo.entity = AuditEntry::EntityType::RESULT;
  auto comboEntries = db.getAuditLogFiltered(combo);
  ASSERT_EQ(comboEntries.size(), static_cast<size_t>(1));
  ASSERT_EQ(comboEntries[0]->getEntityId(), "R1");

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_AuditLogFilterReset() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  AuditEntry e1(AuditEntry::ActionType::CREATE,
                AuditEntry::EntityType::SAMPLE, "S1", "userA", "one");
  e1.setTimestamp(1000);
  ASSERT_TRUE(db.logAudit(e1));

  AuditEntry e2(AuditEntry::ActionType::UPDATE, AuditEntry::EntityType::ORDER,
                "O1", "userB", "two");
  e2.setTimestamp(2000);
  ASSERT_TRUE(db.logAudit(e2));

  Database::AuditLogFilter emptyFilter;
  emptyFilter.limit = 50;
  auto filtered = db.getAuditLogFiltered(emptyFilter);
  auto unfiltered = db.getAuditLog(50);
  ASSERT_EQ(filtered.size(), unfiltered.size());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_AuditLogExport_CsvOutput() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  AuditEntry e1(AuditEntry::ActionType::CREATE,
                AuditEntry::EntityType::SAMPLE, "EXP_S1", "alice", "one");
  e1.setTimestamp(1000);
  ASSERT_TRUE(db.logAudit(e1));

  AuditEntry e2(AuditEntry::ActionType::DELETE,
                AuditEntry::EntityType::ORDER, "EXP_O1", "bob", "two");
  e2.setTimestamp(2000);
  ASSERT_TRUE(db.logAudit(e2));

  Database::AuditLogFilter filter;
  filter.limit = 100;
  int exported = 0;
  std::string exportPath = "test_export_audit_log.csv";
  ASSERT_TRUE(
      db.exportAuditLogToCsv(exportPath, filter, "admin", exported));
  ASSERT_EQ(exported, 2);

  std::ifstream input(exportPath);
  ASSERT_TRUE(input.is_open());

  std::string header;
  std::getline(input, header);
  ASSERT_EQ(header,
            "id,action,entity,entity_id,user,timestamp,details");

  int rows = 0;
  std::string row;
  while (std::getline(input, row)) {
    if (!row.empty()) {
      rows++;
    }
  }
  ASSERT_EQ(rows, 2);

  input.close();
  std::remove(exportPath.c_str());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_AuditLogExport_FilteredAndLogged() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  AuditEntry e1(AuditEntry::ActionType::UPDATE,
                AuditEntry::EntityType::RESULT, "EXP_R1", "alice", "one");
  e1.setTimestamp(1000);
  ASSERT_TRUE(db.logAudit(e1));

  AuditEntry e2(AuditEntry::ActionType::UPDATE,
                AuditEntry::EntityType::RESULT, "EXP_R2", "bob", "two");
  e2.setTimestamp(2000);
  ASSERT_TRUE(db.logAudit(e2));

  Database::AuditLogFilter filter;
  filter.user = "alice";
  filter.limit = 100;
  int exported = 0;
  std::string exportPath = "test_export_audit_filtered.csv";
  ASSERT_TRUE(
      db.exportAuditLogToCsv(exportPath, filter, "admin", exported));
  ASSERT_EQ(exported, 1);

  std::ifstream input(exportPath);
  ASSERT_TRUE(input.is_open());
  std::string header;
  std::getline(input, header);
  std::string row;
  std::getline(input, row);
  ASSERT_FALSE(row.empty());
  ASSERT_NE(row.find("alice"), std::string::npos);
  ASSERT_EQ(row.find("bob"), std::string::npos);
  input.close();

  auto entries =
      db.getAuditLogByEntity(AuditEntry::EntityType::SYSTEM, "audit_log");
  ASSERT_FALSE(entries.empty());
  const AuditEntry *entry =
      findAuditEntry(entries, AuditEntry::ActionType::UPDATE,
                     AuditEntry::EntityType::SYSTEM, "audit_log", "admin");
  ASSERT_NOT_NULL(entry);
  ASSERT_NE(entry->getDetails().find("Export: " + exportPath),
            std::string::npos);
  ASSERT_NE(entry->getDetails().find("Anzahl: 1"),
            std::string::npos);

  std::remove(exportPath.c_str());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_RetentionMinEnforced() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  ASSERT_EQ(db.getRetentionDays(), 180);

  ASSERT_TRUE(db.setRetentionDays(30));
  ASSERT_EQ(db.getRetentionDays(), 180);

  ASSERT_TRUE(db.setRetentionDays(365));
  ASSERT_EQ(db.getRetentionDays(), 365);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_AuditRetentionPurgesAndLogs() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  ASSERT_TRUE(db.setRetentionDays(180));

  const std::time_t now = std::time(nullptr);
  const std::time_t oldTs =
      now - static_cast<std::time_t>(200 * 24 * 60 * 60);
  const std::time_t recentTs =
      now - static_cast<std::time_t>(10 * 24 * 60 * 60);

  AuditEntry old1(AuditEntry::ActionType::CREATE,
                  AuditEntry::EntityType::SAMPLE, "RET_OLD_1", "tester",
                  "old");
  old1.setTimestamp(oldTs);
  ASSERT_TRUE(db.logAudit(old1));

  AuditEntry old2(AuditEntry::ActionType::UPDATE,
                  AuditEntry::EntityType::ORDER, "RET_OLD_2", "tester",
                  "old");
  old2.setTimestamp(oldTs);
  ASSERT_TRUE(db.logAudit(old2));

  AuditEntry recent(AuditEntry::ActionType::DELETE,
                    AuditEntry::EntityType::RESULT, "RET_NEW_1", "tester",
                    "recent");
  recent.setTimestamp(recentTs);
  ASSERT_TRUE(db.logAudit(recent));

  int purged = 0;
  ASSERT_TRUE(db.applyAuditRetention("admin", purged));
  ASSERT_EQ(purged, 2);

  auto entries = db.getAuditLog(50);
  const AuditEntry *retentionEntry =
      findAuditEntry(entries, AuditEntry::ActionType::DELETE,
                     AuditEntry::EntityType::SYSTEM, "audit_log", "admin");
  ASSERT_NOT_NULL(retentionEntry);

  const AuditEntry *recentEntry =
      findAuditEntry(entries, AuditEntry::ActionType::DELETE,
                     AuditEntry::EntityType::RESULT, "RET_NEW_1", "tester");
  ASSERT_NOT_NULL(recentEntry);

  const AuditEntry *oldEntry =
      findAuditEntry(entries, AuditEntry::ActionType::CREATE,
                     AuditEntry::EntityType::SAMPLE, "RET_OLD_1", "tester");
  ASSERT_TRUE(oldEntry == nullptr);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_AuthenticateUserRejectsInactive() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  User user("inactive1", User::hashPassword("secret"),
            User::Role::OPERATOR);
  user.setActive(false);
  ASSERT_TRUE(db.createUser(user));

  auto auth = db.authenticateUser("inactive1", "secret");
  ASSERT_TRUE(auth == nullptr);
  ASSERT_FALSE(db.getLastError().empty());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_AuthConfigLdapEnabled() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  ASSERT_TRUE(db.setLdapEnabled(true));
  ASSERT_TRUE(db.isLdapEnabled());

  ASSERT_TRUE(db.setLdapEnabled(false));
  ASSERT_FALSE(db.isLdapEnabled());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_LdapAuthenticationPath() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  ASSERT_TRUE(db.setLdapEnabled(true));

  const std::string username = "ldap_user";
  const std::string password = "secret";
  ASSERT_TRUE(db.upsertLdapUser(username, User::hashPassword(password), true,
                                false, ""));

  auto user = db.authenticateUser(username, password);
  ASSERT_NOT_NULL(user);
  ASSERT_EQ(user->getUsername(), username);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_MfaRequiredFlowLocalUser() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  const std::string username = "mfa_local";
  const std::string password = "secret";
  const std::string secret = "LOCAL_MFA_SECRET";

  User user(username, User::hashPassword(password), User::Role::OPERATOR);
  ASSERT_TRUE(db.createUser(user));

  ASSERT_TRUE(db.setUserMfaRequirement(username, true, secret));

  auto missing = db.authenticateUser(username, password);
  ASSERT_TRUE(missing == nullptr);
  ASSERT_NE(db.getLastError().find("MFA"), std::string::npos);

  const int code = computeMfaCode(secret, std::time(nullptr));
  auto ok = db.authenticateUser(username, password, std::to_string(code));
  ASSERT_NOT_NULL(ok);
  ASSERT_EQ(ok->getUsername(), username);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_AuthAuditLogging() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  const std::string username = "audit_auth";
  const std::string password = "secret";

  User user(username, User::hashPassword(password), User::Role::ADMIN);
  ASSERT_TRUE(db.createUser(user));

  auto fail = db.authenticateUser(username, "wrong");
  ASSERT_TRUE(fail == nullptr);

  auto success = db.authenticateUser(username, password);
  ASSERT_NOT_NULL(success);

  auto entries =
      db.getAuditLogByEntity(AuditEntry::EntityType::USER, username);
  ASSERT_FALSE(entries.empty());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_SessionStartAndEnd() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  const std::string username = "session_user";
  const std::string password = "secret";

  User user(username, User::hashPassword(password), User::Role::OPERATOR);
  ASSERT_TRUE(db.createUser(user));

  auto auth = db.authenticateUser(username, password);
  ASSERT_NOT_NULL(auth);

  const int userId = auth->getId();
  ASSERT_TRUE(db.hasActiveSession(userId));
  ASSERT_EQ(db.getActiveSessionCount(userId), 1);
  ASSERT_EQ(db.getSessionCount(userId), 1);
  ASSERT_TRUE(db.getActiveSessionId(userId).has_value());

  ASSERT_TRUE(db.endSession(userId, username, "logout"));
  ASSERT_EQ(db.getActiveSessionCount(userId), 0);
  ASSERT_EQ(db.getSessionCount(userId), 1);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_NoSessionOnFailedAuth() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  const std::string username = "session_fail";
  const std::string password = "secret";

  User user(username, User::hashPassword(password), User::Role::OPERATOR);
  ASSERT_TRUE(db.createUser(user));

  auto fail = db.authenticateUser(username, "wrong");
  ASSERT_TRUE(fail == nullptr);

  auto stored = db.getUserByUsername(username);
  ASSERT_NOT_NULL(stored);
  const int userId = stored->getId();
  ASSERT_EQ(db.getSessionCount(userId), 0);
  ASSERT_EQ(db.getActiveSessionCount(userId), 0);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_ReloginClosesPreviousSession() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  const std::string username = "session_relogin";
  const std::string password = "secret";

  User user(username, User::hashPassword(password), User::Role::OPERATOR);
  ASSERT_TRUE(db.createUser(user));

  auto first = db.authenticateUser(username, password);
  ASSERT_NOT_NULL(first);
  const int userId = first->getId();

  auto second = db.authenticateUser(username, password);
  ASSERT_NOT_NULL(second);

  ASSERT_EQ(db.getSessionCount(userId), 2);
  ASSERT_EQ(db.getActiveSessionCount(userId), 1);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_SessionLogoutAudit() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  const std::string username = "session_audit";
  const std::string password = "secret";

  User user(username, User::hashPassword(password), User::Role::ADMIN);
  ASSERT_TRUE(db.createUser(user));

  auto auth = db.authenticateUser(username, password);
  ASSERT_NOT_NULL(auth);
  const int userId = auth->getId();

  ASSERT_TRUE(db.endSession(userId, username, "logout"));

  auto entries =
      db.getAuditLogByEntity(AuditEntry::EntityType::USER, username);
  ASSERT_FALSE(entries.empty());

  bool foundLogout = false;
  for (const auto &entry : entries) {
    if (entry->getAction() == AuditEntry::ActionType::LOGOUT) {
      foundLogout = true;
      break;
    }
  }
  ASSERT_TRUE(foundLogout);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_ExportValidatedResults_CsvOutput() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("RES_EXP", "P305");
  ASSERT_TRUE(db.createSample(sample));

  Order order("ORD_RES_EXP", "RES_EXP", "PCR");
  ASSERT_TRUE(db.createOrder(order));

  auto storedOrder = db.getOrderByOrderId("ORD_RES_EXP");
  ASSERT_NOT_NULL(storedOrder);
  int orderDbId = storedOrder->getId();

  TestResult validated("RES_EXP_1", orderDbId, "PCR");
  validated.setValue("1.1");
  validated.setUnit("mg/L");
  validated.setReferenceLow(1.0);
  validated.setReferenceHigh(2.0);
  validated.setMeasuredDate(1700000000);
  validated.setMeasuredBy("tester");
  validated.setComment("ok");
  validated.setStatus(TestResult::Status::VALIDATED);
  validated.setFlag(validated.evaluateFlag());
  ASSERT_TRUE(db.createTestResult(validated));

  TestResult pending("RES_EXP_2", orderDbId, "PCR");
  pending.setValue("3.3");
  pending.setUnit("mg/L");
  pending.setReferenceLow(1.0);
  pending.setReferenceHigh(2.0);
  pending.setMeasuredDate(1700000001);
  pending.setMeasuredBy("tester");
  pending.setComment("skip");
  pending.setStatus(TestResult::Status::ENTERED);
  pending.setFlag(pending.evaluateFlag());
  ASSERT_TRUE(db.createTestResult(pending));

  std::string exportPath = "test_export_results.csv";
  ASSERT_TRUE(db.exportValidatedResultsToCsv(exportPath, "tester",
                                             std::nullopt));

  std::ifstream input(exportPath);
  ASSERT_TRUE(input.is_open());

  std::string header;
  std::getline(input, header);
  ASSERT_EQ(header,
            "result_id,order_id,test_parameter,value,unit,reference_low,"
            "reference_high,status,flag,measured_date,measured_by,comment");

  std::string row;
  std::getline(input, row);
  ASSERT_FALSE(row.empty());
  ASSERT_NE(row.find("RES_EXP_1"), std::string::npos);
  ASSERT_EQ(row.find("RES_EXP_2"), std::string::npos);

  std::string extra;
  ASSERT_FALSE(std::getline(input, extra));

  input.close();
  std::remove(exportPath.c_str());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_ExportValidatedResults_LogsAudit() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("RES_EXP_AUD", "P306");
  ASSERT_TRUE(db.createSample(sample));

  Order order("ORD_RES_EXP_AUD", "RES_EXP_AUD", "PCR");
  ASSERT_TRUE(db.createOrder(order));

  auto storedOrder = db.getOrderByOrderId("ORD_RES_EXP_AUD");
  ASSERT_NOT_NULL(storedOrder);
  int orderDbId = storedOrder->getId();

  TestResult r1("RES_EXP_AUD_1", orderDbId, "PCR");
  r1.setValue("1.1");
  r1.setUnit("mg/L");
  r1.setReferenceLow(1.0);
  r1.setReferenceHigh(2.0);
  r1.setMeasuredDate(1700000000);
  r1.setMeasuredBy("tester");
  r1.setComment("ok");
  r1.setStatus(TestResult::Status::VALIDATED);
  r1.setFlag(r1.evaluateFlag());
  ASSERT_TRUE(db.createTestResult(r1));

  TestResult r2("RES_EXP_AUD_2", orderDbId, "PCR");
  r2.setValue("1.2");
  r2.setUnit("mg/L");
  r2.setReferenceLow(1.0);
  r2.setReferenceHigh(2.0);
  r2.setMeasuredDate(1700000001);
  r2.setMeasuredBy("tester");
  r2.setComment("ok");
  r2.setStatus(TestResult::Status::VALIDATED);
  r2.setFlag(r2.evaluateFlag());
  ASSERT_TRUE(db.createTestResult(r2));

  std::string exportPath = "test_export_audit.csv";
  ASSERT_TRUE(db.exportValidatedResultsToCsv(exportPath, "tester",
                                             std::nullopt));

  auto entries1 =
      db.getAuditLogByEntity(AuditEntry::EntityType::RESULT, "RES_EXP_AUD_1");
  ASSERT_FALSE(entries1.empty());
  const AuditEntry *entry1 =
      findAuditEntry(entries1, AuditEntry::ActionType::UPDATE,
                     AuditEntry::EntityType::RESULT, "RES_EXP_AUD_1");
  ASSERT_NOT_NULL(entry1);
  ASSERT_EQ(entry1->getDetails(),
            "Export: test_export_audit.csv; Anzahl: 2");

  auto entries2 =
      db.getAuditLogByEntity(AuditEntry::EntityType::RESULT, "RES_EXP_AUD_2");
  ASSERT_FALSE(entries2.empty());
  const AuditEntry *entry2 =
      findAuditEntry(entries2, AuditEntry::ActionType::UPDATE,
                     AuditEntry::EntityType::RESULT, "RES_EXP_AUD_2");
  ASSERT_NOT_NULL(entry2);
  ASSERT_EQ(entry2->getDetails(),
            "Export: test_export_audit.csv; Anzahl: 2");

  std::remove(exportPath.c_str());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_UpdateResultWithAudit_StoresUpdatedFields() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("RES_EDIT", "P303");
  ASSERT_TRUE(db.createSample(sample));

  Order order("ORD_RES_EDIT", "RES_EDIT", "PCR");
  ASSERT_TRUE(db.createOrder(order));

  auto storedOrder = db.getOrderByOrderId("ORD_RES_EDIT");
  ASSERT_NOT_NULL(storedOrder);
  int orderDbId = storedOrder->getId();

  TestResult result("RES_EDIT_1", orderDbId, "PCR");
  result.setValue("1.0");
  result.setUnit("mg/L");
  result.setComment("initial");
  result.setFlag(result.evaluateFlag());
  ASSERT_TRUE(db.createTestResult(result));

  auto storedResult = db.getTestResultByResultId("RES_EDIT_1");
  ASSERT_NOT_NULL(storedResult);

  storedResult->setValue("2.0");
  storedResult->setUnit("g/L");
  storedResult->setComment("updated");
  storedResult->setFlag(storedResult->evaluateFlag());

  ASSERT_TRUE(db.updateTestResultWithAudit(*storedResult, "tester"));

  auto updated = db.getTestResultByResultId("RES_EDIT_1");
  ASSERT_NOT_NULL(updated);
  ASSERT_EQ(updated->getValue(), "2.0");
  ASSERT_EQ(updated->getUnit(), "g/L");
  ASSERT_EQ(updated->getComment(), "updated");

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_UpdateResultWithAudit_LogsChanges() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("RES_EDIT_AUDIT", "P304");
  ASSERT_TRUE(db.createSample(sample));

  Order order("ORD_RES_EDIT_AUDIT", "RES_EDIT_AUDIT", "PCR");
  ASSERT_TRUE(db.createOrder(order));

  auto storedOrder = db.getOrderByOrderId("ORD_RES_EDIT_AUDIT");
  ASSERT_NOT_NULL(storedOrder);
  int orderDbId = storedOrder->getId();

  TestResult result("RES_EDIT_AUDIT_1", orderDbId, "PCR");
  result.setValue("1.0");
  result.setUnit("mg/L");
  result.setComment("initial");
  result.setFlag(result.evaluateFlag());
  ASSERT_TRUE(db.createTestResult(result));

  auto storedResult = db.getTestResultByResultId("RES_EDIT_AUDIT_1");
  ASSERT_NOT_NULL(storedResult);

  storedResult->setValue("2.0");
  storedResult->setUnit("g/L");
  storedResult->setComment("updated");
  storedResult->setFlag(storedResult->evaluateFlag());

  ASSERT_TRUE(db.updateTestResultWithAudit(*storedResult, "tester"));

  auto entries =
      db.getAuditLogByEntity(AuditEntry::EntityType::RESULT, "RES_EDIT_AUDIT_1");
  ASSERT_FALSE(db.hasError());
  ASSERT_FALSE(entries.empty());
  const AuditEntry *entry =
      findAuditEntry(entries, AuditEntry::ActionType::UPDATE,
                     AuditEntry::EntityType::RESULT, "RES_EDIT_AUDIT_1",
                     "tester");
  ASSERT_NOT_NULL(entry);
  ASSERT_NE(entry->getDetails().find("Wert: 1.0 -> 2.0"), std::string::npos);
  ASSERT_NE(entry->getDetails().find("Einheit: mg/L -> g/L"),
            std::string::npos);
  ASSERT_NE(entry->getDetails().find("Kommentar: initial -> updated"),
            std::string::npos);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_GetOrdersByFilter() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sampleA("SAMP_A", "P200");
  Sample sampleB("SAMP_B", "P201");
  ASSERT_TRUE(db.createSample(sampleA));
  ASSERT_TRUE(db.createSample(sampleB));

  Order o1("ORD_A1", "SAMP_A", "Blutbild");
  o1.setPriority(Order::Priority::NORMAL);
  ASSERT_TRUE(db.createOrder(o1));

  Order o2("ORD_A2", "SAMP_A", "Glucose");
  o2.setPriority(Order::Priority::URGENT);
  o2.setStatus(Order::Status::IN_PROGRESS);
  ASSERT_TRUE(db.createOrder(o2));

  Order o3("ORD_B1", "SAMP_B", "PCR");
  o3.setPriority(Order::Priority::EMERGENCY);
  ASSERT_TRUE(db.createOrder(o3));

  Database::OrderFilter statusFilter;
  statusFilter.status = Order::statusToString(Order::Status::IN_PROGRESS);
  auto byStatus = db.getOrdersByFilter(statusFilter);
  ASSERT_FALSE(db.hasError());
  ASSERT_EQ(byStatus.size(), static_cast<size_t>(1));
  ASSERT_EQ(byStatus[0]->getOrderId(), "ORD_A2");

  Database::OrderFilter sampleFilter;
  sampleFilter.sampleId = "SAMP_A";
  auto bySample = db.getOrdersByFilter(sampleFilter);
  ASSERT_FALSE(db.hasError());
  ASSERT_EQ(bySample.size(), static_cast<size_t>(2));

  Database::OrderFilter priorityFilter;
  priorityFilter.priority = Order::priorityToString(Order::Priority::EMERGENCY);
  auto byPriority = db.getOrdersByFilter(priorityFilter);
  ASSERT_FALSE(db.hasError());
  ASSERT_EQ(byPriority.size(), static_cast<size_t>(1));
  ASSERT_EQ(byPriority[0]->getOrderId(), "ORD_B1");

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_LogOrderStatusUpdate() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("ORD_SAMPLE", "P020");
  ASSERT_TRUE(db.createSample(sample));

  Order order("ORD_UPD_1", "ORD_SAMPLE", "Blutbild");
  ASSERT_TRUE(db.createOrder(order));

  auto stored = db.getOrderByOrderId("ORD_UPD_1");
  ASSERT_NOT_NULL(stored);
  stored->setStatus(Order::Status::IN_PROGRESS);
  ASSERT_TRUE(db.updateOrder(*stored, "tester"));

  auto entries =
      db.getAuditLogByEntity(AuditEntry::EntityType::ORDER, "ORD_UPD_1");
  ASSERT_FALSE(db.hasError());
  ASSERT_FALSE(entries.empty());
  const AuditEntry *entry =
      findAuditEntry(entries, AuditEntry::ActionType::UPDATE,
                     AuditEntry::EntityType::ORDER, "ORD_UPD_1", "tester");
  ASSERT_NOT_NULL(entry);
  ASSERT_NE(entry->getDetails().find("Status"), std::string::npos);
  ASSERT_NE(entry->getDetails().find("->"), std::string::npos);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_CancelOrderLogsAudit() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("CANCEL_SAMPLE", "P030");
  ASSERT_TRUE(db.createSample(sample));

  Order order("ORD_CANCEL", "CANCEL_SAMPLE", "PCR");
  ASSERT_TRUE(db.createOrder(order));

  auto stored = db.getOrderByOrderId("ORD_CANCEL");
  ASSERT_NOT_NULL(stored);
  stored->setStatus(Order::Status::CANCELLED);
  ASSERT_TRUE(db.updateOrder(*stored, "tester"));

  auto entries =
      db.getAuditLogByEntity(AuditEntry::EntityType::ORDER, "ORD_CANCEL");
  ASSERT_FALSE(db.hasError());
  ASSERT_FALSE(entries.empty());
  const AuditEntry *entry =
      findAuditEntry(entries, AuditEntry::ActionType::UPDATE,
                     AuditEntry::EntityType::ORDER, "ORD_CANCEL", "tester");
  ASSERT_NOT_NULL(entry);
  ASSERT_NE(entry->getDetails().find("Status"), std::string::npos);
  ASSERT_NE(entry->getDetails().find("->"), std::string::npos);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_database_ExportSamples_CsvOutput() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample sample("S_EXP", "P_EXP");
  sample.setPatientName("Patient X");
  sample.setDescription("Desc");
  sample.setStatus(Sample::Status::REGISTERED);
  ASSERT_TRUE(db.createSample(sample));

  std::string exportPath = "test_export_samples.csv";
  ASSERT_TRUE(db.exportSamplesToCsv(exportPath));

  std::ifstream input(exportPath);
  ASSERT_TRUE(input.is_open());

  std::string header;
  std::getline(input, header);
  ASSERT_EQ(header,
            "sample_id,patient_id,patient_name,description,status");

  std::string row;
  std::getline(input, row);
  ASSERT_NE(row.find("S_EXP"), std::string::npos);
  ASSERT_NE(row.find("P_EXP"), std::string::npos);

  std::string extra;
  ASSERT_FALSE(std::getline(input, extra));

  input.close();
  std::remove(exportPath.c_str());

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

void registerDatabaseTests() {
  registerTest("Database::OpenAndClose", test_database_OpenAndClose);
  registerTest("Database::InitializeSchema", test_database_InitializeSchema);
  registerTest("Database::CreateSample", test_database_CreateSample);
  registerTest("Database::GetSampleByBarcode",
               test_database_GetSampleByBarcode);
  registerTest("Database::GetAllSamples", test_database_GetAllSamples);
  registerTest("Database::GetAllSamples_EmptyDatabase",
               test_database_GetAllSamples_EmptyDatabase);
  registerTest("Database::GetSamplesByFilter", test_database_GetSamplesByFilter);
  registerTest("Database::UpdateSample", test_database_UpdateSample);
  registerTest("Database::LogSampleStatusUpdate",
               test_database_LogSampleStatusUpdate);
  registerTest("Database::LogSampleDelete", test_database_LogSampleDelete);
  registerTest("Database::AuditLogsSampleCrud",
               test_database_AuditLogsSampleCrud);
  registerTest("Database::LogOrderStatusUpdate",
               test_database_LogOrderStatusUpdate);
  registerTest("Database::CancelOrderLogsAudit",
               test_database_CancelOrderLogsAudit);
  registerTest("Database::CreateOrder_RequestedDateStored",
               test_database_CreateOrder_RequestedDateStored);
  registerTest("Database::CreateOrder_MissingFieldsRejected",
               test_database_CreateOrder_MissingFieldsRejected);
  registerTest("Database::NoAuditOnFailedOrderCreate",
               test_database_NoAuditOnFailedOrderCreate);
  registerTest("Database::CreateTestResult_Valid",
               test_database_CreateTestResult_Valid);
  registerTest("Database::CreateTestResult_MissingFieldsRejected",
               test_database_CreateTestResult_MissingFieldsRejected);
  registerTest("Database::ValidateResultLogsAudit",
               test_database_ValidateResultLogsAudit);
  registerTest("Database::LogResultRetryImportAudit",
               test_database_LogResultRetryImportAudit);
  registerTest("Database::LogUserAction", test_database_LogUserAction);
  registerTest("Database::CreateRoleAndPermissions",
               test_database_CreateRoleAndPermissions);
  registerTest("Database::UpdateRolePermissions",
               test_database_UpdateRolePermissions);
  registerTest("Database::AssignUserRoleCustom",
               test_database_AssignUserRoleCustom);
  registerTest("Database::LogRoleAction", test_database_LogRoleAction);
  registerTest("Database::AuditLogFiltering",
               test_database_AuditLogFiltering);
  registerTest("Database::AuditLogFilterReset",
               test_database_AuditLogFilterReset);
  registerTest("Database::AuditLogExport_CsvOutput",
               test_database_AuditLogExport_CsvOutput);
  registerTest("Database::AuditLogExport_FilteredAndLogged",
               test_database_AuditLogExport_FilteredAndLogged);
  registerTest("Database::RetentionMinEnforced",
               test_database_RetentionMinEnforced);
  registerTest("Database::AuditRetentionPurgesAndLogs",
               test_database_AuditRetentionPurgesAndLogs);
  registerTest("Database::AuthenticateUserRejectsInactive",
               test_database_AuthenticateUserRejectsInactive);
  registerTest("Database::AuthConfigLdapEnabled",
               test_database_AuthConfigLdapEnabled);
  registerTest("Database::LdapAuthenticationPath",
               test_database_LdapAuthenticationPath);
  registerTest("Database::MfaRequiredFlowLocalUser",
               test_database_MfaRequiredFlowLocalUser);
  registerTest("Database::AuthAuditLogging", test_database_AuthAuditLogging);
  registerTest("Database::SessionStartAndEnd", test_database_SessionStartAndEnd);
  registerTest("Database::NoSessionOnFailedAuth",
               test_database_NoSessionOnFailedAuth);
  registerTest("Database::ReloginClosesPreviousSession",
               test_database_ReloginClosesPreviousSession);
  registerTest("Database::SessionLogoutAudit",
               test_database_SessionLogoutAudit);
  registerTest("Database::ExportValidatedResults_CsvOutput",
               test_database_ExportValidatedResults_CsvOutput);
  registerTest("Database::ExportValidatedResults_LogsAudit",
               test_database_ExportValidatedResults_LogsAudit);
  registerTest("Database::ExportSamples_CsvOutput",
               test_database_ExportSamples_CsvOutput);
  registerTest("Database::UpdateResultWithAudit_StoresUpdatedFields",
               test_database_UpdateResultWithAudit_StoresUpdatedFields);
  registerTest("Database::UpdateResultWithAudit_LogsChanges",
               test_database_UpdateResultWithAudit_LogsChanges);
  registerTest("Database::GetOrdersByFilter",
               test_database_GetOrdersByFilter);
  registerTest("Database::UpdateSample_NoChangesStillSuccess",
               test_database_UpdateSample_NoChangesStillSuccess);
  registerTest("Database::UpdateSample_NotFound",
               test_database_UpdateSample_NotFound);
  registerTest("Database::DeleteSample", test_database_DeleteSample);
  registerTest("Database::DeleteSample_NotFound",
               test_database_DeleteSample_NotFound);
}
