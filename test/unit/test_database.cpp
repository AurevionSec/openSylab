/**
 * @file test_database.cpp
 * @brief Unit-Tests für die Database-Klasse
 */

#include "db/Database.h"
#include "test_macros.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
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
  ASSERT_EQ(entries[0]->getAction(), AuditEntry::ActionType::UPDATE);
  ASSERT_EQ(entries[0]->getEntity(), AuditEntry::EntityType::SAMPLE);
  ASSERT_EQ(entries[0]->getEntityId(), "AUDIT001");
  ASSERT_EQ(entries[0]->getUser(), "tester");
  ASSERT_EQ(entries[0]->getDetails(), "Status: Erfasst -> Validiert");

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
  ASSERT_EQ(entries[0]->getAction(), AuditEntry::ActionType::DELETE);
  ASSERT_EQ(entries[0]->getEntity(), AuditEntry::EntityType::SAMPLE);
  ASSERT_EQ(entries[0]->getEntityId(), "DEL001");
  ASSERT_EQ(entries[0]->getDetails(), "Sample gelöscht");

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
  ASSERT_EQ(entries[0]->getAction(), AuditEntry::ActionType::UPDATE);
  ASSERT_EQ(entries[0]->getEntity(), AuditEntry::EntityType::RESULT);
  ASSERT_EQ(entries[0]->getEntityId(), "RES_AUDIT_1");
  ASSERT_EQ(entries[0]->getUser(), "tester");
  ASSERT_EQ(entries[0]->getDetails(), "Status: Eingegeben -> Validiert");

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
  ASSERT_EQ(entries[0]->getAction(), AuditEntry::ActionType::UPDATE);
  ASSERT_EQ(entries[0]->getEntity(), AuditEntry::EntityType::RESULT);
  ASSERT_EQ(entries[0]->getEntityId(), "RES_EDIT_AUDIT_1");
  ASSERT_EQ(entries[0]->getUser(), "tester");
  ASSERT_EQ(entries[0]->getDetails(),
            "Wert: 1.0 -> 2.0; Einheit: mg/L -> g/L; Kommentar: initial -> "
            "updated");

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
  ASSERT_TRUE(db.updateOrder(*stored));

  db.logOrderAction(AuditEntry::ActionType::UPDATE, "ORD_UPD_1", "tester",
                    "Status: Angefordert -> In Bearbeitung");

  auto entries =
      db.getAuditLogByEntity(AuditEntry::EntityType::ORDER, "ORD_UPD_1");
  ASSERT_FALSE(db.hasError());
  ASSERT_FALSE(entries.empty());
  ASSERT_EQ(entries[0]->getAction(), AuditEntry::ActionType::UPDATE);
  ASSERT_EQ(entries[0]->getEntity(), AuditEntry::EntityType::ORDER);
  ASSERT_EQ(entries[0]->getEntityId(), "ORD_UPD_1");
  ASSERT_EQ(entries[0]->getDetails(),
            "Status: Angefordert -> In Bearbeitung");

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
  ASSERT_TRUE(db.updateOrder(*stored));

  db.logOrderAction(AuditEntry::ActionType::UPDATE, "ORD_CANCEL", "tester",
                    "Status: Angefordert -> Storniert");

  auto entries =
      db.getAuditLogByEntity(AuditEntry::EntityType::ORDER, "ORD_CANCEL");
  ASSERT_FALSE(db.hasError());
  ASSERT_FALSE(entries.empty());
  ASSERT_EQ(entries[0]->getAction(), AuditEntry::ActionType::UPDATE);
  ASSERT_EQ(entries[0]->getEntity(), AuditEntry::EntityType::ORDER);
  ASSERT_EQ(entries[0]->getEntityId(), "ORD_CANCEL");
  ASSERT_EQ(entries[0]->getDetails(), "Status: Angefordert -> Storniert");

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
  registerTest("Database::LogOrderStatusUpdate",
               test_database_LogOrderStatusUpdate);
  registerTest("Database::CancelOrderLogsAudit",
               test_database_CancelOrderLogsAudit);
  registerTest("Database::CreateOrder_RequestedDateStored",
               test_database_CreateOrder_RequestedDateStored);
  registerTest("Database::CreateOrder_MissingFieldsRejected",
               test_database_CreateOrder_MissingFieldsRejected);
  registerTest("Database::CreateTestResult_Valid",
               test_database_CreateTestResult_Valid);
  registerTest("Database::CreateTestResult_MissingFieldsRejected",
               test_database_CreateTestResult_MissingFieldsRejected);
  registerTest("Database::ValidateResultLogsAudit",
               test_database_ValidateResultLogsAudit);
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
