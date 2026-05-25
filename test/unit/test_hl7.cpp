/**
 * @file test_hl7.cpp
 * @brief Unit-Tests für HL7 Parser und Mapping
 */

#include "db/Database.h"
#include "test_macros.h"
#include "utils/Hl7.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <atomic>
#include <chrono>
#include <sstream>
#include <string>

using namespace opensylab::utils;

namespace {
std::string uniqueDbPath() {
  static std::atomic<int> counter{0};
  auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  std::ostringstream ss;
  ss << "test_hl7_" << ts << "_" << counter++ << ".db";
  return ss.str();
}
} // namespace

bool test_hl7_ParseValidOruR01() {
  const std::string message =
      "MSH|^~\\&|LAB|HOSP|LIS|HOSP|202401011200||ORU^R01|MSG1|P|2.5.1\r"
      "PID|1||P123||Doe^John\r"
      "OBR|1|ORD123|SAMPLE123|GLU^Glucose\r"
      "OBX|1|NM|GLU||5.6|mg/dL|4-6\r";

  Hl7Parser parser;
  ASSERT_TRUE(parser.parse(message));
  ASSERT_TRUE(parser.validateOruR01());

  Hl7Parser::MappedData mapped;
  ASSERT_TRUE(parser.mapOruR01(mapped));

  ASSERT_EQ(mapped.sample.sampleId, "SAMPLE123");
  ASSERT_EQ(mapped.sample.patientId, "P123");
  ASSERT_EQ(mapped.order.orderId, "ORD123");
  ASSERT_EQ(mapped.results.size(), static_cast<size_t>(1));
  ASSERT_EQ(mapped.results[0].resultId, "ORD123-1");
  ASSERT_EQ(mapped.results[0].parameter, "GLU");
  ASSERT_EQ(mapped.results[0].value, "5.6");
  ASSERT_EQ(mapped.results[0].unit, "mg/dL");

  return true;
}

bool test_hl7_MissingSegmentReported() {
  const std::string message =
      "MSH|^~\\&|LAB|HOSP|LIS|HOSP|202401011200||ORU^R01|MSG2|P|2.5.1\r"
      "PID|1||P123||Doe^John\r"
      "OBR|1|ORD123|SAMPLE123|GLU^Glucose\r";

  Hl7Parser parser;
  ASSERT_TRUE(parser.parse(message));
  ASSERT_FALSE(parser.validateOruR01());
  ASSERT_FALSE(parser.getErrors().empty());

  bool foundObx = false;
  for (const auto &err : parser.getErrors()) {
    if (err.segment == "OBX") {
      foundObx = true;
      break;
    }
  }
  ASSERT_TRUE(foundObx);

  return true;
}

bool test_hl7_InvalidMessageType() {
  const std::string message =
      "MSH|^~\\&|LAB|HOSP|LIS|HOSP|202401011200||ADT^A01|MSG3|P|2.5.1\r"
      "PID|1||P123||Doe^John\r"
      "OBR|1|ORD123|SAMPLE123|GLU^Glucose\r"
      "OBX|1|NM|GLU||5.6|mg/dL|4-6\r";

  Hl7Parser parser;
  ASSERT_TRUE(parser.parse(message));
  ASSERT_FALSE(parser.validateOruR01());
  ASSERT_FALSE(parser.getErrors().empty());

  bool foundMsh = false;
  for (const auto &err : parser.getErrors()) {
    if (err.segment == "MSH") {
      foundMsh = true;
      break;
    }
  }
  ASSERT_TRUE(foundMsh);

  return true;
}

bool test_hl7_ImportCreatesEntities() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<opensylab::db::Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());

  const std::string message =
      "MSH|^~\\&|LAB|HOSP|LIS|HOSP|202401011200||ORU^R01|MSG1|P|2.5.1\r"
      "PID|1||P999||Doe^Jane\r"
      "OBR|1|ORD999|SAMP999|HB^Hemoglobin\r"
      "OBX|1|NM|HB||12.1|g/dL|11-15\r";

  Hl7Exchange exchange(db);
  Hl7Exchange::ImportSummary summary;
  ASSERT_TRUE(exchange.importOruR01Message(message, "tester", summary));
  ASSERT_EQ(summary.samplesCreated, 1);
  ASSERT_EQ(summary.ordersCreated, 1);
  ASSERT_EQ(summary.resultsCreated, 1);

  auto sample = db->getSampleByBarcode("SAMP999");
  ASSERT_TRUE(sample != nullptr);
  auto order = db->getOrderByOrderId("ORD999");
  ASSERT_TRUE(order != nullptr);
  auto result = db->getTestResultByResultId("ORD999-1");
  ASSERT_TRUE(result != nullptr);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_hl7_ImportLogsErrors() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<opensylab::db::Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());

  const std::string message =
      "MSH|^~\\&|LAB|HOSP|LIS|HOSP|202401011200||ORU^R01|MSG2|P|2.5.1\r"
      "PID|1||||\r"
      "OBR|1|ORD_ERR|SAMP_ERR|GLU^Glucose\r"
      "OBX|1|NM|GLU||||\r";

  Hl7Exchange exchange(db);
  Hl7Exchange::ImportSummary summary;
  ASSERT_FALSE(exchange.importOruR01Message(message, "tester", summary));
  ASSERT_FALSE(summary.errors.empty());

  bool hasLineContext = false;
  for (const auto &err : summary.errors) {
    if (err.line > 0 && !err.segment.empty()) {
      hasLineContext = true;
      break;
    }
  }
  ASSERT_TRUE(hasLineContext);

  auto entries =
      db->getAuditLogByEntity(opensylab::core::AuditEntry::EntityType::SYSTEM,
                              "hl7");
  ASSERT_FALSE(entries.empty());
  ASSERT_NE(entries[0]->getDetails().find("message_id=MSG2"),
            std::string::npos);

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_hl7_ExportMessageContainsSegments() {
  opensylab::core::Sample sample("S1", "P1");
  sample.setPatientName("Doe^Jane");
  opensylab::core::Order order("O1", "S1", "GLU");

  opensylab::core::TestResult result("R1", 1, "GLU");
  result.setValue("5.5");
  result.setUnit("mg/dL");
  result.setReferenceRange("4-6");

  std::vector<opensylab::core::TestResult> results = {result};

  Hl7Exchange exchange(nullptr);
  const std::string message =
      exchange.exportOruR01Message(sample, order, results);

  ASSERT_NE(message.find("MSH|"), std::string::npos);
  ASSERT_NE(message.find("PID|"), std::string::npos);
  ASSERT_NE(message.find("OBR|"), std::string::npos);
  ASSERT_NE(message.find("OBX|1|ST|GLU"), std::string::npos);
  // Verify HL7 special characters are escaped (^ in patient name)
  ASSERT_NE(message.find("Doe\\S\\Jane"), std::string::npos);
  ASSERT_EQ(message.find("Doe^Jane"), std::string::npos);

  return true;
}

bool test_hl7_ExportEscapesSpecialChars() {
  opensylab::core::Sample sample("S_ESC", "P_ESC");
  sample.setPatientName("O'Brien|Pipe&Amp");
  opensylab::core::Order order("O_ESC", "S_ESC", "PARAM|WITH|PIPE");

  opensylab::core::TestResult result("R_ESC", 1, "Param^Value");
  result.setValue("1.0|extra");
  result.setUnit("mg/dL");
  std::vector<opensylab::core::TestResult> results = {result};

  Hl7Exchange exchange(nullptr);
  const std::string message = exchange.exportOruR01Message(sample, order, results);

  // No unescaped HL7 delimiters in data fields (check that | ^ ~ & \ are escaped)
  // Patient name field: "O'Brien|Pipe&Amp" -> "O'Brien\F\Pipe\T\Amp"
  ASSERT_NE(message.find("O'Brien\\F\\Pipe\\T\\Amp"), std::string::npos);
  // Test parameter: "Param^Value" -> "Param\S\Value"
  ASSERT_NE(message.find("Param\\S\\Value"), std::string::npos);

  return true;
}

bool test_hl7_ImportPartialObservationValidationAbortsAllWrites() {
  std::string dbPath = uniqueDbPath();
  auto db = std::make_shared<opensylab::db::Database>(dbPath);
  ASSERT_TRUE(db->open());
  ASSERT_TRUE(db->initializeSchema());

  // Valid PID/OBR, two OBX: first valid, second missing value (field 5 empty)
  const std::string message =
      "MSH|^~\\&|LAB|HOSP|LIS|HOSP|202401011200||ORU^R01|MSG_PAR|P|2.5.1\r"
      "PID|1||SAMP_PAR||Partial^Patient\r"
      "OBR|1|ORD_PAR|SAMP_PAR|HB^Hemoglobin\r"
      "OBX|1|NM|HB||12.1|g/dL|11-15\r"
      "OBX|2|NM|GLU||||\r";  // missing value triggers validation error

  Hl7Exchange exchange(db);
  Hl7Exchange::ImportSummary summary;
  ASSERT_FALSE(exchange.importOruR01Message(message, "tester", summary));
  ASSERT_FALSE(summary.errors.empty());
  ASSERT_EQ(summary.resultsCreated, 0);

  // No sample, no order must have been written (pre-validation must abort all writes)
  ASSERT_NULL(db->getSampleByBarcode("SAMP_PAR"));
  ASSERT_NULL(db->getOrderByOrderId("ORD_PAR"));

  db->close();
  std::remove(dbPath.c_str());
  return true;
}

void registerHl7Tests() {
  registerTest("HL7::ParseValidOruR01", test_hl7_ParseValidOruR01);
  registerTest("HL7::MissingSegmentReported", test_hl7_MissingSegmentReported);
  registerTest("HL7::InvalidMessageType", test_hl7_InvalidMessageType);
  registerTest("HL7::ImportCreatesEntities", test_hl7_ImportCreatesEntities);
  registerTest("HL7::ImportLogsErrors", test_hl7_ImportLogsErrors);
  registerTest("HL7::ExportMessageContainsSegments",
               test_hl7_ExportMessageContainsSegments);
  registerTest("HL7::ExportEscapesSpecialChars",
               test_hl7_ExportEscapesSpecialChars);
  registerTest("HL7::ImportPartialObservationValidationAbortsAllWrites",
               test_hl7_ImportPartialObservationValidationAbortsAllWrites);
}
