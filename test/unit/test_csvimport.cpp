/**
 * @file test_csvimport.cpp
 * @brief Unit-Tests für die CsvImport-Klasse
 */

#include "test_macros.h"
#include "utils/CsvImport.h"
#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace opensylab::utils;
using namespace opensylab::core;

namespace {
// Generiert eindeutigen Dateinamen für parallele Tests
std::string uniqueCsvPath() {
  std::ostringstream ss;
  ss << "test_csv_" << std::rand() << "_" << std::time(nullptr) << ".csv";
  return ss.str();
}

void createTestCsv(const std::string &path, const std::string &content) {
  std::ofstream file(path);
  file << content;
  file.close();
}
} // namespace

bool test_csvimport_ImportValidCsv() {
  std::string csvPath = uniqueCsvPath();
  createTestCsv(csvPath,
                "sample_id,patient_id,patient_name,description,status\n"
                "S001,P001,Test Patient 1,Test,Erfasst\n"
                "S002,P002,Test Patient 2,Test,Erfasst\n");

  CsvImport importer;
  auto samples = importer.importSamples(csvPath);

  ASSERT_EQ(samples.size(), static_cast<size_t>(2));
  ASSERT_EQ(importer.getImportedCount(), 2);

  std::remove(csvPath.c_str());
  return true;
}

bool test_csvimport_ImportWithMissingFields() {
  std::string csvPath = uniqueCsvPath();
  createTestCsv(csvPath,
                "sample_id,patient_id\n"
                "S001\n" // Fehlt patient_id
  );

  CsvImport importer;
  auto samples = importer.importSamples(csvPath);

  ASSERT_EQ(samples.size(), static_cast<size_t>(0)); // Fehler: zu wenig Felder
  ASSERT_FALSE(importer.getLastError().empty());

  std::remove(csvPath.c_str());
  return true;
}

bool test_csvimport_ImportWithEmptyId() {
  std::string csvPath = uniqueCsvPath();
  createTestCsv(csvPath,
                "sample_id,patient_id\n"
                ",P001\n" // Leere sample_id
  );

  CsvImport importer;
  auto samples = importer.importSamples(csvPath);

  ASSERT_EQ(samples.size(), static_cast<size_t>(0)); // Fehler: leere ID
  ASSERT_EQ(importer.getImportedCount(), 0);

  std::remove(csvPath.c_str());
  return true;
}

bool test_csvimport_ImportWithWhitespaceId() {
  std::string csvPath = uniqueCsvPath();
  createTestCsv(csvPath,
                "sample_id,patient_id\n"
                "   ,P001\n" // Nur Whitespace
  );

  CsvImport importer;
  auto samples = importer.importSamples(csvPath);

  ASSERT_EQ(samples.size(), static_cast<size_t>(0)); // Fehler: nur Whitespace
  ASSERT_EQ(importer.getImportedCount(), 0);

  std::remove(csvPath.c_str());
  return true;
}

bool test_csvimport_ImportMixedValidAndInvalid() {
  std::string csvPath = uniqueCsvPath();
  createTestCsv(csvPath,
                "sample_id,patient_id\n"
                "S001,P001\n" // Gültig
                ",P002\n"     // Ungültig
                "S003,P003\n" // Gültig
  );

  CsvImport importer;
  auto samples = importer.importSamples(csvPath);

  ASSERT_EQ(samples.size(), static_cast<size_t>(2)); // Nur 2 gültige
  ASSERT_EQ(importer.getImportedCount(), 2);

  std::remove(csvPath.c_str());
  return true;
}

bool test_csvimport_ImportEmptyFile() {
  std::string csvPath = uniqueCsvPath();
  createTestCsv(csvPath, ""); // Leere Datei

  CsvImport importer;
  auto samples = importer.importSamples(csvPath);

  ASSERT_EQ(samples.size(), static_cast<size_t>(0));
  ASSERT_FALSE(importer.getLastError().empty());

  std::remove(csvPath.c_str());
  return true;
}

bool test_csvimport_ImportQuotedFields() {
  std::string csvPath = uniqueCsvPath();
  createTestCsv(csvPath,
                "sample_id,patient_id,patient_name\n"
                "S001,P001,\"Mustermann, Max\"\n" // Komma in Anführungszeichen
                "S002,P002,\"Test \"\"Name\"\"\"\n" // Escaped Anführungszeichen
  );

  CsvImport importer;
  auto samples = importer.importSamples(csvPath);

  ASSERT_EQ(samples.size(), static_cast<size_t>(2));
  ASSERT_EQ(samples[0].getPatientName(), "Mustermann, Max");
  ASSERT_EQ(samples[1].getPatientName(), "Test \"Name\"");

  std::remove(csvPath.c_str());
  return true;
}

bool test_csvimport_ImportMultilineFields() {
  std::string csvPath = uniqueCsvPath();
  // RFC 4180: Felder in Anführungszeichen können Zeilenumbrüche enthalten
  createTestCsv(csvPath,
                "sample_id,patient_id,patient_name,description\n"
                "S001,P001,Test Patient,\"Zeile 1\nZeile 2\nZeile 3\"\n"
                "S002,P002,Test Patient 2,Normal\n");

  CsvImport importer;
  auto samples = importer.importSamples(csvPath);

  ASSERT_EQ(samples.size(), static_cast<size_t>(2));
  ASSERT_EQ(samples[0].getDescription(), "Zeile 1\nZeile 2\nZeile 3");
  ASSERT_EQ(samples[1].getDescription(), "Normal");

  std::remove(csvPath.c_str());
  return true;
}

bool test_csvimport_ImportUnclosedQuotes() {
  std::string csvPath = uniqueCsvPath();
  // Nicht geschlossene Anführungszeichen sollten erkannt werden
  createTestCsv(csvPath,
                "sample_id,patient_id,patient_name\n"
                "S001,P001,\"Unclosed quote\n" // Fehlendes schließendes "
  );

  CsvImport importer;
  auto samples = importer.importSamples(csvPath);

  // Der Import sollte trotzdem versuchen, die Daten zu verarbeiten
  // aber einen Fehler melden (getLastError ist nicht leer wenn 0 Samples)
  // Wichtig: Der Parser stürzt nicht ab
  ASSERT_TRUE(samples.size() <= 1); // Maximal 1 (fehlerhafter) Record

  std::remove(csvPath.c_str());
  return true;
}

bool test_csvimport_FailedRowsTracked() {
  std::string csvPath = uniqueCsvPath();
  createTestCsv(csvPath,
                "sample_id,patient_id\n"
                "S001,P001\n"
                ",P002\n"
                "S003,P003\n");

  CsvImport importer;
  auto samples = importer.importSamples(csvPath);

  ASSERT_EQ(samples.size(), static_cast<size_t>(2));
  ASSERT_EQ(importer.getImportedCount(), 2);
  ASSERT_EQ(importer.getFailedCount(), 1);
  ASSERT_EQ(importer.getFailedRecords().size(), static_cast<size_t>(1));
  ASSERT_EQ(importer.getFailedRecords()[0].recordNumber, 2);

  std::remove(csvPath.c_str());
  return true;
}

bool test_csvimport_RetryCsvContainsFailedRows() {
  std::string csvPath = uniqueCsvPath();
  std::string retryPath = uniqueCsvPath();
  createTestCsv(csvPath,
                "sample_id,patient_id\n"
                ",P001\n"
                "S002\n");

  CsvImport importer;
  auto samples = importer.importSamples(csvPath);

  ASSERT_EQ(samples.size(), static_cast<size_t>(0));
  ASSERT_EQ(importer.getFailedCount(), 2);
  ASSERT_TRUE(importer.writeRetryCsv(retryPath));

  std::ifstream retryFile(retryPath);
  std::stringstream buffer;
  buffer << retryFile.rdbuf();
  retryFile.close();

  std::string expected =
      "sample_id,patient_id\n"
      ",P001\n"
      "S002\n";
  ASSERT_EQ(buffer.str(), expected);

  std::remove(csvPath.c_str());
  std::remove(retryPath.c_str());
  return true;
}

bool test_csvimport_InvalidStatusReported() {
  std::string csvPath = uniqueCsvPath();
  createTestCsv(csvPath,
                "sample_id,patient_id,patient_name,description,status\n"
                "S001,P001,Test,Desc,INVALID\n");

  CsvImport importer;
  auto samples = importer.importSamples(csvPath);

  ASSERT_EQ(samples.size(), static_cast<size_t>(0));
  ASSERT_EQ(importer.getFailedCount(), 1);
  ASSERT_TRUE(importer.getFailedRecords()[0].error.find("Ungueltiger Status") !=
              std::string::npos);

  std::remove(csvPath.c_str());
  return true;
}

bool test_csvimport_RetryCsvIncludesExtraFailures() {
  std::string csvPath = uniqueCsvPath();
  std::string retryPath = uniqueCsvPath();
  createTestCsv(csvPath,
                "sample_id,patient_id\n"
                "S001,P001\n");

  CsvImport importer;
  importer.importSamples(csvPath);

  std::vector<CsvImport::FailedRecord> extraFailed;
  extraFailed.push_back({2, "S002,P002", "DB Fehler"});

  ASSERT_TRUE(importer.writeRetryCsv(retryPath, extraFailed));

  std::ifstream retryFile(retryPath);
  std::stringstream buffer;
  buffer << retryFile.rdbuf();
  retryFile.close();

  std::string expected =
      "sample_id,patient_id\n"
      "S002,P002\n";
  ASSERT_EQ(buffer.str(), expected);

  std::remove(csvPath.c_str());
  std::remove(retryPath.c_str());
  return true;
}

void registerCsvImportTests() {
  registerTest("CsvImport::ImportValidCsv", test_csvimport_ImportValidCsv);
  registerTest("CsvImport::ImportWithMissingFields",
               test_csvimport_ImportWithMissingFields);
  registerTest("CsvImport::ImportWithEmptyId",
               test_csvimport_ImportWithEmptyId);
  registerTest("CsvImport::ImportWithWhitespaceId",
               test_csvimport_ImportWithWhitespaceId);
  registerTest("CsvImport::ImportMixedValidAndInvalid",
               test_csvimport_ImportMixedValidAndInvalid);
  registerTest("CsvImport::ImportEmptyFile", test_csvimport_ImportEmptyFile);
  registerTest("CsvImport::ImportQuotedFields",
               test_csvimport_ImportQuotedFields);
  registerTest("CsvImport::ImportMultilineFields",
               test_csvimport_ImportMultilineFields);
  registerTest("CsvImport::ImportUnclosedQuotes",
               test_csvimport_ImportUnclosedQuotes);
  registerTest("CsvImport::FailedRowsTracked", test_csvimport_FailedRowsTracked);
  registerTest("CsvImport::RetryCsvContainsFailedRows",
               test_csvimport_RetryCsvContainsFailedRows);
  registerTest("CsvImport::InvalidStatusReported",
               test_csvimport_InvalidStatusReported);
  registerTest("CsvImport::RetryCsvIncludesExtraFailures",
               test_csvimport_RetryCsvIncludesExtraFailures);
}
