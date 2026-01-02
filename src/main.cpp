/**
 * @file main.cpp
 * @brief Haupteinstiegspunkt für OpenSylab LIMS v0.2
 *
 * OpenSylab - Open Source Laboratory Information Management System
 * Version 0.2.0
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
  std::cout << "OpenSylab v0.2.0 wird gestartet...\n" << std::endl;

  // Datenbankpfad ermitteln
  std::string dbPath;

  if (argc > 1) {
    // Wenn Kommandozeilenargument übergeben wurde, verwende es als DB-Pfad
    dbPath = argv[1];
  } else {
    // Sonst Konfiguration laden
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

  // Aufräumen
  database->close();
  std::cout << "\nDatenbank geschlossen. OpenSylab wurde beendet.\n"
            << std::endl;

  return 0;
}
