/**
 * @file test_api.cpp
 * @brief Unit-Tests für die REST-API Read/Write Access
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
  ASSERT_NE(json.find("\"status\":\"REGISTERED\""), std::string::npos);
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
  ASSERT_NE(json.find("\"status\":\"REQUESTED\""), std::string::npos);
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
  ASSERT_NE(json.find("\"order_id\":\"42\""), std::string::npos);
  ASSERT_NE(json.find("\"status\":\"VALIDATED\""), std::string::npos);
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
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  Sample sample("S_API_LIST", "P2");
  ASSERT_TRUE(db->createSample(sample));

  const size_t before = auditCount(*db, AuditEntry::EntityType::SAMPLE, "*");

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/samples";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 200);
  ASSERT_NE(res.body.find("\"sample_id\":\"S_API_LIST\""),
            std::string::npos);
  ASSERT_EQ(auditCount(*db, AuditEntry::EntityType::SAMPLE, "*"), before + 1);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadSamplesInvalidStatus() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/samples?status=NOPE";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 400);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadSamplesInvalidPagination() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/samples?offset=10";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 400);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadSampleByIdAudits() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  Sample sample("S_API_GET", "P3");
  ASSERT_TRUE(db->createSample(sample));

  const size_t before =
      auditCount(*db, AuditEntry::EntityType::SAMPLE, "S_API_GET");

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/samples/S_API_GET";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";

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
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  Sample sample("S_API_ORDER", "P9");
  ASSERT_TRUE(db->createSample(sample));
  Order order("O_API_LIST", "S_API_ORDER", "PCR");
  ASSERT_TRUE(db->createOrder(order));

  const size_t before = auditCount(*db, AuditEntry::EntityType::ORDER, "*");

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/orders";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 200);
  ASSERT_NE(res.body.find("\"order_id\":\"O_API_LIST\""),
            std::string::npos);
  ASSERT_EQ(auditCount(*db, AuditEntry::EntityType::ORDER, "*"), before + 1);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadOrdersInvalidFilters() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  ApiRouter router(db);
  ApiRequest badStatus;
  badStatus.method = "GET";
  badStatus.path = "/api/v1/orders?status=BAD";
  badStatus.headers["x-api-key"] = "testkey-000000000000000000000000000";
  ApiResponse resStatus = router.handleRequest(badStatus);
  ASSERT_EQ(resStatus.status, 400);

  ApiRequest badPriority;
  badPriority.method = "GET";
  badPriority.path = "/api/v1/orders?priority=BAD";
  badPriority.headers["x-api-key"] = "testkey-000000000000000000000000000";
  ApiResponse resPriority = router.handleRequest(badPriority);
  ASSERT_EQ(resPriority.status, 400);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadOrderByIdAudits() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

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
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";

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
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

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
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 200);
  ASSERT_NE(res.body.find("\"result_id\":\"R_API_LIST\""),
            std::string::npos);
  ASSERT_EQ(auditCount(*db, AuditEntry::EntityType::RESULT, "*"), before + 1);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadResultsInvalidPagination() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/results?limit=-1";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";
  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 400);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ReadResultByIdAudits() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

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
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";

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

bool test_api_ResultsInvalidStatusFilterRejected() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/results?status=BADVALUE";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 400);
  ASSERT_NE(res.body.find("validation_error"), std::string::npos);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ResultsInvalidFlagFilterRejected() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/results?flag=SUPERSONIC";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 400);
  ASSERT_NE(res.body.find("validation_error"), std::string::npos);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_ViewerRoleBlockedOnWrite() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("viewerkey-0000000000000000000000000", true, "VIEWER"));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "POST";
  req.path = "/api/v1/samples";
  req.headers["x-api-key"] = "viewerkey-0000000000000000000000000";
  req.body = R"({"sample_id":"S_VIEWER","patient_id":"P_VIEWER"})";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 403);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_CustomRoleBlockedOnWrite() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("customkey-0000000000000000000000000", true, "CUSTOM"));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "PUT";
  req.path = "/api/v1/samples/S_SOME";
  req.headers["x-api-key"] = "customkey-0000000000000000000000000";
  req.body = R"({"status":"IN_PROGRESS"})";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 403);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_OperatorRoleBlockedOnAdminUserList() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("operatorkey-000000000000000000000000", true, "OPERATOR"));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/users";
  req.headers["x-api-key"] = "operatorkey-000000000000000000000000";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 403);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_OperatorRoleBlockedOnAuditLog() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("operatorkey-000000000000000000000000", true, "OPERATOR"));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/audit";
  req.headers["x-api-key"] = "operatorkey-000000000000000000000000";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 403);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_WriteAuthRequired() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());

  ApiRouter router(db);
  ApiRequest req;
  req.method = "POST";
  req.path = "/api/v1/samples";
  req.body = "{}";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 401);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_InactiveApiKeyRejected() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", false));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/samples";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 401);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_WriteInvalidJsonPayload() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "POST";
  req.path = "/api/v1/samples";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";
  req.headers["content-type"] = "application/json";
  req.body = "{bad json}";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 400);
  ASSERT_NE(res.body.find("\"code\":\"validation_error\""),
            std::string::npos);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_WriteSampleMissingFields() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "POST";
  req.path = "/api/v1/samples";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";
  req.headers["content-type"] = "application/json";
  req.body = "{}";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 400);
  ASSERT_NE(res.body.find("\"code\":\"validation_error\""),
            std::string::npos);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_WriteOrderMissingFields() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "POST";
  req.path = "/api/v1/orders";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";
  req.headers["content-type"] = "application/json";
  req.body = "{\"order_id\":\"O-1\"}";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 400);
  ASSERT_NE(res.body.find("\"code\":\"validation_error\""),
            std::string::npos);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_WriteResultMissingFields() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "POST";
  req.path = "/api/v1/results";
  req.headers["x-api-key"] = "testkey-000000000000000000000000000";
  req.headers["content-type"] = "application/json";
  req.body = "{\"result_id\":\"R-1\",\"order_id\":1}";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 400);
  ASSERT_NE(res.body.find("\"code\":\"validation_error\""),
            std::string::npos);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_WriteSampleCreateAndUpdate() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  ApiRouter router(db);
  ApiRequest createReq;
  createReq.method = "POST";
  createReq.path = "/api/v1/samples";
  createReq.headers["x-api-key"] = "testkey-000000000000000000000000000";
  createReq.headers["content-type"] = "application/json";
  createReq.body =
      "{\"sample_id\":\"S_API_WRITE\",\"patient_id\":\"P1\",\"patient_name\":\""
      "Alice\"}";

  ApiResponse createRes = router.handleRequest(createReq);
  ASSERT_EQ(createRes.status, 201);
  ASSERT_NE(createRes.body.find("\"sample_id\":\"S_API_WRITE\""),
            std::string::npos);

  ApiRequest updateReq;
  updateReq.method = "PUT";
  updateReq.path = "/api/v1/samples/S_API_WRITE";
  updateReq.headers["x-api-key"] = "testkey-000000000000000000000000000";
  updateReq.headers["content-type"] = "application/json";
  updateReq.body = "{\"patient_id\":\"P2\",\"patient_name\":\"Bob\"}";

  ApiResponse updateRes = router.handleRequest(updateReq);
  ASSERT_EQ(updateRes.status, 200);

  auto updated = db->getSampleByBarcode("S_API_WRITE");
  ASSERT_TRUE(updated != nullptr);
  ASSERT_EQ(updated->getPatientId(), "P2");
  ASSERT_EQ(updated->getPatientName(), "Bob");

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_WriteOrderCreateAndUpdate() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  Sample sample("S_API_ORDER", "P9");
  ASSERT_TRUE(db->createSample(sample));

  ApiRouter router(db);
  ApiRequest createReq;
  createReq.method = "POST";
  createReq.path = "/api/v1/orders";
  createReq.headers["x-api-key"] = "testkey-000000000000000000000000000";
  createReq.headers["content-type"] = "application/json";
  createReq.body =
      "{\"order_id\":\"O_API_WRITE\",\"sample_id\":\"S_API_ORDER\",\"test_type\":\""
      "PCR\"}";

  ApiResponse createRes = router.handleRequest(createReq);
  ASSERT_EQ(createRes.status, 201);
  ASSERT_NE(createRes.body.find("\"order_id\":\"O_API_WRITE\""),
            std::string::npos);

  ApiRequest updateReq;
  updateReq.method = "PUT";
  updateReq.path = "/api/v1/orders/O_API_WRITE";
  updateReq.headers["x-api-key"] = "testkey-000000000000000000000000000";
  updateReq.headers["content-type"] = "application/json";
  updateReq.body =
      "{\"sample_id\":\"S_API_ORDER\",\"test_type\":\"ELISA\"}";

  ApiResponse updateRes = router.handleRequest(updateReq);
  ASSERT_EQ(updateRes.status, 200);

  auto updated = db->getOrderByOrderId("O_API_WRITE");
  ASSERT_TRUE(updated != nullptr);
  ASSERT_EQ(updated->getTestType(), "ELISA");

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_api_WriteResultCreateAndUpdate() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey("testkey-000000000000000000000000000", true));

  Sample sample("S_API_RES", "P3");
  ASSERT_TRUE(db->createSample(sample));
  Order order("O_API_RES", "S_API_RES", "PCR");
  ASSERT_TRUE(db->createOrder(order));
  auto createdOrder = db->getOrderByOrderId("O_API_RES");
  ASSERT_TRUE(createdOrder != nullptr);
  createdOrder->setStatus(Order::Status::IN_PROGRESS);
  ASSERT_TRUE(db->updateOrder(*createdOrder, "tester"));

  ApiRouter router(db);
  ApiRequest createReq;
  createReq.method = "POST";
  createReq.path = "/api/v1/results";
  createReq.headers["x-api-key"] = "testkey-000000000000000000000000000";
  createReq.headers["content-type"] = "application/json";
  std::ostringstream payload;
  payload << "{\"result_id\":\"R_API_WRITE\",\"order_id\":"
          << createdOrder->getId()
          << ",\"test_parameter\":\"GLU\",\"value\":\"1.2\",\"unit\":\"mg/L\"}";
  createReq.body = payload.str();

  ApiResponse createRes = router.handleRequest(createReq);
  ASSERT_EQ(createRes.status, 201);
  ASSERT_NE(createRes.body.find("\"result_id\":\"R_API_WRITE\""),
            std::string::npos);

  ApiRequest updateReq;
  updateReq.method = "PUT";
  updateReq.path = "/api/v1/results/R_API_WRITE";
  updateReq.headers["x-api-key"] = "testkey-000000000000000000000000000";
  updateReq.headers["content-type"] = "application/json";
  updateReq.body =
      "{\"test_parameter\":\"GLU\",\"value\":\"2.4\",\"unit\":\"mg/L\"}";

  ApiResponse updateRes = router.handleRequest(updateReq);
  ASSERT_EQ(updateRes.status, 200);

  auto updated = db->getTestResultByResultId("R_API_WRITE");
  ASSERT_TRUE(updated != nullptr);
  ASSERT_EQ(updated->getValue(), "2.4");

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

// --- Compliance-critical branches (ISO 15189): immutability guards, status
// --- transitions, ADMIN-only validation, and delete guards. These paths were
// --- relocated into per-route handler methods during the handleRequest
// --- decomposition and previously had no HTTP-level regression coverage.

namespace {
constexpr const char *kOpKey = "testkey-000000000000000000000000000";

// Persist an entity's status directly at the DB layer, bypassing the API's
// transition rules, to set up a precondition state for a test.
bool forceSampleStatus(Database &db, const std::string &sampleId,
                       Sample::Status status) {
  auto s = db.getSampleByBarcode(sampleId);
  if (!s) return false;
  s->setStatus(status);
  return db.updateSample(*s, "setup");
}
bool forceOrderStatus(Database &db, const std::string &orderId,
                      Order::Status status) {
  auto o = db.getOrderByOrderId(orderId);
  if (!o) return false;
  o->setStatus(status);
  return db.updateOrder(*o, "setup");
}
bool forceResultStatus(Database &db, const std::string &resultId,
                       TestResult::Status status) {
  auto r = db.getTestResultByResultId(resultId);
  if (!r) return false;
  r->setStatus(status);
  return db.updateTestResult(*r, "setup");
}
} // namespace

// PUT on a VALIDATED sample must be rejected as immutable (409).
bool test_api_UpdateSampleImmutableWhenValidated() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey(kOpKey, true));

  Sample sample("S_IMMUT", "P1");
  ASSERT_TRUE(db->createSample(sample));
  ASSERT_TRUE(forceSampleStatus(*db, "S_IMMUT", Sample::Status::VALIDATED));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "PUT";
  req.path = "/api/v1/samples/S_IMMUT";
  req.headers["x-api-key"] = kOpKey;
  req.headers["content-type"] = "application/json";
  req.body = "{\"patient_id\":\"P2\"}";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 409);
  ASSERT_NE(res.body.find("immutable"), std::string::npos);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

// An illegal sample status transition (REGISTERED -> ANALYZED) must yield 409.
bool test_api_UpdateSampleInvalidStatusTransition() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey(kOpKey, true));

  Sample sample("S_TRANS", "P1"); // defaults to REGISTERED
  ASSERT_TRUE(db->createSample(sample));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "PUT";
  req.path = "/api/v1/samples/S_TRANS";
  req.headers["x-api-key"] = kOpKey;
  req.headers["content-type"] = "application/json";
  req.body = "{\"patient_id\":\"P1\",\"status\":\"ANALYZED\"}";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 409);
  ASSERT_NE(res.body.find("Invalid status transition"), std::string::npos);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

// Non-ADMIN (OPERATOR key) may not set an order to VALIDATED (403).
bool test_api_UpdateOrderValidateRequiresAdmin() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey(kOpKey, true));

  Sample sample("S_OVAL", "P1");
  ASSERT_TRUE(db->createSample(sample));
  Order order("O_OVAL", "S_OVAL", "PCR");
  ASSERT_TRUE(db->createOrder(order));
  // COMPLETED -> VALIDATED is a legal transition, so only the ADMIN gate blocks.
  ASSERT_TRUE(forceOrderStatus(*db, "O_OVAL", Order::Status::COMPLETED));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "PUT";
  req.path = "/api/v1/orders/O_OVAL";
  req.headers["x-api-key"] = kOpKey;
  req.headers["content-type"] = "application/json";
  req.body =
      "{\"sample_id\":\"S_OVAL\",\"test_type\":\"PCR\",\"status\":\"VALIDATED\"}";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 403);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

// Non-ADMIN (OPERATOR key) may not validate (release) a result (403).
bool test_api_UpdateResultValidateRequiresAdmin() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey(kOpKey, true));

  Sample sample("S_RVAL", "P1");
  ASSERT_TRUE(db->createSample(sample));
  Order order("O_RVAL", "S_RVAL", "PCR");
  ASSERT_TRUE(db->createOrder(order));
  auto createdOrder = db->getOrderByOrderId("O_RVAL");
  ASSERT_TRUE(createdOrder != nullptr);
  TestResult result("R_RVAL", createdOrder->getId(), "GLU");
  result.setValue("1.0");
  result.setUnit("mg/L");
  result.setStatus(TestResult::Status::ENTERED); // ENTERED -> VALIDATED legal
  ASSERT_TRUE(db->createTestResult(result));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "PUT";
  req.path = "/api/v1/results/R_RVAL";
  req.headers["x-api-key"] = kOpKey;
  req.headers["content-type"] = "application/json";
  req.body = "{\"test_parameter\":\"GLU\",\"value\":\"1.0\",\"unit\":\"mg/L\","
             "\"status\":\"VALIDATED\"}";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 403);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

// Deleting a sample that still has a non-cancelled order must yield 409.
bool test_api_DeleteSampleBlockedByActiveOrders() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey(kOpKey, true));

  Sample sample("S_DELBLK", "P1"); // REGISTERED (deletable status)
  ASSERT_TRUE(db->createSample(sample));
  Order order("O_DELBLK", "S_DELBLK", "PCR"); // REQUESTED = active
  ASSERT_TRUE(db->createOrder(order));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "DELETE";
  req.path = "/api/v1/samples/S_DELBLK";
  req.headers["x-api-key"] = kOpKey;

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 409);
  ASSERT_NE(res.body.find("active orders"), std::string::npos);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

// Deleting an already-REJECTED result is idempotent (204).
bool test_api_DeleteResultRejectedIsIdempotent() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey(kOpKey, true));

  Sample sample("S_RREJ", "P1");
  ASSERT_TRUE(db->createSample(sample));
  Order order("O_RREJ", "S_RREJ", "PCR");
  ASSERT_TRUE(db->createOrder(order));
  auto createdOrder = db->getOrderByOrderId("O_RREJ");
  ASSERT_TRUE(createdOrder != nullptr);
  TestResult result("R_RREJ", createdOrder->getId(), "GLU");
  result.setValue("1.0");
  result.setUnit("mg/L");
  ASSERT_TRUE(db->createTestResult(result));
  ASSERT_TRUE(forceResultStatus(*db, "R_RREJ", TestResult::Status::REJECTED));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "DELETE";
  req.path = "/api/v1/results/R_RREJ";
  req.headers["x-api-key"] = kOpKey;

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 204);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

// Deleting a VALIDATED result must be rejected as immutable (409).
bool test_api_DeleteResultValidatedBlocked() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());
  ASSERT_TRUE(db->upsertApiKey(kOpKey, true));

  Sample sample("S_RVALDEL", "P1");
  ASSERT_TRUE(db->createSample(sample));
  Order order("O_RVALDEL", "S_RVALDEL", "PCR");
  ASSERT_TRUE(db->createOrder(order));
  auto createdOrder = db->getOrderByOrderId("O_RVALDEL");
  ASSERT_TRUE(createdOrder != nullptr);
  TestResult result("R_RVALDEL", createdOrder->getId(), "GLU");
  result.setValue("1.0");
  result.setUnit("mg/L");
  ASSERT_TRUE(db->createTestResult(result));
  ASSERT_TRUE(forceResultStatus(*db, "R_RVALDEL", TestResult::Status::VALIDATED));

  ApiRouter router(db);
  ApiRequest req;
  req.method = "DELETE";
  req.path = "/api/v1/results/R_RVALDEL";
  req.headers["x-api-key"] = kOpKey;

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 409);

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
  registerTest("Api::ReadSamplesInvalidStatus", test_api_ReadSamplesInvalidStatus);
  registerTest("Api::ReadSamplesInvalidPagination",
               test_api_ReadSamplesInvalidPagination);
  registerTest("Api::ReadSampleByIdAudits", test_api_ReadSampleByIdAudits);
  registerTest("Api::ReadOrdersReturnsJson", test_api_ReadOrdersReturnsJson);
  registerTest("Api::ReadOrdersInvalidFilters", test_api_ReadOrdersInvalidFilters);
  registerTest("Api::ReadOrderByIdAudits", test_api_ReadOrderByIdAudits);
  registerTest("Api::ReadResultsReturnsJson", test_api_ReadResultsReturnsJson);
  registerTest("Api::ReadResultsInvalidPagination",
               test_api_ReadResultsInvalidPagination);
  registerTest("Api::ResultsInvalidStatusFilterRejected",
               test_api_ResultsInvalidStatusFilterRejected);
  registerTest("Api::ResultsInvalidFlagFilterRejected",
               test_api_ResultsInvalidFlagFilterRejected);
  registerTest("Api::ReadResultByIdAudits", test_api_ReadResultByIdAudits);
  registerTest("Api::ViewerRoleBlockedOnWrite", test_api_ViewerRoleBlockedOnWrite);
  registerTest("Api::CustomRoleBlockedOnWrite", test_api_CustomRoleBlockedOnWrite);
  registerTest("Api::OperatorRoleBlockedOnAdminUserList", test_api_OperatorRoleBlockedOnAdminUserList);
  registerTest("Api::OperatorRoleBlockedOnAuditLog", test_api_OperatorRoleBlockedOnAuditLog);
  registerTest("Api::WriteAuthRequired", test_api_WriteAuthRequired);
  registerTest("Api::InactiveApiKeyRejected", test_api_InactiveApiKeyRejected);
  registerTest("Api::WriteInvalidJsonPayload", test_api_WriteInvalidJsonPayload);
  registerTest("Api::WriteSampleMissingFields",
               test_api_WriteSampleMissingFields);
  registerTest("Api::WriteOrderMissingFields",
               test_api_WriteOrderMissingFields);
  registerTest("Api::WriteResultMissingFields",
               test_api_WriteResultMissingFields);
  registerTest("Api::WriteSampleCreateAndUpdate",
               test_api_WriteSampleCreateAndUpdate);
  registerTest("Api::WriteOrderCreateAndUpdate",
               test_api_WriteOrderCreateAndUpdate);
  registerTest("Api::WriteResultCreateAndUpdate",
               test_api_WriteResultCreateAndUpdate);
  registerTest("Api::UpdateSampleImmutableWhenValidated",
               test_api_UpdateSampleImmutableWhenValidated);
  registerTest("Api::UpdateSampleInvalidStatusTransition",
               test_api_UpdateSampleInvalidStatusTransition);
  registerTest("Api::UpdateOrderValidateRequiresAdmin",
               test_api_UpdateOrderValidateRequiresAdmin);
  registerTest("Api::UpdateResultValidateRequiresAdmin",
               test_api_UpdateResultValidateRequiresAdmin);
  registerTest("Api::DeleteSampleBlockedByActiveOrders",
               test_api_DeleteSampleBlockedByActiveOrders);
  registerTest("Api::DeleteResultRejectedIsIdempotent",
               test_api_DeleteResultRejectedIsIdempotent);
  registerTest("Api::DeleteResultValidatedBlocked",
               test_api_DeleteResultValidatedBlocked);
}
