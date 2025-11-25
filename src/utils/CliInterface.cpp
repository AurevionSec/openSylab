#include "utils/CliInterface.h"
#include "utils/CsvImport.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <vector>

namespace opensylab {
namespace utils {

CliInterface::CliInterface(std::shared_ptr<db::Database> database)
    : database_(database)
    , running_(false)
{
}

void CliInterface::run() {
    running_ = true;
    showWelcome();

    while (running_) {
        showMainMenu();
    }
}

void CliInterface::showWelcome() {
    clearScreen();
    printSeparator();
    std::cout << "\n";
    std::cout << "  ██████╗ ██████╗ ███████╗███╗   ██╗███████╗██╗   ██╗██╗      █████╗ ██████╗ \n";
    std::cout << " ██╔═══██╗██╔══██╗██╔════╝████╗  ██║██╔════╝╚██╗ ██╔╝██║     ██╔══██╗██╔══██╗\n";
    std::cout << " ██║   ██║██████╔╝█████╗  ██╔██╗ ██║███████╗ ╚████╔╝ ██║     ███████║██████╔╝\n";
    std::cout << " ██║   ██║██╔═══╝ ██╔══╝  ██║╚██╗██║╚════██║  ╚██╔╝  ██║     ██╔══██║██╔══██╗\n";
    std::cout << " ╚██████╔╝██║     ███████╗██║ ╚████║███████║   ██║   ███████╗██║  ██║██████╔╝\n";
    std::cout << "  ╚═════╝ ╚═╝     ╚══════╝╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═════╝ \n";
    std::cout << "\n";
    std::cout << "                     Labor Information Management System\n";
    std::cout << "                              Version 0.1.1\n";
    std::cout << "\n";
    printSeparator();
    std::cout << "\nWillkommen bei OpenSylab - Ihr Open Source LIMS\n";
    std::cout << "Entwickelt für kleine medizinische Diagnostiklabore\n";
    printSeparator();
    waitForEnter();
}

void CliInterface::showMainMenu() {
    clearScreen();
    printSeparator();
    std::cout << "                        HAUPTMENÜ\n";
    printSeparator();
    std::cout << "\n";
    std::cout << "  [1] Neue Probe erfassen\n";
    std::cout << "  [2] Alle Proben anzeigen\n";
    std::cout << "  [3] Probe suchen\n";
    std::cout << "  [4] Probe aktualisieren\n";
    std::cout << "  [5] Probe löschen\n";
    std::cout << "  [6] CSV-Import\n";
    std::cout << "  [7] Statistiken\n";
    std::cout << "  [0] Beenden\n";
    std::cout << "\n";
    printSeparator();

    int choice = readInteger("Ihre Wahl");
    if (!running_) return;  // EOF - Programm beenden

    switch (choice) {
        case 1: handleNewSample(); break;
        case 2: handleListSamples(); break;
        case 3: handleSearchSample(); break;
        case 4: handleUpdateSample(); break;
        case 5: handleDeleteSample(); break;
        case 6: handleImportCsv(); break;
        case 7: handleStatistics(); break;
        case 0: handleExit(); break;
        default:
            std::cout << "\nUngültige Auswahl. Bitte versuchen Sie es erneut.\n";
            waitForEnter();
    }
}

void CliInterface::handleNewSample() {
    clearScreen();
    printSeparator();
    std::cout << "               NEUE PROBE ERFASSEN\n";
    printSeparator();
    std::cout << "\n";

    // Pflichtfelder mit Validierung einlesen
    std::string sampleId = readValidatedInput("Proben-ID (Barcode)", "Proben-ID");
    if (!running_) return;  // EOF

    std::string patientId = readValidatedInput("Patienten-ID", "Patienten-ID");
    if (!running_) return;  // EOF

    // Optionale Felder
    std::string patientName = readInput("Patientenname (optional)");
    if (!running_) return;  // EOF
    patientName = trim(patientName);

    std::string description = readInput("Beschreibung (optional)");
    if (!running_) return;  // EOF
    description = trim(description);

    core::Sample sample(sampleId, patientId);
    sample.setPatientName(patientName);
    sample.setDescription(description);

    if (database_->createSample(sample)) {
        std::cout << "\n✓ Probe erfolgreich erfasst!\n";
    } else {
        std::cout << "\n✗ Fehler beim Erfassen der Probe: "
                 << database_->getLastError() << "\n";
    }

    waitForEnter();
}

void CliInterface::handleListSamples() {
    clearScreen();
    printSeparator();
    std::cout << "                ALLE PROBEN\n";
    printSeparator();
    std::cout << "\n";

    auto samples = database_->getAllSamples();

    // Fehlerprüfung ZUERST
    if (database_->hasError()) {
        std::cout << "✗ Fehler beim Abrufen der Proben:\n";
        std::cout << "  " << database_->getLastError() << "\n";
    } else if (samples.empty()) {
        std::cout << "ℹ Keine Proben in der Datenbank.\n";
    } else {
        std::cout << std::left
                 << std::setw(5) << "ID"
                 << std::setw(15) << "Proben-ID"
                 << std::setw(15) << "Patienten-ID"
                 << std::setw(25) << "Name"
                 << std::setw(15) << "Status" << "\n";
        printSeparator();

        for (const auto& sample : samples) {
            std::cout << std::left
                     << std::setw(5) << sample->getId()
                     << std::setw(15) << sample->getSampleId()
                     << std::setw(15) << sample->getPatientId()
                     << std::setw(25) << sample->getPatientName()
                     << std::setw(15) << sample->getStatusString() << "\n";
        }

        std::cout << "\nGesamt: " << samples.size() << " Proben\n";
    }

    waitForEnter();
}

void CliInterface::handleSearchSample() {
    clearScreen();
    printSeparator();
    std::cout << "              PROBE SUCHEN\n";
    printSeparator();
    std::cout << "\n";

    std::string sampleId = readInput("Proben-ID (Barcode)");
    if (!running_) return;  // EOF
    sampleId = trim(sampleId);

    // Validierung: Leere Eingabe abfangen
    if (isEmpty(sampleId)) {
        std::cout << "\n✗ Bitte geben Sie eine Proben-ID ein.\n";
        waitForEnter();
        return;
    }

    auto sample = database_->getSampleByBarcode(sampleId);

    if (sample) {
        printSeparator();
        std::cout << "\nProbe gefunden:\n\n";
        std::cout << "  ID:                " << sample->getId() << "\n";
        std::cout << "  Proben-ID:         " << sample->getSampleId() << "\n";
        std::cout << "  Patienten-ID:      " << sample->getPatientId() << "\n";
        std::cout << "  Patientenname:     " << sample->getPatientName() << "\n";
        std::cout << "  Beschreibung:      " << sample->getDescription() << "\n";
        std::cout << "  Status:            " << sample->getStatusString() << "\n";

        std::time_t regDate = sample->getRegistrationDate();
        std::cout << "  Registriert am:    " << std::ctime(&regDate);

        printSeparator();
    } else {
        std::cout << "\n✗ Probe nicht gefunden: " << database_->getLastError() << "\n";
    }

    waitForEnter();
}

void CliInterface::handleUpdateSample() {
    clearScreen();
    printSeparator();
    std::cout << "            PROBE AKTUALISIEREN\n";
    printSeparator();
    std::cout << "\n";

    int id = readInteger("Proben-ID (numerisch)");
    if (!running_) return;  // EOF

    auto sample = database_->getSample(id);

    if (!sample) {
        std::cout << "\n✗ Probe nicht gefunden: " << database_->getLastError() << "\n";
        waitForEnter();
        return;
    }

    std::cout << "\nAktuelle Werte:\n";
    std::cout << "  Status: " << sample->getStatusString() << "\n";
    std::cout << "  Beschreibung: " << sample->getDescription() << "\n\n";

    std::cout << "Status-Optionen:\n";
    std::cout << "  [1] Erfasst\n";
    std::cout << "  [2] In Analyse\n";
    std::cout << "  [3] Analysiert\n";
    std::cout << "  [4] Validiert\n";
    std::cout << "  [5] Archiviert\n\n";

    int statusChoice = readInteger("Neuer Status (1-5)");
    if (!running_) return;  // EOF

    core::Sample::Status newStatus;
    switch (statusChoice) {
        case 1: newStatus = core::Sample::Status::REGISTERED; break;
        case 2: newStatus = core::Sample::Status::IN_ANALYSIS; break;
        case 3: newStatus = core::Sample::Status::ANALYZED; break;
        case 4: newStatus = core::Sample::Status::VALIDATED; break;
        case 5: newStatus = core::Sample::Status::ARCHIVED; break;
        default:
            std::cout << "\nUngültige Auswahl.\n";
            waitForEnter();
            return;
    }

    sample->setStatus(newStatus);

    if (database_->updateSample(*sample)) {
        std::cout << "\n✓ Probe erfolgreich aktualisiert!\n";
    } else {
        std::cout << "\n✗ Fehler beim Aktualisieren: "
                 << database_->getLastError() << "\n";
    }

    waitForEnter();
}

void CliInterface::handleDeleteSample() {
    clearScreen();
    printSeparator();
    std::cout << "              PROBE LÖSCHEN\n";
    printSeparator();
    std::cout << "\n";

    int id = readInteger("Proben-ID (numerisch)");
    if (!running_) return;  // EOF

    auto sample = database_->getSample(id);

    if (!sample) {
        std::cout << "\n✗ Probe nicht gefunden: " << database_->getLastError() << "\n";
        waitForEnter();
        return;
    }

    std::cout << "\nProbe:\n";
    std::cout << "  Proben-ID: " << sample->getSampleId() << "\n";
    std::cout << "  Patient: " << sample->getPatientName() << "\n\n";

    std::string confirm = readInput("Wirklich löschen? (ja/nein)");
    if (!running_) return;  // EOF

    // Case-insensitive Prüfung
    std::string confirmLower = confirm;
    for (char& c : confirmLower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    if (confirmLower == "ja" || confirmLower == "j" || confirmLower == "yes" || confirmLower == "y") {
        if (database_->deleteSample(id)) {
            std::cout << "\n✓ Probe erfolgreich gelöscht!\n";
        } else {
            std::cout << "\n✗ Fehler beim Löschen: "
                     << database_->getLastError() << "\n";
        }
    } else {
        std::cout << "\nLöschen abgebrochen.\n";
    }

    waitForEnter();
}

void CliInterface::handleImportCsv() {
    clearScreen();
    printSeparator();
    std::cout << "                CSV-IMPORT\n";
    printSeparator();
    std::cout << "\n";

    std::string filePath = readInput("CSV-Datei Pfad");
    if (!running_) return;  // EOF

    CsvImport importer;
    auto samples = importer.importSamples(filePath);

    if (samples.empty()) {
        std::cout << "\n✗ Keine Proben importiert: "
                 << importer.getLastError() << "\n";
    } else {
        size_t imported = 0;
        size_t failed = 0;
        std::vector<std::string> failedSamples;

        for (const auto& sample : samples) {
            if (database_->createSample(sample)) {
                imported++;
            } else {
                failed++;
                failedSamples.push_back(sample.getSampleId() + ": " + database_->getLastError());
            }
        }

        std::cout << "\n✓ " << imported << " von " << samples.size()
                 << " Proben erfolgreich importiert!\n";

        // Fehlgeschlagene Importe anzeigen
        if (failed > 0) {
            std::cout << "\n✗ " << failed << " Proben konnten nicht importiert werden:\n";
            for (const auto& msg : failedSamples) {
                std::cout << "  - " << msg << "\n";
            }
        }
    }

    waitForEnter();
}

void CliInterface::handleStatistics() {
    clearScreen();
    printSeparator();
    std::cout << "               STATISTIKEN\n";
    printSeparator();
    std::cout << "\n";

    auto samples = database_->getAllSamples();

    // Fehlerprüfung
    if (database_->hasError()) {
        std::cout << "✗ Fehler beim Abrufen der Statistiken:\n";
        std::cout << "  " << database_->getLastError() << "\n";
        waitForEnter();
        return;
    }

    size_t total = samples.size();
    size_t registered = 0, inAnalysis = 0, analyzed = 0, validated = 0, archived = 0;

    for (const auto& sample : samples) {
        switch (sample->getStatus()) {
            case core::Sample::Status::REGISTERED: registered++; break;
            case core::Sample::Status::IN_ANALYSIS: inAnalysis++; break;
            case core::Sample::Status::ANALYZED: analyzed++; break;
            case core::Sample::Status::VALIDATED: validated++; break;
            case core::Sample::Status::ARCHIVED: archived++; break;
        }
    }

    std::cout << "Gesamtanzahl Proben:     " << total << "\n\n";
    std::cout << "Nach Status:\n";
    std::cout << "  Erfasst:               " << registered << "\n";
    std::cout << "  In Analyse:            " << inAnalysis << "\n";
    std::cout << "  Analysiert:            " << analyzed << "\n";
    std::cout << "  Validiert:             " << validated << "\n";
    std::cout << "  Archiviert:            " << archived << "\n";

    printSeparator();
    waitForEnter();
}

void CliInterface::handleExit() {
    clearScreen();
    printSeparator();
    std::cout << "\nVielen Dank für die Nutzung von OpenSylab!\n";
    std::cout << "Auf Wiedersehen.\n\n";
    printSeparator();
    running_ = false;
}

std::string CliInterface::readInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt << ": ";
    std::getline(std::cin, input);

    // EOF-Prüfung: Bei Ctrl+D/Ctrl+Z Programm beenden
    if (std::cin.eof()) {
        std::cout << "\n\nEingabe beendet (EOF). Programm wird beendet.\n";
        running_ = false;
    }

    return input;
}

std::string CliInterface::readValidatedInput(const std::string& prompt, const std::string& fieldName) {
    std::string input;

    while (true) {
        std::cout << prompt << ": ";
        std::getline(std::cin, input);

        // EOF-Prüfung: Bei Ctrl+D/Ctrl+Z Programm beenden
        if (std::cin.eof()) {
            std::cout << "\n\nEingabe beendet (EOF). Programm wird beendet.\n";
            running_ = false;
            return "";
        }

        // Whitespace trimmen
        input = trim(input);

        // Validierung
        if (isEmpty(input)) {
            std::cout << "✗ " << fieldName << " darf nicht leer sein!\n";
            continue;
        }

        if (!isValidId(input)) {
            std::cout << "✗ " << fieldName << " enthält ungültige Zeichen!\n";
            std::cout << "  Erlaubt: Buchstaben, Zahlen, Bindestrich, Unterstrich\n";
            continue;
        }

        break;
    }

    return input;
}

int CliInterface::readInteger(const std::string& prompt) {
    int value;
    std::cout << prompt << ": ";

    while (!(std::cin >> value)) {
        // EOF-Prüfung: Bei Ctrl+D/Ctrl+Z Programm beenden
        if (std::cin.eof()) {
            std::cout << "\n\nEingabe beendet (EOF). Programm wird beendet.\n";
            running_ = false;
            return -1;
        }

        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Ungültige Eingabe. Bitte geben Sie eine Zahl ein.\n";
        std::cout << prompt << ": ";
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

void CliInterface::clearScreen() {
    // ANSI Escape Codes - sicherer als system()
    // \033[2J = Clear entire screen
    // \033[H  = Move cursor to home position (0,0)
    std::cout << "\033[2J\033[H" << std::flush;
}

void CliInterface::waitForEnter() {
    std::cout << "\nDrücken Sie Enter zum Fortfahren...";
    std::cin.get();
}

void CliInterface::printSeparator() {
    std::cout << std::string(80, '=') << "\n";
}

// Validierungsfunktionen
std::string CliInterface::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";  // String besteht nur aus Whitespace
    }

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool CliInterface::isValidId(const std::string& id) {
    if (id.empty()) {
        return false;
    }

    // Erlaubt: Buchstaben, Zahlen, Bindestrich, Unterstrich
    for (char c : id) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') {
            return false;
        }
    }

    return true;
}

bool CliInterface::isEmpty(const std::string& str) {
    return str.empty() || str.find_first_not_of(" \t\r\n") == std::string::npos;
}

} // namespace utils
} // namespace opensylab
