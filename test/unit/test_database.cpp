/**
 * @file test_database.cpp
 * @brief Unit-Tests für die Database-Klasse
 */

#include "db/Database.h"
#include "test_macros.h"
#include <cstdio>
#include <cstdlib>
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

void registerDatabaseTests() {
  registerTest("Database::OpenAndClose", test_database_OpenAndClose);
  registerTest("Database::InitializeSchema", test_database_InitializeSchema);
  registerTest("Database::CreateSample", test_database_CreateSample);
  registerTest("Database::GetSampleByBarcode",
               test_database_GetSampleByBarcode);
  registerTest("Database::GetAllSamples", test_database_GetAllSamples);
  registerTest("Database::GetAllSamples_EmptyDatabase",
               test_database_GetAllSamples_EmptyDatabase);
  registerTest("Database::UpdateSample", test_database_UpdateSample);
  registerTest("Database::UpdateSample_NoChangesStillSuccess",
               test_database_UpdateSample_NoChangesStillSuccess);
  registerTest("Database::UpdateSample_NotFound",
               test_database_UpdateSample_NotFound);
  registerTest("Database::DeleteSample", test_database_DeleteSample);
  registerTest("Database::DeleteSample_NotFound",
               test_database_DeleteSample_NotFound);
}
