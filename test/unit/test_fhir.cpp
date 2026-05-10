/**
 * @file test_fhir.cpp
 * @brief Unit-Tests für FHIR Parser und Mapping
 */

#include "db/Database.h"
#include "test_macros.h"
#include "utils/Fhir.h"
#include <cstdio>
#include <string>

using namespace opensylab::utils;

bool test_fhir_ParseValidBundle() {
  const std::string payload =
      "{\"resourceType\":\"Bundle\",\"type\":\"collection\",\"entry\":["
      "{\"resource\":{\"resourceType\":\"Patient\",\"identifier\":[{\"value\":\"P123\"}],\"name\":[{\"text\":\"Doe^John\"}]}}"
      ",{\"resource\":{\"resourceType\":\"Specimen\",\"identifier\":[{\"value\":\"S123\"}]}}"
      ",{\"resource\":{\"resourceType\":\"ServiceRequest\",\"identifier\":[{\"value\":\"O123\"}],\"code\":{\"text\":\"GLU\"}}}"
      ",{\"resource\":{\"resourceType\":\"Observation\",\"code\":{\"text\":\"GLU\"},\"valueQuantity\":{\"value\":5.6,\"unit\":\"mg/dL\"},\"referenceRange\":[{\"low\":{\"value\":4},\"high\":{\"value\":6}}]}}"
      "]}";

  FhirParser parser;
  ASSERT_TRUE(parser.parse(payload));

  FhirParser::MappedData mapped;
  ASSERT_TRUE(parser.mapBundle(mapped));
  ASSERT_EQ(mapped.sample.patientId, "P123");
  ASSERT_EQ(mapped.sample.sampleId, "S123");
  ASSERT_EQ(mapped.order.orderId, "O123");
  ASSERT_EQ(mapped.results.size(), static_cast<size_t>(1));
  ASSERT_EQ(mapped.results[0].parameter, "GLU");
  ASSERT_EQ(mapped.results[0].value, "5.6");
  ASSERT_EQ(mapped.results[0].unit, "mg/dL");

  return true;
}

bool test_fhir_ImportCreatesEntities() {
  std::string dbPath = "test_fhir_import.db";
  auto db = std::make_shared<opensylab::db::Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());

  const std::string payload =
      "{\"resourceType\":\"Bundle\",\"type\":\"collection\",\"entry\":["
      "{\"resource\":{\"resourceType\":\"Patient\",\"identifier\":[{\"value\":\"P999\"}],\"name\":[{\"text\":\"Doe^Jane\"}]}}"
      ",{\"resource\":{\"resourceType\":\"Specimen\",\"identifier\":[{\"value\":\"S999\"}]}}"
      ",{\"resource\":{\"resourceType\":\"ServiceRequest\",\"identifier\":[{\"value\":\"O999\"}],\"code\":{\"text\":\"HB\"}}}"
      ",{\"resource\":{\"resourceType\":\"Observation\",\"code\":{\"text\":\"HB\"},\"valueQuantity\":{\"value\":12.1,\"unit\":\"g/dL\"}}}"
      "]}";

  FhirExchange exchange(db);
  FhirExchange::ImportSummary summary;
  ASSERT_TRUE(exchange.importBundle(payload, "tester", summary));
  ASSERT_EQ(summary.samplesCreated, 1);
  ASSERT_EQ(summary.ordersCreated, 1);
  ASSERT_EQ(summary.resultsCreated, 1);

  auto sample = db->getSampleByBarcode("S999");
  ASSERT_TRUE(sample != nullptr);
  auto order = db->getOrderByOrderId("O999");
  ASSERT_TRUE(order != nullptr);
  auto result = db->getTestResultByResultId("O999-1");
  ASSERT_TRUE(result != nullptr);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_fhir_ImportLogsErrors() {
  std::string dbPath = "test_fhir_errors.db";
  auto db = std::make_shared<opensylab::db::Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());

  const std::string payload =
      "{\"resourceType\":\"Bundle\",\"type\":\"collection\",\"entry\":["
      "{\"resource\":{\"resourceType\":\"Patient\",\"identifier\":[{\"value\":\"\"}]}}"
      ",{\"resource\":{\"resourceType\":\"Specimen\",\"identifier\":[{\"value\":\"SERR\"}]}}"
      ",{\"resource\":{\"resourceType\":\"ServiceRequest\",\"identifier\":[{\"value\":\"OERR\"}],\"code\":{\"text\":\"GLU\"}}}"
      ",{\"resource\":{\"resourceType\":\"Observation\",\"code\":{\"text\":\"GLU\"},\"valueQuantity\":{\"value\":\"\",\"unit\":\"mg/dL\"}}}"
      "]}";

  FhirExchange exchange(db);
  FhirExchange::ImportSummary summary;
  ASSERT_FALSE(exchange.importBundle(payload, "tester", summary));
  ASSERT_FALSE(summary.operationOutcome.empty());
  ASSERT_NE(summary.operationOutcome.find("\"code\":\"invalid\""),
            std::string::npos);

  auto entries =
      db->getAuditLogByEntity(opensylab::core::AuditEntry::EntityType::SYSTEM,
                              "fhir");
  ASSERT_FALSE(entries.empty());

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_fhir_ExportBundleContainsResources() {
  opensylab::core::Sample sample("S1", "P1");
  sample.setPatientName("Doe^Jane");
  opensylab::core::Order order("O1", "S1", "GLU");

  opensylab::core::TestResult result("R1", 1, "GLU");
  result.setValue("5.5");
  result.setUnit("mg/dL");
  result.setReferenceRange("4-6");
  result.setReferenceLow(4.0);
  result.setReferenceHigh(6.0);
  std::vector<opensylab::core::TestResult> results = {result};

  FhirExchange exchange(nullptr);
  const std::string payload = exchange.exportBundle(sample, order, results);
  ASSERT_NE(payload.find("\"resourceType\":\"Bundle\""), std::string::npos);
  ASSERT_NE(payload.find("\"resourceType\":\"Patient\""), std::string::npos);
  ASSERT_NE(payload.find("\"resourceType\":\"Specimen\""), std::string::npos);
  ASSERT_NE(payload.find("\"resourceType\":\"Observation\""),
            std::string::npos);
  ASSERT_NE(payload.find("\"referenceRange\""), std::string::npos);
  ASSERT_NE(payload.find("\"low\""), std::string::npos);
  ASSERT_NE(payload.find("\"high\""), std::string::npos);

  return true;
}

bool test_fhir_ParseBundleWithBraceInString() {
  // Regression test: findObjectStart must not pick up '{' inside a string value
  const std::string tricky = R"FHIR(
    {"resourceType":"Bundle","entry":[
      {"resource":{"resourceType":"Patient",
       "note":"{acme} lab ref",
       "identifier":[{"value":"P-TRICKY"}],
       "name":[{"text":"Tricky Patient"}]}},
      {"resource":{"resourceType":"Specimen",
       "identifier":[{"value":"S-TRICKY"}]}},
      {"resource":{"resourceType":"ServiceRequest",
       "identifier":[{"value":"O-TRICKY"}]}},
      {"resource":{"resourceType":"Observation",
       "id":"obs-1",
       "code":{"coding":[{"code":"GLU","display":"Glucose"}]},
       "valueQuantity":{"value":5.5,"unit":"mg/dL"},
       "referenceRange":[{"text":"4-7"}]}}
    ]})FHIR";

  FhirParser parser;
  ASSERT_TRUE(parser.parse(tricky));

  FhirParser::MappedData mapped;
  ASSERT_TRUE(parser.mapBundle(mapped));
  ASSERT_EQ(mapped.sample.patientId, std::string("P-TRICKY"));
  ASSERT_EQ(mapped.sample.sampleId, std::string("S-TRICKY"));
  ASSERT_EQ(mapped.order.orderId, std::string("O-TRICKY"));

  return true;
}

void registerFhirTests() {
  registerTest("FHIR::ParseValidBundle", test_fhir_ParseValidBundle);
  registerTest("FHIR::ImportCreatesEntities", test_fhir_ImportCreatesEntities);
  registerTest("FHIR::ImportLogsErrors", test_fhir_ImportLogsErrors);
  registerTest("FHIR::ExportBundleContainsResources",
               test_fhir_ExportBundleContainsResources);
  registerTest("FHIR::ParseBundleWithBraceInString",
               test_fhir_ParseBundleWithBraceInString);
}
