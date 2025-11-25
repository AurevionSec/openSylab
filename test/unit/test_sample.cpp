/**
 * @file test_sample.cpp
 * @brief Unit-Tests für die Sample-Klasse
 */

#include "core/Sample.h"
#include "test_macros.h"

using namespace opensylab::core;

// Test-Funktionen
bool test_sample_DefaultConstructor() {
    Sample sample;
    ASSERT_EQ(sample.getId(), 0);
    ASSERT_TRUE(sample.getSampleId().empty());
    ASSERT_EQ(sample.getStatus(), Sample::Status::REGISTERED);
    return true;
}

bool test_sample_ParameterizedConstructor() {
    Sample sample("S001", "P12345");
    ASSERT_EQ(sample.getSampleId(), "S001");
    ASSERT_EQ(sample.getPatientId(), "P12345");
    ASSERT_EQ(sample.getStatus(), Sample::Status::REGISTERED);
    return true;
}

bool test_sample_SettersAndGetters() {
    Sample sample;
    sample.setSampleId("S002");
    sample.setPatientId("P67890");
    sample.setPatientName("Test Patient");
    sample.setDescription("Test Description");
    sample.setStatus(Sample::Status::IN_ANALYSIS);

    ASSERT_EQ(sample.getSampleId(), "S002");
    ASSERT_EQ(sample.getPatientId(), "P67890");
    ASSERT_EQ(sample.getPatientName(), "Test Patient");
    ASSERT_EQ(sample.getDescription(), "Test Description");
    ASSERT_EQ(sample.getStatus(), Sample::Status::IN_ANALYSIS);
    return true;
}

bool test_sample_StatusToString() {
    ASSERT_EQ(Sample::statusToString(Sample::Status::REGISTERED), "Erfasst");
    ASSERT_EQ(Sample::statusToString(Sample::Status::IN_ANALYSIS), "In Analyse");
    ASSERT_EQ(Sample::statusToString(Sample::Status::ANALYZED), "Analysiert");
    ASSERT_EQ(Sample::statusToString(Sample::Status::VALIDATED), "Validiert");
    ASSERT_EQ(Sample::statusToString(Sample::Status::ARCHIVED), "Archiviert");
    return true;
}

bool test_sample_StringToStatus() {
    ASSERT_EQ(Sample::stringToStatus("Erfasst"), Sample::Status::REGISTERED);
    ASSERT_EQ(Sample::stringToStatus("REGISTERED"), Sample::Status::REGISTERED);
    ASSERT_EQ(Sample::stringToStatus("In Analyse"), Sample::Status::IN_ANALYSIS);
    ASSERT_EQ(Sample::stringToStatus("Validiert"), Sample::Status::VALIDATED);
    return true;
}

bool test_sample_StatusRoundtrip() {
    for (auto status : {Sample::Status::REGISTERED, Sample::Status::IN_ANALYSIS,
                        Sample::Status::ANALYZED, Sample::Status::VALIDATED,
                        Sample::Status::ARCHIVED}) {
        std::string statusStr = Sample::statusToString(status);
        Sample::Status roundtrip = Sample::stringToStatus(statusStr);
        ASSERT_EQ(status, roundtrip);
    }
    return true;
}

void registerSampleTests() {
    registerTest("Sample::DefaultConstructor", test_sample_DefaultConstructor);
    registerTest("Sample::ParameterizedConstructor", test_sample_ParameterizedConstructor);
    registerTest("Sample::SettersAndGetters", test_sample_SettersAndGetters);
    registerTest("Sample::StatusToString", test_sample_StatusToString);
    registerTest("Sample::StringToStatus", test_sample_StringToStatus);
    registerTest("Sample::StatusRoundtrip", test_sample_StatusRoundtrip);
}
