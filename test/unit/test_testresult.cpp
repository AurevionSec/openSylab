/**
 * @file test_testresult.cpp
 * @brief Unit-Tests für die TestResult-Klasse
 */

#include "core/TestResult.h"
#include "test_macros.h"

using namespace opensylab::core;

// Test-Funktionen
bool test_testresult_DefaultConstructor() {
  TestResult result;
  ASSERT_EQ(result.getId(), 0);
  ASSERT_TRUE(result.getResultId().empty());
  ASSERT_EQ(result.getOrderId(), 0);
  ASSERT_TRUE(result.getTestParameter().empty());
  ASSERT_TRUE(result.getValue().empty());
  ASSERT_TRUE(result.getUnit().empty());
  ASSERT_EQ(result.getStatus(), TestResult::Status::PENDING);
  ASSERT_EQ(result.getFlag(), TestResult::Flag::UNDEFINED);
  return true;
}

bool test_testresult_ParameterizedConstructor() {
  TestResult result("R001", 42, "Glucose");
  ASSERT_EQ(result.getResultId(), "R001");
  ASSERT_EQ(result.getOrderId(), 42);
  ASSERT_EQ(result.getTestParameter(), "Glucose");
  ASSERT_EQ(result.getStatus(), TestResult::Status::PENDING);
  ASSERT_EQ(result.getFlag(), TestResult::Flag::UNDEFINED);
  return true;
}

bool test_testresult_SettersAndGetters() {
  TestResult result;
  result.setId(99);
  result.setResultId("R002");
  result.setOrderId(5);
  result.setTestParameter("Hämoglobin");
  result.setValue("14.5");
  result.setUnit("g/dL");
  result.setReferenceRange("12-16");
  result.setReferenceLow(12.0);
  result.setReferenceHigh(16.0);
  result.setStatus(TestResult::Status::ENTERED);
  result.setFlag(TestResult::Flag::NORMAL);
  result.setMeasuredBy("Laborant1");
  result.setComment("Testkommentar");

  ASSERT_EQ(result.getId(), 99);
  ASSERT_EQ(result.getResultId(), "R002");
  ASSERT_EQ(result.getOrderId(), 5);
  ASSERT_EQ(result.getTestParameter(), "Hämoglobin");
  ASSERT_EQ(result.getValue(), "14.5");
  ASSERT_EQ(result.getUnit(), "g/dL");
  ASSERT_EQ(result.getReferenceRange(), "12-16");
  ASSERT_EQ(result.getReferenceLow(), 12.0);
  ASSERT_EQ(result.getReferenceHigh(), 16.0);
  ASSERT_EQ(result.getStatus(), TestResult::Status::ENTERED);
  ASSERT_EQ(result.getFlag(), TestResult::Flag::NORMAL);
  ASSERT_EQ(result.getMeasuredBy(), "Laborant1");
  ASSERT_EQ(result.getComment(), "Testkommentar");
  return true;
}

bool test_testresult_StatusToString() {
  ASSERT_EQ(TestResult::statusToString(TestResult::Status::PENDING),
            "Ausstehend");
  ASSERT_EQ(TestResult::statusToString(TestResult::Status::ENTERED),
            "Eingegeben");
  ASSERT_EQ(TestResult::statusToString(TestResult::Status::VALIDATED),
            "Validiert");
  ASSERT_EQ(TestResult::statusToString(TestResult::Status::REJECTED),
            "Abgelehnt");
  ASSERT_EQ(TestResult::statusToString(TestResult::Status::REPEATED),
            "Wiederholung");
  return true;
}

bool test_testresult_StringToStatus() {
  ASSERT_EQ(TestResult::stringToStatus("Ausstehend"),
            TestResult::Status::PENDING);
  ASSERT_EQ(TestResult::stringToStatus("PENDING"), TestResult::Status::PENDING);
  ASSERT_EQ(TestResult::stringToStatus("Eingegeben"),
            TestResult::Status::ENTERED);
  ASSERT_EQ(TestResult::stringToStatus("Validiert"),
            TestResult::Status::VALIDATED);
  ASSERT_EQ(TestResult::stringToStatus("Abgelehnt"),
            TestResult::Status::REJECTED);
  ASSERT_EQ(TestResult::stringToStatus("Wiederholung"),
            TestResult::Status::REPEATED);
  return true;
}

bool test_testresult_StatusRoundtrip() {
  for (auto status :
       {TestResult::Status::PENDING, TestResult::Status::ENTERED,
        TestResult::Status::VALIDATED, TestResult::Status::REJECTED,
        TestResult::Status::REPEATED}) {
    std::string statusStr = TestResult::statusToString(status);
    TestResult::Status roundtrip = TestResult::stringToStatus(statusStr);
    ASSERT_EQ(status, roundtrip);
  }
  return true;
}

bool test_testresult_FlagToString() {
  ASSERT_EQ(TestResult::flagToString(TestResult::Flag::NORMAL), "Normal");
  ASSERT_EQ(TestResult::flagToString(TestResult::Flag::LOW), "Niedrig");
  ASSERT_EQ(TestResult::flagToString(TestResult::Flag::HIGH), "Hoch");
  ASSERT_EQ(TestResult::flagToString(TestResult::Flag::CRITICAL), "Kritisch");
  ASSERT_EQ(TestResult::flagToString(TestResult::Flag::UNDEFINED),
            "Undefiniert");
  return true;
}

bool test_testresult_StringToFlag() {
  ASSERT_EQ(TestResult::stringToFlag("Normal"), TestResult::Flag::NORMAL);
  ASSERT_EQ(TestResult::stringToFlag("NORMAL"), TestResult::Flag::NORMAL);
  ASSERT_EQ(TestResult::stringToFlag("Niedrig"), TestResult::Flag::LOW);
  ASSERT_EQ(TestResult::stringToFlag("Hoch"), TestResult::Flag::HIGH);
  ASSERT_EQ(TestResult::stringToFlag("Kritisch"), TestResult::Flag::CRITICAL);
  ASSERT_EQ(TestResult::stringToFlag("Undefiniert"),
            TestResult::Flag::UNDEFINED);
  return true;
}

bool test_testresult_FlagRoundtrip() {
  for (auto flag :
       {TestResult::Flag::NORMAL, TestResult::Flag::LOW, TestResult::Flag::HIGH,
        TestResult::Flag::CRITICAL, TestResult::Flag::UNDEFINED}) {
    std::string flagStr = TestResult::flagToString(flag);
    TestResult::Flag roundtrip = TestResult::stringToFlag(flagStr);
    ASSERT_EQ(flag, roundtrip);
  }
  return true;
}

bool test_testresult_IsNumeric() {
  TestResult result;

  result.setValue("123.45");
  ASSERT_TRUE(result.isNumeric());

  result.setValue("-42.5");
  ASSERT_TRUE(result.isNumeric());

  result.setValue("  100  ");
  ASSERT_TRUE(result.isNumeric());

  result.setValue("abc");
  ASSERT_FALSE(result.isNumeric());

  result.setValue("");
  ASSERT_FALSE(result.isNumeric());

  result.setValue("12.34abc");
  ASSERT_FALSE(result.isNumeric());

  return true;
}

bool test_testresult_GetNumericValue() {
  TestResult result;

  result.setValue("123.45");
  ASSERT_EQ(result.getNumericValue(), 123.45);

  result.setValue("-42.5");
  ASSERT_EQ(result.getNumericValue(), -42.5);

  result.setValue("  100  ");
  ASSERT_EQ(result.getNumericValue(), 100.0);

  return true;
}

bool test_testresult_EvaluateFlag_Normal() {
  TestResult result;
  result.setValue("85");
  result.setReferenceLow(70.0);
  result.setReferenceHigh(100.0);

  TestResult::Flag flag = result.evaluateFlag();
  ASSERT_EQ(flag, TestResult::Flag::NORMAL);
  return true;
}

bool test_testresult_EvaluateFlag_Low() {
  TestResult result;
  result.setValue("50");
  result.setReferenceLow(70.0);
  result.setReferenceHigh(100.0);

  TestResult::Flag flag = result.evaluateFlag();
  ASSERT_EQ(flag, TestResult::Flag::LOW);
  return true;
}

bool test_testresult_EvaluateFlag_High() {
  TestResult result;
  result.setValue("150");
  result.setReferenceLow(70.0);
  result.setReferenceHigh(100.0);

  TestResult::Flag flag = result.evaluateFlag();
  ASSERT_EQ(flag, TestResult::Flag::HIGH);
  return true;
}

bool test_testresult_EvaluateFlag_CriticalLow() {
  TestResult result;
  result.setValue("20");
  result.setReferenceLow(70.0);
  result.setReferenceHigh(100.0);

  TestResult::Flag flag = result.evaluateFlag();
  ASSERT_EQ(flag, TestResult::Flag::CRITICAL);
  return true;
}

bool test_testresult_EvaluateFlag_CriticalHigh() {
  TestResult result;
  result.setValue("200");
  result.setReferenceLow(70.0);
  result.setReferenceHigh(100.0);

  TestResult::Flag flag = result.evaluateFlag();
  ASSERT_EQ(flag, TestResult::Flag::CRITICAL);
  return true;
}

bool test_testresult_EvaluateFlag_MissingReference() {
  TestResult result;
  result.setValue("85");
  result.setReferenceLow(70.0);
  result.setReferenceHigh(0.0);

  TestResult::Flag flag = result.evaluateFlag();
  ASSERT_EQ(flag, TestResult::Flag::UNDEFINED);
  return true;
}

bool test_testresult_EvaluateFlag_NoReference() {
  TestResult result;
  result.setValue("85");
  // Keine Referenzwerte gesetzt (beide 0.0)

  TestResult::Flag flag = result.evaluateFlag();
  ASSERT_EQ(flag, TestResult::Flag::UNDEFINED);
  return true;
}

bool test_testresult_EvaluateFlag_NonNumeric() {
  TestResult result;
  result.setValue("positiv");
  result.setReferenceLow(70.0);
  result.setReferenceHigh(100.0);

  TestResult::Flag flag = result.evaluateFlag();
  ASSERT_EQ(flag, TestResult::Flag::UNDEFINED);
  return true;
}

void registerTestResultTests() {
  registerTest("TestResult::DefaultConstructor",
               test_testresult_DefaultConstructor);
  registerTest("TestResult::ParameterizedConstructor",
               test_testresult_ParameterizedConstructor);
  registerTest("TestResult::SettersAndGetters",
               test_testresult_SettersAndGetters);
  registerTest("TestResult::StatusToString", test_testresult_StatusToString);
  registerTest("TestResult::StringToStatus", test_testresult_StringToStatus);
  registerTest("TestResult::StatusRoundtrip", test_testresult_StatusRoundtrip);
  registerTest("TestResult::FlagToString", test_testresult_FlagToString);
  registerTest("TestResult::StringToFlag", test_testresult_StringToFlag);
  registerTest("TestResult::FlagRoundtrip", test_testresult_FlagRoundtrip);
  registerTest("TestResult::IsNumeric", test_testresult_IsNumeric);
  registerTest("TestResult::GetNumericValue", test_testresult_GetNumericValue);
  registerTest("TestResult::EvaluateFlag_Normal",
               test_testresult_EvaluateFlag_Normal);
  registerTest("TestResult::EvaluateFlag_Low",
               test_testresult_EvaluateFlag_Low);
  registerTest("TestResult::EvaluateFlag_High",
               test_testresult_EvaluateFlag_High);
  registerTest("TestResult::EvaluateFlag_CriticalLow",
               test_testresult_EvaluateFlag_CriticalLow);
  registerTest("TestResult::EvaluateFlag_CriticalHigh",
               test_testresult_EvaluateFlag_CriticalHigh);
  registerTest("TestResult::EvaluateFlag_MissingReference",
               test_testresult_EvaluateFlag_MissingReference);
  registerTest("TestResult::EvaluateFlag_NoReference",
               test_testresult_EvaluateFlag_NoReference);
  registerTest("TestResult::EvaluateFlag_NonNumeric",
               test_testresult_EvaluateFlag_NonNumeric);
}
