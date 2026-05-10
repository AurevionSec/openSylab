/**
 * @file test_csvresultimport.cpp
 * @brief Unit-Tests für die CsvResultImport-Klasse
 */

#include "db/Database.h"
#include "utils/CsvResultImport.h"
#include "test_macros.h"
#include <cstdio>
#include <fstream>
#include <memory>

using namespace opensylab::utils;
using namespace opensylab::db;

namespace {
// Hilfsfunktion zum Erstellen einer temporären CSV-Datei
std::string createTempCsv(const std::string &content) {
  static int counter = 0;
  std::string filename =
      "test_results_" + std::to_string(++counter) + "_" +
      std::to_string(static_cast<int>(std::time(nullptr))) + ".csv";

  std::ofstream file(filename);
  file << content;
  file.close();

  return filename;
}

void createLargeCsv(const std::string &path, size_t bytes) {
  std::ofstream file(path, std::ios::binary);
  std::string chunk(1024, 'a');
  size_t written = 0;
  while (written + chunk.size() <= bytes) {
    file.write(chunk.data(), static_cast<std::streamsize>(chunk.size()));
    written += chunk.size();
  }
  if (written < bytes) {
    const size_t remaining = bytes - written;
    file.write(chunk.data(), static_cast<std::streamsize>(remaining));
  }
  file.close();
}

// Hilfsfunktion zum Löschen einer temporären Datei
void deleteTempFile(const std::string &filename) { std::remove(filename.c_str()); }

// Erstellt eine Testdatenbank mit einem Order
std::shared_ptr<Database> createTestDatabase() {
  static int dbCounter = 0;
  std::string dbPath = "test_result_import_db_" + std::to_string(++dbCounter) +
                       "_" + std::to_string(std::time(nullptr)) + ".db";

  auto db = std::make_shared<Database>(dbPath);
  (void)db->open();
  (void)db->initializeSchema();

  // Sample erstellen
  opensylab::core::Sample sample("S001", "P001");
  (void)db->createSample(sample);

  // Order erstellen
  opensylab::core::Order order("O001", "S001", "Blutbild");
  (void)db->createOrder(order);

  return db;
}
} // namespace

bool test_csvresultimport_ImportValidCsv() {
  auto db = createTestDatabase();

  std::string csvContent =
      "result_id,order_id,test_parameter,value,unit,ref_low,ref_high,"
      "measured_by\n"
      "R001,1,Glucose,95,mg/dL,70,100,Laborant1\n"
      "R002,1,HbA1c,5.5,%,4.0,6.0,Laborant1\n";

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 2);
  ASSERT_EQ(importer.getImportedCount(), 2);
  ASSERT_EQ(importer.getErrorCount(), 0);

  // Erstes Ergebnis prüfen
  ASSERT_EQ(results[0].getResultId(), "R001");
  ASSERT_EQ(results[0].getOrderId(), 1);
  ASSERT_EQ(results[0].getTestParameter(), "Glucose");
  ASSERT_EQ(results[0].getValue(), "95");
  ASSERT_EQ(results[0].getUnit(), "mg/dL");
  ASSERT_EQ(results[0].getReferenceLow(), 70.0);
  ASSERT_EQ(results[0].getReferenceHigh(), 100.0);
  ASSERT_EQ(results[0].getMeasuredBy(), "Laborant1");
  ASSERT_EQ(results[0].getFlag(), opensylab::core::TestResult::Flag::NORMAL);

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_HeaderWithBomAccepted() {
  auto db = createTestDatabase();

  std::string csvContent =
      "\xEF\xBB\xBFresult_id,order_id,test_parameter,value,unit,ref_low,ref_high,measured_by\n"
      "R001,1,Glucose,95,mg/dL,70,100,Laborant1\n";

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 1);
  ASSERT_EQ(importer.getImportedCount(), 1);

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_HeaderWithExtraColumnsRejected() {
  auto db = createTestDatabase();

  std::string csvContent =
      "result_id,order_id,test_parameter,value,unit,ref_low,ref_high,measured_by,extra\n"
      "R001,1,Glucose,95,mg/dL,70,100,Laborant1,X\n";

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 0);
  ASSERT_FALSE(importer.getLastError().empty());

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_LargeFileRejected() {
  auto db = createTestDatabase();
  std::string filename =
      "test_results_large_" + std::to_string(std::time(nullptr)) + ".csv";
  createLargeCsv(filename, 10 * 1024 * 1024 + 1);

  CsvResultImport importer(db);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 0);
  ASSERT_FALSE(importer.getLastError().empty());

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_ImportWithMissingFields() {
  auto db = createTestDatabase();

  std::string csvContent = "result_id,order_id,test_parameter\n"
                           "R001,1\n"; // Zu wenig Felder

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 0);
  ASSERT_EQ(importer.getErrorCount(), 1);

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_ImportWithEmptyResultId() {
  auto db = createTestDatabase();

  std::string csvContent = "result_id,order_id,test_parameter,value\n"
                           ",1,Glucose,95\n"; // Leere result_id

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 0);
  ASSERT_EQ(importer.getErrorCount(), 1);

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_ImportWithInvalidOrderId() {
  auto db = createTestDatabase();

  std::string csvContent = "result_id,order_id,test_parameter,value\n"
                           "R001,abc,Glucose,95\n"; // order_id nicht numerisch

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 0);
  ASSERT_EQ(importer.getErrorCount(), 1);

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_ImportWithNonExistentOrder() {
  auto db = createTestDatabase();

  std::string csvContent = "result_id,order_id,test_parameter,value\n"
                           "R001,999,Glucose,95\n"; // Order existiert nicht

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  importer.setValidateOrders(true);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 0);
  ASSERT_EQ(importer.getErrorCount(), 1);

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_ImportWithDisabledValidation() {
  auto db = createTestDatabase();

  std::string csvContent = "result_id,order_id,test_parameter,value\n"
                           "R001,999,Glucose,95\n"; // Order existiert nicht

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  importer.setValidateOrders(false); // Validierung deaktiviert
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 1); // Sollte trotzdem importiert werden
  ASSERT_EQ(importer.getErrorCount(), 0);

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_FlagCalculation_Low() {
  auto db = createTestDatabase();

  std::string csvContent = "result_id,order_id,test_parameter,value,unit,ref_low,ref_high\n"
                           "R001,1,Glucose,60,mg/dL,70,100\n"; // Unter Referenz (LOW zone: 55-70)

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 1);
  ASSERT_EQ(results[0].getFlag(), opensylab::core::TestResult::Flag::LOW);

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_FlagCalculation_High() {
  auto db = createTestDatabase();

  std::string csvContent = "result_id,order_id,test_parameter,value,unit,ref_low,ref_high\n"
                           "R001,1,Glucose,110,mg/dL,70,100\n"; // Über Referenz (HIGH zone: 100-115)

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 1);
  ASSERT_EQ(results[0].getFlag(), opensylab::core::TestResult::Flag::HIGH);

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_ImportAndStore() {
  auto db = createTestDatabase();

  std::string csvContent = "result_id,order_id,test_parameter,value,unit\n"
                           "R001,1,Glucose,95,mg/dL\n";

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  int stored = importer.importAndStore(filename);

  ASSERT_EQ(stored, 1);

  // Prüfen ob in Datenbank
  auto result = db->getTestResultByResultId("R001");
  ASSERT_TRUE(result != nullptr);
  ASSERT_EQ(result->getTestParameter(), "Glucose");

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_ImportEmptyFile() {
  auto db = createTestDatabase();

  std::string csvContent = "result_id,order_id,test_parameter,value\n";
  // Nur Header, keine Daten

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 0);
  ASSERT_EQ(importer.getImportedCount(), 0);

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_InvalidHeaderRejected() {
  auto db = createTestDatabase();

  std::string csvContent = "result_id,order_id,wrong,value\n"
                           "R001,1,X,95\n";

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 0);
  ASSERT_EQ(importer.getErrorCount(), 1);
  ASSERT_FALSE(importer.getLastError().empty());

  deleteTempFile(filename);
  return true;
}

bool test_csvresultimport_WriteRetryCsvForFailedRows() {
  auto db = createTestDatabase();

  std::string csvContent = "result_id,order_id,test_parameter,value\n"
                           "R_BAD,abc,Glucose,95\n";

  std::string filename = createTempCsv(csvContent);

  CsvResultImport importer(db);
  auto results = importer.importResults(filename);

  ASSERT_EQ(results.size(), 0);
  ASSERT_EQ(importer.getErrorCount(), 1);

  std::string retryFile = "test_results_retry.csv";
  ASSERT_TRUE(importer.writeRetryCsv(retryFile));

  std::ifstream input(retryFile);
  ASSERT_TRUE(input.is_open());
  std::string header;
  std::getline(input, header);
  ASSERT_EQ(header, "result_id,order_id,test_parameter,value");

  std::string row;
  std::getline(input, row);
  ASSERT_EQ(row, "R_BAD,abc,Glucose,95");

  input.close();
  deleteTempFile(filename);
  deleteTempFile(retryFile);
  return true;
}

void registerCsvResultImportTests() {
  registerTest("CsvResultImport::ImportValidCsv",
               test_csvresultimport_ImportValidCsv);
  registerTest("CsvResultImport::HeaderWithBomAccepted",
               test_csvresultimport_HeaderWithBomAccepted);
  registerTest("CsvResultImport::HeaderWithExtraColumnsRejected",
               test_csvresultimport_HeaderWithExtraColumnsRejected);
  registerTest("CsvResultImport::LargeFileRejected",
               test_csvresultimport_LargeFileRejected);
  registerTest("CsvResultImport::ImportWithMissingFields",
               test_csvresultimport_ImportWithMissingFields);
  registerTest("CsvResultImport::ImportWithEmptyResultId",
               test_csvresultimport_ImportWithEmptyResultId);
  registerTest("CsvResultImport::ImportWithInvalidOrderId",
               test_csvresultimport_ImportWithInvalidOrderId);
  registerTest("CsvResultImport::ImportWithNonExistentOrder",
               test_csvresultimport_ImportWithNonExistentOrder);
  registerTest("CsvResultImport::ImportWithDisabledValidation",
               test_csvresultimport_ImportWithDisabledValidation);
  registerTest("CsvResultImport::FlagCalculation_Low",
               test_csvresultimport_FlagCalculation_Low);
  registerTest("CsvResultImport::FlagCalculation_High",
               test_csvresultimport_FlagCalculation_High);
  registerTest("CsvResultImport::ImportAndStore",
               test_csvresultimport_ImportAndStore);
  registerTest("CsvResultImport::ImportEmptyFile",
               test_csvresultimport_ImportEmptyFile);
  registerTest("CsvResultImport::InvalidHeaderRejected",
               test_csvresultimport_InvalidHeaderRejected);
  registerTest("CsvResultImport::WriteRetryCsvForFailedRows",
               test_csvresultimport_WriteRetryCsvForFailedRows);
}
