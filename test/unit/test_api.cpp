/**
 * @file test_api.cpp
 * @brief Unit-Tests für die REST-API Read Access
 */

#include "api/ApiServer.h"
#include "core/Order.h"
#include "core/Sample.h"
#include "core/TestResult.h"
#include "db/Database.h"
#include "test_macros.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <sstream>

using namespace opensylab::api;
using namespace opensylab::core;
using namespace opensylab::db;

namespace {
std::string uniqueDbPath() {
  std::ostringstream ss;
  ss << "test_api_db_" << std::rand() << "_" << std::time(nullptr) << ".db";
  return ss.str();
}

size_t auditCount(Database &db, AuditEntry::EntityType entity,
                  const std::string &entityId) {
  return db.getAuditLogByEntity(entity, entityId).size();
}
} // namespace

bool test_api_SerializeSampleJson() {
  Sample sample("S_API_1", "P_API_1");
  sample.setPatientName("Patient A");
  sample.setDescription("Desc");
  sample.setStatus(Sample::Status::REGISTERED);
  sample.setRegistrationDate(1700000000);

  const std::string json = ApiRouter::sampleToJson(sample);
  ASSERT_NE(json.find("\"sample_id\":\"S_API_1\""), std::string::npos);
  ASSERT_NE(json.find("\"patient_id\":\"P_API_1\""), std::string::npos);
  ASSERT_NE(json.find("\"status\":\"Erfasst\""), std::string::npos);
  return true;
}

bool test_api_SerializeOrderJson() {
  Order order("O_API_1", "S_API_1", "PCR");
  order.setStatus(Order::Status::REQUESTED);
  order.setPriority(Order::Priority::NORMAL);
  order.setRequestedDate(1700000100);

  const std::string json = ApiRouter::orderToJson(order);
  ASSERT_NE(json.find("\"order_id\":\"O_API_1\""), std::string::npos);
  ASSERT_NE(json.find("\"sample_id\":\"S_API_1\""), std::string::npos);
  ASSERT_NE(json.find("\"status\":\"Angefordert\""), std::string::npos);
  return true;
}

bool test_api_SerializeResultJson() {
  TestResult result("R_API_1", 42, "GLU");
  result.setValue("1.2");
  result.setUnit("mg/L");
  result.setReferenceLow(1.0);
  result.setReferenceHigh(2.0);
  result.setStatus(TestResult::Status::VALIDATED);
  result.setFlag(result.evaluateFlag());
  result.setMeasuredDate(1700000200);

  const std::string json = ApiRouter::resultToJson(result);
  ASSERT_NE(json.find("\"result_id\":\"R_API_1\""), std::string::npos);
  ASSERT_NE(json.find("\"order_id\":42"), std::string::npos);
  ASSERT_NE(json.find("\"status\":\"Validiert\""), std::string::npos);
  return true;
}

bool test_api_ReadSamplesAuthRequired() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());

  Sample sample("S_API_AUTH", "P1");
  ASSERT_TRUE(db->createSample(sample));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/samples";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 401);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadSamplesReturnsJson() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey", true));

  Sample sample("S_API_LIST", "P2");
  ASSERT_TRUE(db->createSample(sample));

  const size_t before = auditCount(*db, AuditEntry::EntityType::SAMPLE, "*");

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/samples";
  req.headers["x-api-key"] = "testkey";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 200);
  ASSERT_NE(res.body.find("\"sample_id\":\"S_API_LIST\""),
            std::string::npos);
  ASSERT_EQ(auditCount(*db, AuditEntry::EntityType::SAMPLE, "*"), before + 1);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadSampleByIdAudits() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey", true));

  Sample sample("S_API_GET", "P3");
  ASSERT_TRUE(db->createSample(sample));

  const size_t before =
      auditCount(*db, AuditEntry::EntityType::SAMPLE, "S_API_GET");

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/samples/S_API_GET";
  req.headers["x-api-key"] = "testkey";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 200);
  ASSERT_NE(res.body.find("\"sample_id\":\"S_API_GET\""),
            std::string::npos);
  ASSERT_EQ(auditCount(*db, AuditEntry::EntityType::SAMPLE, "S_API_GET"),
            before + 1);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadOrdersReturnsJson() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey", true));

  Sample sample("S_API_ORDER", "P9");
  ASSERT_TRUE(db->createSample(sample));
  Order order("O_API_LIST", "S_API_ORDER", "PCR");
  ASSERT_TRUE(db->createOrder(order));

  const size_t before = auditCount(*db, AuditEntry::EntityType::ORDER, "*");

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/orders";
  req.headers["x-api-key"] = "testkey";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 200);
  ASSERT_NE(res.body.find("\"order_id\":\"O_API_LIST\""),
            std::string::npos);
  ASSERT_EQ(auditCount(*db, AuditEntry::EntityType::ORDER, "*"), before + 1);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadOrderByIdAudits() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey", true));

  Sample sample("S_API_ORDER_GET", "P10");
  ASSERT_TRUE(db->createSample(sample));
  Order order("O_API_GET", "S_API_ORDER_GET", "PCR");
  ASSERT_TRUE(db->createOrder(order));

  const size_t before =
      auditCount(*db, AuditEntry::EntityType::ORDER, "O_API_GET");

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/orders/O_API_GET";
  req.headers["x-api-key"] = "testkey";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 200);
  ASSERT_NE(res.body.find("\"order_id\":\"O_API_GET\""),
            std::string::npos);
  ASSERT_EQ(auditCount(*db, AuditEntry::EntityType::ORDER, "O_API_GET"),
            before + 1);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadResultsReturnsJson() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey", true));

  Sample sample("S_API_RES", "P11");
  ASSERT_TRUE(db->createSample(sample));
  Order order("O_API_RES", "S_API_RES", "PCR");
  ASSERT_TRUE(db->createOrder(order));
  auto createdOrder = db->getOrderByOrderId("O_API_RES");
  ASSERT_TRUE(createdOrder != nullptr);

  TestResult result("R_API_LIST", createdOrder->getId(), "GLU");
  result.setValue("1.2");
  result.setUnit("mg/L");
  ASSERT_TRUE(db->createTestResult(result));

  const size_t before = auditCount(*db, AuditEntry::EntityType::RESULT, "*");

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  std::ostringstream path;
  path << "/api/v1/results?order_id=" << createdOrder->getId();
  req.path = path.str();
  req.headers["x-api-key"] = "testkey";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 200);
  ASSERT_NE(res.body.find("\"result_id\":\"R_API_LIST\""),
            std::string::npos);
  ASSERT_EQ(auditCount(*db, AuditEntry::EntityType::RESULT, "*"), before + 1);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadResultByIdAudits() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey", true));

  Sample sample("S_API_RES_GET", "P12");
  ASSERT_TRUE(db->createSample(sample));
  Order order("O_API_RES_GET", "S_API_RES_GET", "PCR");
  ASSERT_TRUE(db->createOrder(order));
  auto createdOrder = db->getOrderByOrderId("O_API_RES_GET");
  ASSERT_TRUE(createdOrder != nullptr);

  TestResult result("R_API_GET", createdOrder->getId(), "GLU");
  result.setValue("1.2");
  result.setUnit("mg/L");
  ASSERT_TRUE(db->createTestResult(result));

  const size_t before =
      auditCount(*db, AuditEntry::EntityType::RESULT, "R_API_GET");

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/results/R_API_GET";
  req.headers["x-api-key"] = "testkey";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 200);
  ASSERT_NE(res.body.find("\"result_id\":\"R_API_GET\""),
            std::string::npos);
  ASSERT_EQ(auditCount(*db, AuditEntry::EntityType::RESULT, "R_API_GET"),
            before + 1);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

void registerApiTests() {
  registerTest("Api::SerializeSampleJson", test_api_SerializeSampleJson);
  registerTest("Api::SerializeOrderJson", test_api_SerializeOrderJson);
  registerTest("Api::SerializeResultJson", test_api_SerializeResultJson);
  registerTest("Api::ReadSamplesAuthRequired", test_api_ReadSamplesAuthRequired);
  registerTest("Api::ReadSamplesReturnsJson", test_api_ReadSamplesReturnsJson);
  registerTest("Api::ReadSampleByIdAudits", test_api_ReadSampleByIdAudits);
  registerTest("Api::ReadOrdersReturnsJson", test_api_ReadOrdersReturnsJson);
  registerTest("Api::ReadOrderByIdAudits", test_api_ReadOrderByIdAudits);
  registerTest("Api::ReadResultsReturnsJson", test_api_ReadResultsReturnsJson);
  registerTest("Api::ReadResultByIdAudits", test_api_ReadResultByIdAudits);
}
