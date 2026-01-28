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

  ApiRouter router(db);
  ApiRequest req;
  req.method = "GET";
  req.path = "/api/v1/samples";
  req.headers["x-api-key"] = "testkey";

  ApiResponse res = router.handleRequest(req);
  ASSERT_EQ(res.status, 200);
  ASSERT_NE(res.body.find("\"sample_id\":\"S_API_LIST\""),
            std::string::npos);

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
}
