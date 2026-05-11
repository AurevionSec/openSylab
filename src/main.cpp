/**
 * @file main.cpp
 * @brief Haupteinstiegspunkt für OpenSylab LIMS
 *
 * OpenSylab - Open Source Laboratory Information Management System
 * Version: siehe include/version.h (SSOT: CMakeLists.txt)
 *
 * Dieses Programm bietet LIMS-Funktionalität für kleine
 * medizinische Diagnostiklabore:
 * - Probenverwaltung (CRUD-Operationen)
 * - Auftragsverwaltung
 * - Ergebniseingabe mit Plausibilitätsprüfung
 * - Gerätedatenschnittstelle (CSV/HL7)
 * - Audit-Trail
 * - Benutzerauthentifizierung
 * - SQLite-Datenbank
 * - Command-Line Interface
 */

#include "version.h"
#include "api/ApiServer.h"
#include "db/Database.h"
#include "utils/CliInterface.h"
#include <cstdlib>
#include <iostream>
#include <memory>

// Standardpfad für Datenbank (kann über Umgebungsvariable überschrieben werden)
const std::string DEFAULT_DB_PATH = "opensylab.db";

/**
 * @brief Lädt Konfiguration und gibt Datenbankpfad zurück
 */
std::string loadConfiguration() {
  // Prüfe Umgebungsvariable
  const char *dbPathEnv = std::getenv("OPENSYLAB_DB_PATH");
  if (dbPathEnv != nullptr) {
    return std::string(dbPathEnv);
  }

  // Verwende Standard
  return DEFAULT_DB_PATH;
}

/**
 * @brief Hauptfunktion
 */
int main(int argc, char *argv[]) {
  std::cout << OPENSYLAB_VERSION_STRING << " wird gestartet...\n" << std::endl;

  // Datenbankpfad ermitteln
  std::string dbPath;
  bool runApi = false;
  int apiPort = 8080;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--api") {
      runApi = true;
      continue;
    }
    if (arg == "--api-port" && i + 1 < argc) {
      try {
        apiPort = std::stoi(argv[++i]);
      } catch (...) {
        apiPort = 8080;
      }
      continue;
    }
    if (arg == "--db" && i + 1 < argc) {
      dbPath = argv[++i];
      continue;
    }
    if (dbPath.empty()) {
      dbPath = arg;
    }
  }

  if (dbPath.empty()) {
    dbPath = loadConfiguration();
  }

  std::cout << "Verwende Datenbank: " << dbPath << "\n" << std::endl;

  // Datenbank initialisieren
  auto database = std::make_shared<opensylab::db::Database>(dbPath);

  if (!database->open()) {
    std::cerr << "FEHLER: Kann Datenbank nicht öffnen!\n";
    std::cerr << "Details: " << database->getLastError() << std::endl;
    return 1;
  }

  // Datenbankschema initialisieren
  if (!database->initializeSchema()) {
    std::cerr << "FEHLER: Kann Datenbankschema nicht initialisieren!\n";
    std::cerr << "Details: " << database->getLastError() << std::endl;
    database->close();
    return 1;
  }

  std::cout << "Datenbank erfolgreich initialisiert.\n" << std::endl;

  if (runApi) {
    std::cout << "API-Server wird auf Port " << apiPort << " gestartet...\n"
              << std::endl;
    opensylab::api::ApiServer server(database, apiPort);
    if (!server.run()) {
      std::cerr << "FEHLER: API-Server konnte nicht gestartet werden.\n";
      database->close();
      return 1;
    }
  } else {
    // CLI-Interface starten
    try {
      auto cli = std::make_unique<opensylab::utils::CliInterface>(database);
      cli->run();
    } catch (const std::exception &e) {
      std::cerr << "FEHLER: Unerwarteter Fehler im CLI: " << e.what()
                << std::endl;
      database->close();
      return 1;
    }
  }

  // Aufräumen
  database->close();
  std::cout << "\nDatenbank geschlossen. OpenSylab wurde beendet.\n"
            << std::endl;

  return 0;
}
