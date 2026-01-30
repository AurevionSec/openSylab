#include "utils/CliInterface.h"
#include "utils/CsvImport.h"
#include "utils/CsvResultImport.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <thread>
#include <vector>
#ifdef __unix__
#include <sys/select.h>
#include <unistd.h>
#endif

namespace {
std::string formatDuration(std::time_t seconds) {
  if (seconds < 0) {
    seconds = 0;
  }
  const std::time_t days = seconds / 86400;
  const std::time_t hours = (seconds % 86400) / 3600;
  const std::time_t minutes = (seconds % 3600) / 60;
  const std::time_t secs = seconds % 60;

  std::ostringstream out;
  if (days > 0) {
    out << days << "d ";
  }
  out << std::setw(2) << std::setfill('0') << hours << ":"
      << std::setw(2) << std::setfill('0') << minutes << ":"
      << std::setw(2) << std::setfill('0') << secs;
  return out.str();
}

std::string toLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::string trimCopy(const std::string &value) {
  size_t start = 0;
  while (start < value.size() &&
         std::isspace(static_cast<unsigned char>(value[start]))) {
    ++start;
  }
  size_t end = value.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return value.substr(start, end - start);
}

bool waitForAutoRefresh(int seconds) {
#ifdef __unix__
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);
  timeval tv;
  tv.tv_sec = seconds;
  tv.tv_usec = 0;
  int rc = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
  if (rc > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
    std::string input;
    std::getline(std::cin, input);
    const std::string trimmed = toLowerCopy(trimCopy(input));
    return !(trimmed == "q" || trimmed == "quit");
  }
  return true;
#else
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
  return true;
#endif
}

std::string normalizeStatusInput(const std::string &input,
                                 const std::vector<std::string> &options) {
  const std::string cleaned = toLowerCopy(trimCopy(input));
  if (cleaned.empty()) {
    return "";
  }
  for (const auto &option : options) {
    if (toLowerCopy(option) == cleaned) {
      return option;
    }
  }
  return "";
}
} // namespace

namespace opensylab {
namespace utils {

CliInterface::CliInterface(std::shared_ptr<db::Database> database)
    : database_(database), running_(false), currentUser_(nullptr),
      startTime_(std::time(nullptr)) {}

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
  std::cout << "  ██████╗ ██████╗ ███████╗███╗   ██╗███████╗██╗   ██╗██╗      "
               "█████╗ ██████╗ \n";
  std::cout << " ██╔═══██╗██╔══██╗██╔════╝████╗  ██║██╔════╝╚██╗ ██╔╝██║     "
               "██╔══██╗██╔══██╗\n";
  std::cout << " ██║   ██║██████╔╝█████╗  ██╔██╗ ██║███████╗ ╚████╔╝ ██║     "
               "███████║██████╔╝\n";
  std::cout << " ██║   ██║██╔═══╝ ██╔══╝  ██║╚██╗██║╚════██║  ╚██╔╝  ██║     "
               "██╔══██║██╔══██╗\n";
  std::cout << " ╚██████╔╝██║     ███████╗██║ ╚████║███████║   ██║   "
               "███████╗██║  ██║██████╔╝\n";
  std::cout << "  ╚═════╝ ╚═╝     ╚══════╝╚═╝  ╚═══╝╚══════╝   ╚═╝   "
               "╚══════╝╚═╝  ╚═╝╚═════╝ \n";
  std::cout << "\n";
  std::cout << "                     Labor Information Management System\n";
  std::cout << "                              Version 0.2.0\n";
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
  std::cout << "  Angemeldet als: " << getCurrentUsername();
  if (isLoggedIn()) {
    std::cout << " (" << currentUser_->getRoleString() << ")";
  }
  std::cout << "\n";
  printSeparator();
  std::cout << "\n";
  std::cout << "  === Probenverwaltung ===\n";
  std::cout << "  [1] Neue Probe erfassen\n";
  std::cout << "  [2] Alle Proben anzeigen\n";
  std::cout << "  [3] Probe suchen\n";
  std::cout << "  [4] Probe aktualisieren\n";
  std::cout << "  [5] Probe löschen\n";
  std::cout << "  [6] CSV-Import\n";
  std::cout << "  [8] CSV-Export\n";
  std::cout << "\n  === Auftragsverwaltung ===\n";
  std::cout << "  [10] Neuer Auftrag\n";
  std::cout << "  [11] Alle Aufträge anzeigen\n";
  std::cout << "  [12] Auftrag suchen\n";
  std::cout << "  [13] Auftrag aktualisieren\n";
  std::cout << "  [14] Auftrag löschen\n";
  std::cout << "  [15] Aufträge zu Probe anzeigen\n";
  std::cout << "\n  === Ergebnisverwaltung ===\n";
  std::cout << "  [20] Neues Ergebnis erfassen\n";
  std::cout << "  [21] Alle Ergebnisse anzeigen\n";
  std::cout << "  [22] Ergebnis suchen\n";
  std::cout << "  [23] Ergebnis aktualisieren\n";
  std::cout << "  [24] Ergebnis validieren\n";
  std::cout << "  [25] Ergebnisse zu Auftrag anzeigen\n";
  std::cout << "  [26] Ergebnisse aus CSV importieren\n";
  std::cout << "  [27] Ergebnisse exportieren\n";
  std::cout << "\n  === Audit-Trail ===\n";
  std::cout << "  [30] Audit-Log anzeigen\n";
  std::cout << "  [31] Audit für Entität anzeigen\n";
  if (isAdmin()) {
    std::cout << "  [32] Retention konfigurieren\n";
    std::cout << "  [33] Retention jetzt ausführen\n";
    std::cout << "  [34] Audit-Log exportieren\n";
  }
  std::cout << "\n  === Benutzerverwaltung ===\n";
  if (!isLoggedIn()) {
    std::cout << "  [40] Anmelden\n";
  } else {
    std::cout << "  [41] Abmelden\n";
    std::cout << "  [42] Passwort ändern\n";
    if (isAdmin()) {
      std::cout << "  [43] Benutzer anlegen\n";
      std::cout << "  [44] Benutzer anzeigen\n";
      std::cout << "  [45] Benutzer bearbeiten\n";
      std::cout << "  [46] Benutzer deaktivieren\n";
      std::cout << "  [47] Rollen verwalten\n";
    }
  }
  std::cout << "\n  === System ===\n";
  std::cout << "  [7] Statistiken\n";
  if (isAdmin()) {
    std::cout << "  [9] Systemstatus\n";
  }
  if (canAccessDiagnostics()) {
    std::cout << "  [35] Diagnose/Logs\n";
  }
  std::cout << "  [0] Beenden\n";
  std::cout << "\n";
  printSeparator();

  int choice = readInteger("Ihre Wahl");
  if (!running_)
    return; // EOF - Programm beenden

  switch (choice) {
  // Probenverwaltung
  case 1:
    handleNewSample();
    break;
  case 2:
    handleListSamples();
    break;
  case 3:
    handleSearchSample();
    break;
  case 4:
    handleUpdateSample();
    break;
  case 5:
    handleDeleteSample();
    break;
  case 6:
    handleImportCsv();
    break;
  case 8:
    handleExportSamples();
    break;
  // Auftragsverwaltung
  case 10:
    handleNewOrder();
    break;
  case 11:
    handleListOrders();
    break;
  case 12:
    handleSearchOrder();
    break;
  case 13:
    handleUpdateOrder();
    break;
  case 14:
    handleDeleteOrder();
    break;
  case 15:
    handleOrdersForSample();
    break;
  // Ergebnisverwaltung
  case 20:
    handleNewResult();
    break;
  case 21:
    handleListResults();
    break;
  case 22:
    handleSearchResult();
    break;
  case 23:
    handleUpdateResult();
    break;
  case 24:
    handleValidateResult();
    break;
  case 25:
    handleResultsForOrder();
    break;
  case 26:
    handleImportResultsCsv();
    break;
  case 27:
    handleExportResults();
    break;
  // Audit-Trail
  case 30:
    handleShowAuditLog();
    break;
  case 31:
    handleAuditForEntity();
    break;
  case 32:
    handleConfigureRetention();
    break;
  case 33:
    handleRunRetention();
    break;
  case 34:
    handleExportAuditLog();
    break;
  // Benutzerverwaltung
  case 40:
    handleLogin();
    break;
  case 41:
    handleLogout();
    break;
  case 42:
    handleChangePassword();
    break;
  case 43:
    handleCreateUser();
    break;
  case 44:
    handleListUsers();
    break;
  case 45:
    handleUpdateUser();
    break;
  case 46:
    handleDeleteUser();
    break;
  case 47:
    handleManageRoles();
    break;
  // System
  case 7:
    handleStatistics();
    break;
  case 9:
    handleSystemStatus();
    break;
  case 35:
    handleDiagnosticsLogs();
    break;
  case 0:
    handleExit();
    break;
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
  if (!running_)
    return; // EOF

  std::string patientId = readValidatedInput("Patienten-ID", "Patienten-ID");
  if (!running_)
    return; // EOF

  // Optionale Felder
  std::string patientName = readInput("Patientenname (optional)");
  if (!running_)
    return; // EOF
  patientName = trim(patientName);

  std::string description = readInput("Beschreibung (optional)");
  if (!running_)
    return; // EOF
  description = trim(description);

  core::Sample sample(sampleId, patientId);
  sample.setPatientName(patientName);
  sample.setDescription(description);

  if (database_->createSample(sample, getCurrentUsername())) {
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

  db::Database::SampleFilter filter;
  bool useFilter = false;

  std::string useFilterInput = readInput("Filter anwenden? (j/n)");
  if (!running_)
    return;
  useFilterInput = trim(useFilterInput);
  if (!useFilterInput.empty() &&
      (useFilterInput == "j" || useFilterInput == "ja" ||
       useFilterInput == "y" || useFilterInput == "yes")) {
    useFilter = true;
  }

  if (useFilter) {
    std::string query =
        readInput("Suchbegriff (Proben-/Patienten-ID oder Name, optional)");
    if (!running_)
      return;
    query = trim(query);
    filter.query = query;

    while (true) {
      std::cout << "\nStatus-Filter:\n";
      std::cout << "  [0] Alle\n";
      std::cout << "  [1] Erfasst\n";
      std::cout << "  [2] In Analyse\n";
      std::cout << "  [3] Analysiert\n";
      std::cout << "  [4] Validiert\n";
      std::cout << "  [5] Archiviert\n";
      std::string statusChoice = readInput("Status-Auswahl (0-5, optional)");
      if (!running_)
        return;
      statusChoice = trim(statusChoice);
      if (statusChoice.empty() || statusChoice == "0") {
        std::string excludeInput =
            readInput("Archivierte ausblenden? (j/n)");
        if (!running_)
          return;
        excludeInput = trim(excludeInput);
        filter.excludeArchived =
            (!excludeInput.empty() &&
             (excludeInput == "j" || excludeInput == "ja" ||
              excludeInput == "y" || excludeInput == "yes"));
        break;
      }
      if (statusChoice == "1") {
        filter.status = "Erfasst";
      } else if (statusChoice == "2") {
        filter.status = "In Analyse";
      } else if (statusChoice == "3") {
        filter.status = "Analysiert";
      } else if (statusChoice == "4") {
        filter.status = "Validiert";
      } else if (statusChoice == "5") {
        filter.status = "Archiviert";
      } else {
        std::cout << "\n✗ Ungültige Status-Auswahl.\n";
        continue;
      }
      break;
    }

    while (true) {
      std::string fromDateInput =
          readInput("Von-Datum (YYYY-MM-DD, optional)");
      if (!running_)
        return;
      fromDateInput = trim(fromDateInput);
      if (!fromDateInput.empty()) {
        std::time_t fromDate;
        if (!parseDate(fromDateInput, fromDate)) {
          std::cout << "\n✗ Ungültiges Von-Datum.\n";
          continue;
        }
        filter.fromDate = fromDate;
      }

      std::string toDateInput = readInput("Bis-Datum (YYYY-MM-DD, optional)");
      if (!running_)
        return;
      toDateInput = trim(toDateInput);
      if (!toDateInput.empty()) {
        std::time_t toDate;
        if (!parseDate(toDateInput, toDate)) {
          std::cout << "\n✗ Ungültiges Bis-Datum.\n";
          continue;
        }
        filter.toDate = toDate + (24 * 60 * 60 - 1);
      }

      if (filter.fromDate.has_value() && filter.toDate.has_value() &&
          filter.fromDate.value() > filter.toDate.value()) {
        std::cout << "\n✗ Von-Datum darf nicht nach dem Bis-Datum liegen.\n";
        filter.toDate.reset();
        continue;
      }
      break;
    }
  }

  auto printSamples = [&](const std::string &title,
                          const std::vector<std::unique_ptr<core::Sample>>
                              &samplesToPrint) {
    std::cout << "\n" << title << "\n";
    printSeparator();
    if (database_->hasError()) {
      std::cout << "✗ Fehler beim Abrufen der Proben:\n";
      std::cout << "  " << database_->getLastError() << "\n";
      return;
    }
    if (samplesToPrint.empty()) {
      std::cout << "ℹ Keine Proben in der Datenbank.\n";
      return;
    }

    std::cout << std::left << std::setw(5) << "ID" << std::setw(15)
              << "Proben-ID" << std::setw(15) << "Patienten-ID" << std::setw(25)
              << "Name" << std::setw(15) << "Status" << "\n";
    printSeparator();

    for (const auto &sample : samplesToPrint) {
      std::cout << std::left << std::setw(5) << sample->getId() << std::setw(15)
                << sample->getSampleId() << std::setw(15)
                << sample->getPatientId() << std::setw(25)
                << sample->getPatientName() << std::setw(15)
                << sample->getStatusString() << "\n";
    }

    std::cout << "\nGesamt: " << samplesToPrint.size() << " Proben\n";
  };

  if (!useFilter) {
    filter.excludeArchived = true;
  }

  bool hasCriteria = !filter.query.empty() || !filter.status.empty() ||
                     filter.fromDate.has_value() || filter.toDate.has_value();
  auto samples = database_->getSamplesByFilter(filter);

  printSamples(hasCriteria ? "Suchergebnisse" : "Alle Proben", samples);

  db::Database::SampleFilter activeFilter = filter;
  std::string activeTitle = hasCriteria ? "Suchergebnisse" : "Alle Proben";

  if (useFilter && hasCriteria) {
    std::string resetInput =
        readInput("\nFilter zuruecksetzen und alle Proben anzeigen? (j/n)");
    if (!running_)
      return;
    resetInput = trim(resetInput);
    if (!resetInput.empty() &&
        (resetInput == "j" || resetInput == "ja" || resetInput == "y" ||
         resetInput == "yes")) {
      database_->clearError();
      db::Database::SampleFilter resetFilter;
      resetFilter.excludeArchived = true;
      auto allSamples = database_->getSamplesByFilter(resetFilter);
      printSamples("Alle Proben", allSamples);
      activeFilter = resetFilter;
      activeTitle = "Alle Proben";
    }
  }

  std::string autoRefreshInput =
      readInput("\nAuto-Refresh alle 5s aktivieren? (j/n)");
  if (!running_)
    return;
  autoRefreshInput = trim(autoRefreshInput);
  if (!autoRefreshInput.empty() &&
      (autoRefreshInput == "j" || autoRefreshInput == "ja" ||
       autoRefreshInput == "y" || autoRefreshInput == "yes")) {
    const int refreshSeconds = autoRefreshIntervalSeconds();
    std::cout << "\nAuto-Refresh aktiv. 'q' + Enter beendet.\n";
    while (waitForAutoRefresh(refreshSeconds)) {
      clearScreen();
      printSeparator();
      std::cout << "                ALLE PROBEN\n";
      printSeparator();
      std::cout << "\n";
      auto refreshed = database_->getSamplesByFilter(activeFilter);
      printSamples(activeTitle, refreshed);
      std::cout << "\nAuto-Refresh aktiv. 'q' + Enter beendet.\n";
    }
    return;
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
  if (!running_)
    return; // EOF
  sampleId = trim(sampleId);

  // Validierung: Leere Eingabe abfangen
  if (isEmpty(sampleId)) {
    std::cout << "\n✗ Bitte geben Sie eine Proben-ID ein.\n";
    waitForEnter();
    return;
  }

  auto sample = database_->getSampleByBarcode(sampleId);

  if (sample) {
    auto printSampleDetail =
        [&](const core::Sample &detail, bool supportView) {
          printSeparator();
          std::cout << "\nProbe gefunden:\n\n";
          std::cout << "  ID:                " << detail.getId() << "\n";
          std::cout << "  Proben-ID:         " << detail.getSampleId() << "\n";
          std::cout << "  Patienten-ID:      " << detail.getPatientId() << "\n";
          if (!supportView) {
            std::cout << "  Patientenname:     " << detail.getPatientName() << "\n";
            std::cout << "  Beschreibung:      " << detail.getDescription() << "\n";
          }
          std::cout << "  Status:            " << detail.getStatusString() << "\n";

          std::time_t regDate = detail.getRegistrationDate();
          std::cout << "  Registriert am:    " << std::ctime(&regDate);
          printSeparator();
        };

    const bool supportView = canAccessSupportData() && !isAdmin();
    if (supportView) {
      const std::string actor =
          currentUser_ ? currentUser_->getUsername() : std::string("system");
      if (!database_->logSupportAccess(core::AuditEntry::EntityType::SAMPLE,
                                       sample->getSampleId(), actor)) {
        std::cout << "\n⚠ Zugriff konnte nicht protokolliert werden: "
                  << database_->getLastError() << "\n";
      }
    }
    printSampleDetail(*sample, supportView);

    std::string autoRefreshInput =
        readInput("\nAuto-Refresh alle 5s aktivieren? (j/n)");
    if (!running_)
      return;
    autoRefreshInput = trim(autoRefreshInput);
    if (!autoRefreshInput.empty() &&
        (autoRefreshInput == "j" || autoRefreshInput == "ja" ||
         autoRefreshInput == "y" || autoRefreshInput == "yes")) {
      const int refreshSeconds = autoRefreshIntervalSeconds();
      std::cout << "\nAuto-Refresh aktiv. 'q' + Enter beendet.\n";
      while (waitForAutoRefresh(refreshSeconds)) {
        clearScreen();
        printSeparator();
        std::cout << "              PROBE SUCHEN\n";
        printSeparator();
        std::cout << "\n";
        auto refreshed = database_->getSampleByBarcode(sampleId);
        if (!refreshed) {
          std::cout << "\n✗ Probe nicht gefunden: "
                    << database_->getLastError() << "\n";
          break;
        }
        printSampleDetail(*refreshed, supportView);
        std::cout << "\nAuto-Refresh aktiv. 'q' + Enter beendet.\n";
      }
      return;
    }
  } else {
    std::cout << "\n✗ Probe nicht gefunden: " << database_->getLastError()
              << "\n";
  }

  waitForEnter();
}

void CliInterface::handleUpdateSample() {
  clearScreen();
  printSeparator();
  std::cout << "            PROBE AKTUALISIEREN\n";
  printSeparator();
  std::cout << "\n";

  if (!canEdit()) {
    std::cout << "✗ Keine Berechtigung. Bitte anmelden.\n";
    waitForEnter();
    return;
  }

  int id = readInteger("Proben-ID (numerisch)");
  if (!running_)
    return; // EOF

  auto sample = database_->getSample(id);

  if (!sample) {
    std::cout << "\n✗ Probe nicht gefunden: " << database_->getLastError()
              << "\n";
    waitForEnter();
    return;
  }

  std::cout << "\nAktuelle Werte:\n";
  core::Sample::Status oldStatusEnum = sample->getStatus();
  std::string oldStatus = sample->getStatusString();
  std::cout << "  Status: " << oldStatus << "\n";
  std::cout << "  Beschreibung: " << sample->getDescription() << "\n\n";

  const std::vector<core::Sample::Status> statusOptions = {
      core::Sample::Status::REGISTERED, core::Sample::Status::IN_ANALYSIS,
      core::Sample::Status::ANALYZED, core::Sample::Status::VALIDATED,
      core::Sample::Status::ARCHIVED};

  std::cout << "Status-Optionen:\n";
  for (size_t i = 0; i < statusOptions.size(); ++i) {
    std::cout << "  [" << (i + 1) << "] "
              << core::Sample::statusToString(statusOptions[i]) << "\n";
  }
  std::cout << "\n";

  int statusChoice =
      readInteger("Neuer Status (1-" + std::to_string(statusOptions.size()) +
                  ")");
  if (!running_)
    return; // EOF

  if (statusChoice < 1 ||
      static_cast<size_t>(statusChoice) > statusOptions.size()) {
    std::cout << "\nUngültige Auswahl.\n";
    waitForEnter();
    return;
  }
  core::Sample::Status newStatus = statusOptions[statusChoice - 1];

  if (newStatus == oldStatusEnum) {
    std::cout << "\nℹ Status bleibt unverändert.\n";
    waitForEnter();
    return;
  }

  sample->setStatus(newStatus);

  if (database_->updateSample(*sample, getCurrentUsername())) {
    std::cout << "\n✓ Probe erfolgreich aktualisiert!\n";
  } else {
    std::cout << "\n✗ Fehler beim Aktualisieren: " << database_->getLastError()
              << "\n";
  }

  waitForEnter();
}

void CliInterface::handleDeleteSample() {
  clearScreen();
  printSeparator();
  std::cout << "              PROBE LÖSCHEN\n";
  printSeparator();
  std::cout << "\n";

  if (!canEdit()) {
    std::cout << "✗ Keine Berechtigung. Bitte anmelden.\n";
    waitForEnter();
    return;
  }

  int id = readInteger("Proben-ID (numerisch)");
  if (!running_)
    return; // EOF

  auto sample = database_->getSample(id);

  if (!sample) {
    std::cout << "\n✗ Probe nicht gefunden: " << database_->getLastError()
              << "\n";
    waitForEnter();
    return;
  }

  std::cout << "\nProbe:\n";
  std::cout << "  Proben-ID: " << sample->getSampleId() << "\n";
  std::cout << "  Patient: " << sample->getPatientName() << "\n\n";

  std::cout << "Aktion wählen:\n";
  std::cout << "  [1] Archivieren\n";
  std::cout << "  [2] Löschen\n";
  std::cout << "  [0] Abbrechen\n\n";

  int actionChoice = readInteger("Ihre Wahl");
  if (!running_)
    return; // EOF

  if (actionChoice == 0) {
    std::cout << "\nAbgebrochen.\n";
    waitForEnter();
    return;
  }

  std::string confirmPrompt =
      actionChoice == 1 ? "Wirklich archivieren? (ja/nein)"
                        : "Wirklich löschen? (ja/nein)";
  std::string confirm = readInput(confirmPrompt);
  if (!running_)
    return; // EOF

  std::string confirmLower = confirm;
  for (char &c : confirmLower) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }

  if (confirmLower != "ja" && confirmLower != "j" && confirmLower != "yes" &&
      confirmLower != "y") {
    std::cout << "\nAktion abgebrochen.\n";
    waitForEnter();
    return;
  }

  if (actionChoice == 1) {
    sample->setStatus(core::Sample::Status::ARCHIVED);
    if (database_->updateSample(*sample, getCurrentUsername())) {
      std::cout << "\n✓ Probe erfolgreich archiviert!\n";
    } else {
      std::cout << "\n✗ Fehler beim Archivieren: " << database_->getLastError()
                << "\n";
    }
  } else if (actionChoice == 2) {
    if (database_->deleteSample(id, getCurrentUsername())) {
      std::cout << "\n✓ Probe erfolgreich gelöscht!\n";
    } else {
      std::cout << "\n✗ Fehler beim Löschen: " << database_->getLastError()
                << "\n";
    }
  } else {
    std::cout << "\nUngültige Auswahl.\n";
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
  if (!running_)
    return; // EOF

  auto buildRetryPath = [](const std::string &path) {
    std::string retryPath = path;
    const std::string suffix = ".csv";
    size_t pos = retryPath.rfind(suffix);
    if (pos != std::string::npos && pos == retryPath.size() - suffix.size()) {
      retryPath.insert(pos, "_retry");
    } else {
      retryPath += "_retry.csv";
    }
    return retryPath;
  };

  std::string currentPath = filePath;
  bool retryImport = false;

  do {
    CsvImport importer;
    auto samples = importer.importSamples(currentPath);
    const auto &importedRecords = importer.getImportedRecords();
    std::vector<CsvImport::FailedRecord> dbFailedRecords;

    if (samples.empty()) {
      std::cout << "\n✗ Keine Proben importiert: " << importer.getLastError()
                << "\n";
    } else {
      size_t imported = 0;
      size_t failed = 0;
      std::vector<std::string> failedSamples;

      for (const auto &record : importedRecords) {
        if (database_->createSample(record.sample, getCurrentUsername())) {
          imported++;
        } else {
          failed++;
          const std::string error = database_->getLastError();
          failedSamples.push_back("Zeile " + std::to_string(record.recordNumber) +
                                  " (" + record.sample.getSampleId() + "): " +
                                  error);
          dbFailedRecords.push_back(
              {record.recordNumber, record.record, error});
        }
      }

      std::cout << "\n✓ " << imported << " von " << importedRecords.size()
                << " Proben erfolgreich importiert!\n";

      // Fehlgeschlagene Importe anzeigen
      if (failed > 0) {
        std::cout << "\n✗ " << failed
                  << " Proben konnten nicht importiert werden:\n";
        for (const auto &msg : failedSamples) {
          std::cout << "  - " << msg << "\n";
        }
      }
    }

    if (importer.getFailedCount() > 0 || !dbFailedRecords.empty()) {
      if (importer.getFailedCount() > 0) {
        std::cout << "\n✗ CSV-Fehler (" << importer.getFailedCount()
                  << " Zeilen):\n";
        for (const auto &failed : importer.getFailedRecords()) {
          std::cout << "  - Zeile " << failed.recordNumber << ": "
                    << failed.error << " (\"" << failed.record << "\")\n";
        }
      }

      if (!dbFailedRecords.empty()) {
        std::cout << "\n✗ DB-Fehler (" << dbFailedRecords.size()
                  << " Zeilen):\n";
        for (const auto &failed : dbFailedRecords) {
          std::cout << "  - Zeile " << failed.recordNumber << ": "
                    << failed.error << " (\"" << failed.record << "\")\n";
        }
      }

      std::string retryPath = buildRetryPath(currentPath);
      if (importer.writeRetryCsv(retryPath, dbFailedRecords)) {
        std::cout << "\n✓ Retry-Datei erstellt: " << retryPath << "\n";
        std::cout << "  Tipp: Datei korrigieren und erneut importieren.\n";

        std::string choice = readInput("Retry-Datei jetzt importieren? (y/n)");
        if (!running_)
          return;

        if (!choice.empty() &&
            (choice[0] == 'y' || choice[0] == 'Y')) {
          currentPath = retryPath;
          retryImport = true;
          continue;
        }
      } else {
        std::cout << "\n✗ Konnte Retry-Datei nicht schreiben.\n";
      }
    }

    retryImport = false;
  } while (retryImport);

  waitForEnter();
}

void CliInterface::handleExportSamples() {
  clearScreen();
  printSeparator();
  std::cout << "                CSV-EXPORT (PROBEN)\n";
  printSeparator();
  std::cout << "\n";

  std::string filePath = readInput("Export-Dateipfad");
  if (!running_)
    return;
  filePath = trim(filePath);
  if (isEmpty(filePath)) {
    std::cout << "\n✗ Bitte geben Sie einen Dateipfad an.\n";
    waitForEnter();
    return;
  }

  if (database_->exportSamplesToCsv(filePath)) {
    std::cout << "\n✓ Export erfolgreich!\n";
  } else {
    std::cout << "\n✗ Fehler beim Export:\n";
    std::cout << "  " << database_->getLastError() << "\n";
  }

  waitForEnter();
}

void CliInterface::handleStatistics() {
  clearScreen();
  printSeparator();
  std::cout << "               STATISTIKEN\n";
  printSeparator();
  std::cout << "\n";

  bool applyFilters = false;
  db::Database::StatsFilter sampleFilter;
  db::Database::StatsFilter orderFilter;
  db::Database::StatsFilter resultFilter;

  std::string filterChoice = readInput("Filter anwenden? (y/n)");
  if (!running_)
    return;
  filterChoice = trim(filterChoice);
  if (!filterChoice.empty() &&
      (filterChoice[0] == 'y' || filterChoice[0] == 'Y')) {
    applyFilters = true;

    std::string fromInput = readInput("Von-Datum (YYYY-MM-DD, optional)");
    if (!running_)
      return;
    fromInput = trim(fromInput);
    if (!fromInput.empty()) {
      std::time_t fromDate;
      if (!parseDate(fromInput, fromDate)) {
        std::cout << "\n✗ Ungültiges Von-Datum.\n";
        waitForEnter();
        return;
      }
      sampleFilter.fromDate = fromDate;
      orderFilter.fromDate = fromDate;
      resultFilter.fromDate = fromDate;
    }

    std::string toInput = readInput("Bis-Datum (YYYY-MM-DD, optional)");
    if (!running_)
      return;
    toInput = trim(toInput);
    if (!toInput.empty()) {
      std::time_t toDate;
      if (!parseDate(toInput, toDate)) {
        std::cout << "\n✗ Ungültiges Bis-Datum.\n";
        waitForEnter();
        return;
      }
      toDate += (24 * 60 * 60 - 1);
      sampleFilter.toDate = toDate;
      orderFilter.toDate = toDate;
      resultFilter.toDate = toDate;
    }

    if (sampleFilter.fromDate.has_value() &&
        sampleFilter.toDate.has_value() &&
        sampleFilter.fromDate.value() > sampleFilter.toDate.value()) {
      std::cout << "\n✗ Von-Datum darf nicht nach dem Bis-Datum liegen.\n";
      waitForEnter();
      return;
    }

    auto readStatusFilter =
        [&](const std::string &title,
            const std::vector<std::string> &options,
            std::optional<std::string> &out) {
          std::cout << "\n" << title << "\n";
          for (size_t i = 0; i < options.size(); ++i) {
            std::cout << "  " << (i + 1) << ") " << options[i] << "\n";
          }
          std::string choice = readInput("Status-Auswahl (0=alle, optional)");
          if (!running_)
            return false;
          choice = trim(choice);
          if (choice.empty() || choice == "0") {
            out.reset();
            return true;
          }
          try {
            int index = std::stoi(choice);
            if (index < 1 || static_cast<size_t>(index) > options.size()) {
              throw std::out_of_range("status");
            }
            out = options[index - 1];
            return true;
          } catch (...) {
            std::cout << "\n✗ Ungültige Status-Auswahl.\n";
            waitForEnter();
            return false;
          }
        };

    std::optional<std::string> sampleStatus;
    std::optional<std::string> orderStatus;
    std::optional<std::string> resultStatus;

    const std::vector<std::string> sampleOptions = {
        core::Sample::statusToString(core::Sample::Status::REGISTERED),
        core::Sample::statusToString(core::Sample::Status::IN_ANALYSIS),
        core::Sample::statusToString(core::Sample::Status::ANALYZED),
        core::Sample::statusToString(core::Sample::Status::VALIDATED),
        core::Sample::statusToString(core::Sample::Status::ARCHIVED)};

    if (!readStatusFilter("Proben-Status:", sampleOptions, sampleStatus)) {
      return;
    }

    const std::vector<std::string> orderOptions = {
        core::Order::statusToString(core::Order::Status::REQUESTED),
        core::Order::statusToString(core::Order::Status::IN_PROGRESS),
        core::Order::statusToString(core::Order::Status::COMPLETED),
        core::Order::statusToString(core::Order::Status::VALIDATED),
        core::Order::statusToString(core::Order::Status::CANCELLED)};

    if (!readStatusFilter("Auftrags-Status:", orderOptions, orderStatus)) {
      return;
    }

    const std::vector<std::string> resultOptions = {
        core::TestResult::statusToString(core::TestResult::Status::PENDING),
        core::TestResult::statusToString(core::TestResult::Status::ENTERED),
        core::TestResult::statusToString(core::TestResult::Status::VALIDATED),
        core::TestResult::statusToString(core::TestResult::Status::REJECTED),
        core::TestResult::statusToString(core::TestResult::Status::REPEATED)};

    if (!readStatusFilter("Ergebnis-Status:", resultOptions, resultStatus)) {
      return;
    }

    if (sampleStatus.has_value()) {
      sampleFilter.status = sampleStatus;
    }
    if (orderStatus.has_value()) {
      orderFilter.status = orderStatus;
    }
    if (resultStatus.has_value()) {
      resultFilter.status = resultStatus;
    }
  }

  auto countFor = [](const std::vector<db::Database::StatusCount> &entries,
                     const std::string &status) {
    for (const auto &entry : entries) {
      if (entry.status == status) {
        return entry.count;
      }
    }
    return 0;
  };

  while (true) {
    const auto sampleStats = database_->getSampleStats(sampleFilter);
    if (database_->hasError()) {
      std::cout << "✗ Fehler beim Abrufen der Proben-Statistiken:\n";
      std::cout << "  " << database_->getLastError() << "\n";
      waitForEnter();
      return;
    }

    const auto orderStats = database_->getOrderStats(orderFilter);
    if (database_->hasError()) {
      std::cout << "✗ Fehler beim Abrufen der Auftrags-Statistiken:\n";
      std::cout << "  " << database_->getLastError() << "\n";
      waitForEnter();
      return;
    }

    const auto resultStats = database_->getResultStats(resultFilter);
    if (database_->hasError()) {
      std::cout << "✗ Fehler beim Abrufen der Ergebnis-Statistiken:\n";
      std::cout << "  " << database_->getLastError() << "\n";
      waitForEnter();
      return;
    }

    std::cout << "PROBEN\n";
    std::cout << "Gesamtanzahl:            " << sampleStats.total << "\n";
    std::cout << "Nach Status:\n";
    std::cout << "  Erfasst:               "
              << countFor(sampleStats.byStatus,
                          core::Sample::statusToString(
                              core::Sample::Status::REGISTERED))
              << "\n";
    std::cout << "  In Analyse:            "
              << countFor(sampleStats.byStatus,
                          core::Sample::statusToString(
                              core::Sample::Status::IN_ANALYSIS))
              << "\n";
    std::cout << "  Analysiert:            "
              << countFor(sampleStats.byStatus,
                          core::Sample::statusToString(
                              core::Sample::Status::ANALYZED))
              << "\n";
    std::cout << "  Validiert:             "
              << countFor(sampleStats.byStatus,
                          core::Sample::statusToString(
                              core::Sample::Status::VALIDATED))
              << "\n";
    std::cout << "  Archiviert:            "
              << countFor(sampleStats.byStatus,
                          core::Sample::statusToString(
                              core::Sample::Status::ARCHIVED))
              << "\n\n";

    std::cout << "AUFTRÄGE\n";
    std::cout << "Gesamtanzahl:            " << orderStats.total << "\n";
    std::cout << "Nach Status:\n";
    std::cout << "  Angefordert:           "
              << countFor(orderStats.byStatus,
                          core::Order::statusToString(
                              core::Order::Status::REQUESTED))
              << "\n";
    std::cout << "  In Bearbeitung:        "
              << countFor(orderStats.byStatus,
                          core::Order::statusToString(
                              core::Order::Status::IN_PROGRESS))
              << "\n";
    std::cout << "  Abgeschlossen:         "
              << countFor(orderStats.byStatus,
                          core::Order::statusToString(
                              core::Order::Status::COMPLETED))
              << "\n";
    std::cout << "  Validiert:             "
              << countFor(orderStats.byStatus,
                          core::Order::statusToString(
                              core::Order::Status::VALIDATED))
              << "\n";
    std::cout << "  Storniert:             "
              << countFor(orderStats.byStatus,
                          core::Order::statusToString(
                              core::Order::Status::CANCELLED))
              << "\n\n";

    std::cout << "ERGEBNISSE\n";
    std::cout << "Gesamtanzahl:            " << resultStats.total << "\n";
    std::cout << "Nach Status:\n";
    std::cout << "  Ausstehend:            "
              << countFor(resultStats.byStatus,
                          core::TestResult::statusToString(
                              core::TestResult::Status::PENDING))
              << "\n";
    std::cout << "  Eingegeben:            "
              << countFor(resultStats.byStatus,
                          core::TestResult::statusToString(
                              core::TestResult::Status::ENTERED))
              << "\n";
    std::cout << "  Validiert:             "
              << countFor(resultStats.byStatus,
                          core::TestResult::statusToString(
                              core::TestResult::Status::VALIDATED))
              << "\n";
    std::cout << "  Abgelehnt:             "
              << countFor(resultStats.byStatus,
                          core::TestResult::statusToString(
                              core::TestResult::Status::REJECTED))
              << "\n";
    std::cout << "  Wiederholung nötig:    "
              << countFor(resultStats.byStatus,
                          core::TestResult::statusToString(
                              core::TestResult::Status::REPEATED))
              << "\n";

    printSeparator();
    if (!applyFilters) {
      waitForEnter();
      return;
    }

    std::string exportChoice = readInput("Report exportieren? (y/n)");
    if (!running_)
      return;
    exportChoice = trim(exportChoice);
    if (!exportChoice.empty() &&
        (exportChoice[0] == 'y' || exportChoice[0] == 'Y')) {
      std::string filePath = readInput("Export-Dateipfad");
      if (!running_)
        return;
      filePath = trim(filePath);
      if (isEmpty(filePath)) {
        std::cout << "\n✗ Bitte geben Sie einen Dateipfad an.\n";
      } else if (database_->exportStatsReportToCsv(
                     filePath, sampleFilter, orderFilter, resultFilter,
                     getCurrentUsername())) {
        std::cout << "\n✓ Export erfolgreich!\n";
      } else {
        std::cout << "\n✗ Fehler beim Export:\n";
        std::cout << "  " << database_->getLastError() << "\n";
      }
    }

    std::string resetChoice =
        readInput("Filter zurücksetzen und alle anzeigen? (y/n)");
    if (!running_)
      return;
    resetChoice = trim(resetChoice);
    if (!resetChoice.empty() &&
        (resetChoice[0] == 'y' || resetChoice[0] == 'Y')) {
      applyFilters = false;
      sampleFilter = db::Database::StatsFilter{};
      orderFilter = db::Database::StatsFilter{};
      resultFilter = db::Database::StatsFilter{};
      continue;
    }
    waitForEnter();
    return;
  }
}

void CliInterface::handleExit() {
  clearScreen();
  printSeparator();
  std::cout << "\nVielen Dank für die Nutzung von OpenSylab!\n";
  std::cout << "Auf Wiedersehen.\n\n";
  printSeparator();
  running_ = false;
}

std::string CliInterface::readInput(const std::string &prompt) {
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

std::string CliInterface::readValidatedInput(const std::string &prompt,
                                             const std::string &fieldName) {
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

int CliInterface::readInteger(const std::string &prompt) {
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

// ============================================================================
// Order-Handler
// ============================================================================

void CliInterface::handleNewOrder() {
  clearScreen();
  printSeparator();
  std::cout << "              NEUER AUFTRAG\n";
  printSeparator();
  std::cout << "\n";

  if (!canEdit()) {
    std::cout << "✗ Keine Berechtigung. Bitte anmelden.\n";
    waitForEnter();
    return;
  }

  // Auftrags-ID
  std::string orderId = readValidatedInput("Auftrags-ID", "Auftrags-ID");
  if (!running_)
    return;

  // Proben-ID (muss existieren)
  std::string sampleId = readValidatedInput("Proben-ID (Barcode)", "Proben-ID");
  if (!running_)
    return;

  // Prüfen ob Probe existiert
  auto sample = database_->getSampleByBarcode(sampleId);
  if (!sample) {
    std::cout << "\n✗ Probe nicht gefunden. Bitte zuerst Probe erfassen.\n";
    waitForEnter();
    return;
  }

  // Testtyp
  std::string testType;
  while (true) {
    testType = readInput("Testtyp (z.B. Blutbild)");
    if (!running_)
      return;
    testType = trim(testType);
    if (isEmpty(testType)) {
      std::cout << "✗ Testtyp darf nicht leer sein!\n";
      continue;
    }
    break;
  }

  // Gewünschtes Datum (YYYY-MM-DD)
  std::time_t requestedDate = 0;
  while (true) {
    std::string dateInput = readInput("Gewünschtes Datum (YYYY-MM-DD)");
    if (!running_)
      return;
    dateInput = trim(dateInput);
    if (isEmpty(dateInput)) {
      std::cout << "✗ Datum darf nicht leer sein!\n";
      continue;
    }
    if (!parseDate(dateInput, requestedDate)) {
      std::cout << "✗ Ungültiges Datum. Format: YYYY-MM-DD\n";
      continue;
    }
    break;
  }

  // Priorität
  std::cout << "\nPriorität:\n";
  std::cout << "  [1] Normal\n";
  std::cout << "  [2] Dringend\n";
  std::cout << "  [3] Notfall\n\n";

  int priorityChoice = readInteger("Priorität (1-3)");
  if (!running_)
    return;

  core::Order::Priority priority;
  switch (priorityChoice) {
  case 1:
    priority = core::Order::Priority::NORMAL;
    break;
  case 2:
    priority = core::Order::Priority::URGENT;
    break;
  case 3:
    priority = core::Order::Priority::EMERGENCY;
    break;
  default:
    std::cout << "\nUngültige Auswahl. Verwende 'Normal'.\n";
    priority = core::Order::Priority::NORMAL;
  }

  // Notizen (optional)
  std::string notes = readInput("Notizen (optional)");
  if (!running_)
    return;
  notes = trim(notes);

  core::Order order(orderId, sampleId, testType);
  order.setRequestedDate(requestedDate);
  order.setPriority(priority);
  order.setNotes(notes);

  if (database_->createOrder(order, getCurrentUsername())) {
    std::cout << "\n✓ Auftrag erfolgreich erstellt!\n";
  } else {
    std::cout << "\n✗ Fehler beim Erstellen: " << database_->getLastError()
              << "\n";
  }

  waitForEnter();
}

void CliInterface::handleSystemStatus() {
  clearScreen();
  printSeparator();
  std::cout << "              SYSTEMSTATUS\n";
  printSeparator();
  std::cout << "\n";

  if (!isAdmin()) {
    std::cout << "✗ Nur Administratoren dürfen den Systemstatus anzeigen.\n";
    waitForEnter();
    return;
  }

  auto status = database_->getHealthStatus();

  std::cout << "Uptime:           "
            << formatDuration(std::time(nullptr) - startTime_) << "\n";
  std::cout << "DB-Verbindung:   "
            << (status.dbOpen ? "OK" : "FEHLER") << "\n";
  std::cout << "Schema-Status:   "
            << (status.schemaOk ? "OK" : "FEHLER") << "\n";

  if (!status.schemaOk && !status.missingTables.empty()) {
    std::cout << "\nFehlende Tabellen:\n";
    for (const auto &table : status.missingTables) {
      std::cout << "  - " << table << "\n";
    }
  }

  if (!database_->getLastError().empty()) {
    std::cout << "\nLetzter Fehler: " << database_->getLastError() << "\n";
  }

  waitForEnter();
}

void CliInterface::handleDiagnosticsLogs() {
  clearScreen();
  printSeparator();
  std::cout << "           DIAGNOSE & LOGS\n";
  printSeparator();
  std::cout << "\n";

  if (!canAccessDiagnostics()) {
    std::cout << "✗ Keine Berechtigung für Diagnose/Logs.\n";
    waitForEnter();
    return;
  }

  db::Database::DiagnosticsFilter filter;

  std::string fromInput = readInput("Von-Datum (YYYY-MM-DD, optional)");
  if (!running_)
    return;
  fromInput = trim(fromInput);
  if (!fromInput.empty()) {
    std::time_t fromDate;
    if (!parseDate(fromInput, fromDate)) {
      std::cout << "\n✗ Ungültiges Von-Datum.\n";
      waitForEnter();
      return;
    }
    filter.fromTime = fromDate;
  }

  std::string toInput = readInput("Bis-Datum (YYYY-MM-DD, optional)");
  if (!running_)
    return;
  toInput = trim(toInput);
  if (!toInput.empty()) {
    std::time_t toDate;
    if (!parseDate(toInput, toDate)) {
      std::cout << "\n✗ Ungültiges Bis-Datum.\n";
      waitForEnter();
      return;
    }
    toDate += (24 * 60 * 60 - 1);
    filter.toTime = toDate;
  }

  if (filter.fromTime.has_value() && filter.toTime.has_value() &&
      filter.fromTime.value() > filter.toTime.value()) {
    std::cout << "\n✗ Von-Datum darf nicht nach dem Bis-Datum liegen.\n";
    waitForEnter();
    return;
  }

  std::cout << "\nKomponente filtern:\n";
  std::cout << "  [0] Alle\n";
  std::cout << "  [1] Probe\n";
  std::cout << "  [2] Auftrag\n";
  std::cout << "  [3] Ergebnis\n";
  std::cout << "  [4] Benutzer\n";
  std::cout << "  [5] Rolle\n";
  std::cout << "  [6] System\n\n";

  int componentChoice = readInteger("Komponente (0-6)");
  if (!running_)
    return;
  switch (componentChoice) {
  case 0:
    break;
  case 1:
    filter.component = core::AuditEntry::EntityType::SAMPLE;
    break;
  case 2:
    filter.component = core::AuditEntry::EntityType::ORDER;
    break;
  case 3:
    filter.component = core::AuditEntry::EntityType::RESULT;
    break;
  case 4:
    filter.component = core::AuditEntry::EntityType::USER;
    break;
  case 5:
    filter.component = core::AuditEntry::EntityType::ROLE;
    break;
  case 6:
    filter.component = core::AuditEntry::EntityType::SYSTEM;
    break;
  default:
    std::cout << "\n✗ Ungültige Auswahl.\n";
    waitForEnter();
    return;
  }

  std::string limitStr = readInput("Anzahl der Einträge (200)");
  if (!running_)
    return;
  limitStr = trim(limitStr);
  if (!limitStr.empty()) {
    try {
      int limit = std::stoi(limitStr);
      if (limit > 0) {
        filter.limit = limit;
      }
    } catch (...) {
      // ignore invalid input
    }
  }

  auto entries = database_->getDiagnosticsLogs(filter);
  if (database_->hasError()) {
    std::cout << "✗ Fehler beim Abrufen der Diagnose-Logs:\n";
    std::cout << "  " << database_->getLastError() << "\n";
    waitForEnter();
    return;
  }

  if (entries.empty()) {
    std::cout << "ℹ Keine Diagnose-Logs vorhanden.\n";
  } else {
    std::cout << std::left << std::setw(5) << "ID" << std::setw(20)
              << "Zeitstempel" << std::setw(14) << "Aktion" << std::setw(12)
              << "Komponente" << std::setw(12) << "Entität-ID" << std::setw(15)
              << "Benutzer" << "\n";
    printSeparator();

    for (const auto &entry : entries) {
      std::cout << std::left << std::setw(5) << entry->getId()
                << std::setw(20) << entry->getTimestampString()
                << std::setw(14) << entry->getActionString() << std::setw(12)
                << entry->getEntityString() << std::setw(12)
                << entry->getEntityId() << std::setw(15) << entry->getUser()
                << "\n";
      if (!entry->getDetails().empty()) {
        std::cout << "      Details: " << entry->getDetails() << "\n";
      }
    }
    std::cout << "\nAngezeigt: " << entries.size() << " Einträge\n";
  }

  std::string exportChoice = readInput("Logs exportieren? (j/n)");
  if (!running_)
    return;
  exportChoice = trim(exportChoice);
  if (!exportChoice.empty() &&
      (exportChoice == "j" || exportChoice == "ja" || exportChoice == "y" ||
       exportChoice == "yes")) {
    std::string filePath = readInput("Export-Dateipfad");
    if (!running_)
      return;
    filePath = trim(filePath);
    if (isEmpty(filePath)) {
      std::cout << "\n✗ Bitte geben Sie einen Dateipfad an.\n";
      waitForEnter();
      return;
    }

    int exported = 0;
    const std::string actor =
        currentUser_ ? currentUser_->getUsername() : std::string("system");
    if (database_->exportDiagnosticsLogsToCsv(filePath, filter, actor,
                                              exported)) {
      std::cout << "\n✓ Export erfolgreich! (" << exported << " Einträge)\n";
    } else {
      std::cout << "\n✗ Fehler beim Export:\n";
      std::cout << "  " << database_->getLastError() << "\n";
    }
  }

  waitForEnter();
}

bool CliInterface::canAccessDiagnostics() {
  if (!currentUser_) {
    return false;
  }
  if (isAdmin()) {
    return true;
  }
  const std::string roleName = currentUser_->getRoleName();
  if (roleName.empty()) {
    return false;
  }
  if (toLowerCopy(roleName) == "support") {
    return true;
  }
  const auto permissions = database_->getRolePermissions(roleName);
  for (const auto &perm : permissions) {
    const std::string lower = toLowerCopy(perm);
    if (lower == "support" || lower == "diagnostics") {
      return true;
    }
  }
  return false;
}

bool CliInterface::canAccessSupportData() {
  if (!currentUser_) {
    return false;
  }
  if (isAdmin()) {
    return true;
  }
  const std::string roleName = currentUser_->getRoleName();
  if (roleName.empty()) {
    return false;
  }
  if (toLowerCopy(roleName) == "support") {
    return true;
  }
  const auto permissions = database_->getRolePermissions(roleName);
  for (const auto &perm : permissions) {
    const std::string lower = toLowerCopy(perm);
    if (lower == "support" || lower == "support_data" ||
        lower == "support-data") {
      return true;
    }
  }
  return false;
}

void CliInterface::handleListOrders() {
  clearScreen();
  printSeparator();
  std::cout << "              ALLE AUFTRÄGE\n";
  printSeparator();
  std::cout << "\n";

  std::cout << "Filter (optional):\n";
  std::cout << "  Status:    (z.B. Angefordert, In Bearbeitung, Abgeschlossen,\n";
  std::cout << "              Validiert, Storniert)\n";
  std::cout << "  Proben-ID: (z.B. S001)\n";
  std::cout << "  Priorität: (Normal, Dringend, Notfall)\n";
  std::cout << "  [Enter] leer lassen = kein Filter\n\n";

  std::string statusFilter = trim(readInput("Status-Filter"));
  if (!running_)
    return;
  std::string sampleFilter = trim(readInput("Proben-ID-Filter"));
  if (!running_)
    return;
  std::string priorityFilter = trim(readInput("Priorität-Filter"));
  if (!running_)
    return;

  if (!statusFilter.empty()) {
    statusFilter = normalizeStatusInput(statusFilter,
                                        {"Angefordert", "In Bearbeitung",
                                         "Abgeschlossen", "Validiert",
                                         "Storniert"});
    if (statusFilter.empty()) {
      std::cout << "\n✗ Ungültiger Status-Filter.\n";
      waitForEnter();
      return;
    }
  }

  if (!priorityFilter.empty()) {
    priorityFilter = normalizeStatusInput(
        priorityFilter, {"Normal", "Dringend", "Notfall"});
    if (priorityFilter.empty()) {
      std::cout << "\n✗ Ungültiger Prioritäts-Filter.\n";
      waitForEnter();
      return;
    }
  }

  db::Database::OrderFilter filter;
  filter.status = statusFilter;
  filter.sampleId = sampleFilter;
  filter.priority = priorityFilter;

  auto orders = (statusFilter.empty() && sampleFilter.empty() &&
                 priorityFilter.empty())
                    ? database_->getAllOrders()
                    : database_->getOrdersByFilter(filter);
  const bool hasFilters =
      !(statusFilter.empty() && sampleFilter.empty() && priorityFilter.empty());
  db::Database::OrderFilter activeFilter = filter;
  bool activeHasFilters = hasFilters;

  if (database_->hasError()) {
    std::cout << "✗ Fehler beim Abrufen der Aufträge:\n";
    std::cout << "  " << database_->getLastError() << "\n";
  } else if (orders.empty() &&
             !(statusFilter.empty() && sampleFilter.empty() &&
               priorityFilter.empty())) {
    std::cout << "ℹ Keine passenden Aufträge für die Filter.\n";
    std::string reset = readInput("Filter zurücksetzen? (y/n)");
    if (!running_)
      return;
    if (!reset.empty() && (reset[0] == 'y' || reset[0] == 'Y')) {
      orders = database_->getAllOrders();
      activeHasFilters = false;
    }
  } else if (orders.empty()) {
    std::cout << "ℹ Keine Aufträge in der Datenbank.\n";
  } else {
    std::cout << std::left << std::setw(5) << "ID" << std::setw(12)
              << "Auftrags-ID" << std::setw(12) << "Proben-ID" << std::setw(15)
              << "Testtyp" << std::setw(14) << "Status" << std::setw(10)
              << "Priorität" << "\n";
    printSeparator();

    for (const auto &order : orders) {
      std::cout << std::left << std::setw(5) << order->getId() << std::setw(12)
                << order->getOrderId() << std::setw(12) << order->getSampleId()
                << std::setw(15) << order->getTestType() << std::setw(14)
                << order->getStatusString() << std::setw(10)
                << order->getPriorityString() << "\n";
    }

    std::cout << "\nGesamt: " << orders.size() << " Aufträge\n";
  }

  if (activeHasFilters) {
    std::string reset =
        readInput("\nFilter zurücksetzen und alle Aufträge anzeigen? (j/n)");
    if (!running_)
      return;
    reset = trim(reset);
    if (!reset.empty() && (reset == "j" || reset == "ja" || reset == "y" ||
                           reset == "yes")) {
      orders = database_->getAllOrders();
      activeHasFilters = false;
      activeFilter = db::Database::OrderFilter{};
      if (database_->hasError()) {
        std::cout << "✗ Fehler beim Abrufen der Aufträge:\n";
        std::cout << "  " << database_->getLastError() << "\n";
      } else if (orders.empty()) {
        std::cout << "ℹ Keine Aufträge in der Datenbank.\n";
      } else {
        std::cout << std::left << std::setw(5) << "ID" << std::setw(12)
                  << "Auftrags-ID" << std::setw(12) << "Proben-ID"
                  << std::setw(15) << "Testtyp" << std::setw(14) << "Status"
                  << std::setw(10) << "Priorität" << "\n";
        printSeparator();

        for (const auto &order : orders) {
          std::cout << std::left << std::setw(5) << order->getId()
                    << std::setw(12) << order->getOrderId() << std::setw(12)
                    << order->getSampleId() << std::setw(15)
                    << order->getTestType() << std::setw(14)
                    << order->getStatusString() << std::setw(10)
                    << order->getPriorityString() << "\n";
        }

        std::cout << "\nGesamt: " << orders.size() << " Aufträge\n";
      }
    }
  }

  std::string autoRefreshInput =
      readInput("\nAuto-Refresh alle 5s aktivieren? (j/n)");
  if (!running_)
    return;
  autoRefreshInput = trim(autoRefreshInput);
  if (!autoRefreshInput.empty() &&
      (autoRefreshInput == "j" || autoRefreshInput == "ja" ||
       autoRefreshInput == "y" || autoRefreshInput == "yes")) {
    const int refreshSeconds = autoRefreshIntervalSeconds();
    std::cout << "\nAuto-Refresh aktiv. 'q' + Enter beendet.\n";
    while (waitForAutoRefresh(refreshSeconds)) {
      clearScreen();
      printSeparator();
      std::cout << "              ALLE AUFTRÄGE\n";
      printSeparator();
      std::cout << "\n";
      auto refreshed = activeHasFilters ? database_->getOrdersByFilter(activeFilter)
                                        : database_->getAllOrders();
      if (database_->hasError()) {
        std::cout << "✗ Fehler beim Abrufen der Aufträge:\n";
        std::cout << "  " << database_->getLastError() << "\n";
      } else if (refreshed.empty()) {
        std::cout << "ℹ Keine Aufträge in der Datenbank.\n";
      } else {
        std::cout << std::left << std::setw(5) << "ID" << std::setw(12)
                  << "Auftrags-ID" << std::setw(12) << "Proben-ID"
                  << std::setw(15) << "Testtyp" << std::setw(14) << "Status"
                  << std::setw(10) << "Priorität" << "\n";
        printSeparator();

        for (const auto &order : refreshed) {
          std::cout << std::left << std::setw(5) << order->getId()
                    << std::setw(12) << order->getOrderId() << std::setw(12)
                    << order->getSampleId() << std::setw(15)
                    << order->getTestType() << std::setw(14)
                    << order->getStatusString() << std::setw(10)
                    << order->getPriorityString() << "\n";
        }

        std::cout << "\nGesamt: " << refreshed.size() << " Aufträge\n";
      }
      std::cout << "\nAuto-Refresh aktiv. 'q' + Enter beendet.\n";
    }
    return;
  }

  waitForEnter();
}

void CliInterface::handleSearchOrder() {
  clearScreen();
  printSeparator();
  std::cout << "            AUFTRAG SUCHEN\n";
  printSeparator();
  std::cout << "\n";

  std::string orderId = readInput("Auftrags-ID");
  if (!running_)
    return;
  orderId = trim(orderId);

  if (isEmpty(orderId)) {
    std::cout << "\n✗ Bitte geben Sie eine Auftrags-ID ein.\n";
    waitForEnter();
    return;
  }

  auto order = database_->getOrderByOrderId(orderId);

  if (order) {
    auto printOrderDetail =
        [&](const core::Order &detail, bool supportView) {
          printSeparator();
          std::cout << "\nAuftrag gefunden:\n\n";
          std::cout << "  ID:              " << detail.getId() << "\n";
          std::cout << "  Auftrags-ID:     " << detail.getOrderId() << "\n";
          std::cout << "  Proben-ID:       " << detail.getSampleId() << "\n";
          std::cout << "  Testtyp:         " << detail.getTestType() << "\n";
          std::cout << "  Status:          " << detail.getStatusString() << "\n";
          std::cout << "  Priorität:       " << detail.getPriorityString() << "\n";
          if (!supportView) {
            std::cout << "  Notizen:         " << detail.getNotes() << "\n";
          }

          std::time_t reqDate = detail.getRequestedDate();
          std::cout << "  Angefordert am:  " << std::ctime(&reqDate);

          if (detail.getCompletedDate() > 0) {
            std::time_t compDate = detail.getCompletedDate();
            std::cout << "  Abgeschlossen:   " << std::ctime(&compDate);
          }

          printSeparator();
        };

    const bool supportView = canAccessSupportData() && !isAdmin();
    if (supportView) {
      const std::string actor =
          currentUser_ ? currentUser_->getUsername() : std::string("system");
      if (!database_->logSupportAccess(core::AuditEntry::EntityType::ORDER,
                                       order->getOrderId(), actor)) {
        std::cout << "\n⚠ Zugriff konnte nicht protokolliert werden: "
                  << database_->getLastError() << "\n";
      }
    }
    printOrderDetail(*order, supportView);

    std::string autoRefreshInput =
        readInput("\nAuto-Refresh alle 5s aktivieren? (j/n)");
    if (!running_)
      return;
    autoRefreshInput = trim(autoRefreshInput);
    if (!autoRefreshInput.empty() &&
        (autoRefreshInput == "j" || autoRefreshInput == "ja" ||
         autoRefreshInput == "y" || autoRefreshInput == "yes")) {
      const int refreshSeconds = autoRefreshIntervalSeconds();
      std::cout << "\nAuto-Refresh aktiv. 'q' + Enter beendet.\n";
      while (waitForAutoRefresh(refreshSeconds)) {
        clearScreen();
        printSeparator();
        std::cout << "            AUFTRAG SUCHEN\n";
        printSeparator();
        std::cout << "\n";
        auto refreshed = database_->getOrderByOrderId(orderId);
        if (!refreshed) {
          std::cout << "\n✗ Auftrag nicht gefunden: "
                    << database_->getLastError() << "\n";
          break;
        }
        printOrderDetail(*refreshed, supportView);
        std::cout << "\nAuto-Refresh aktiv. 'q' + Enter beendet.\n";
      }
      return;
    }
  } else {
    std::cout << "\n✗ Auftrag nicht gefunden: " << database_->getLastError()
              << "\n";
  }

  waitForEnter();
}

void CliInterface::handleUpdateOrder() {
  clearScreen();
  printSeparator();
  std::cout << "          AUFTRAG AKTUALISIEREN\n";
  printSeparator();
  std::cout << "\n";

  if (!canEdit()) {
    std::cout << "✗ Keine Berechtigung. Bitte anmelden.\n";
    waitForEnter();
    return;
  }

  int id = readInteger("Auftrags-ID (numerisch)");
  if (!running_)
    return;

  auto order = database_->getOrder(id);

  if (!order) {
    std::cout << "\n✗ Auftrag nicht gefunden: " << database_->getLastError()
              << "\n";
    waitForEnter();
    return;
  }

  std::cout << "\nAktuelle Werte:\n";
  std::cout << "  Testtyp:   " << order->getTestType() << "\n";
  std::cout << "  Status:    " << order->getStatusString() << "\n";
  std::cout << "  Priorität: " << order->getPriorityString() << "\n\n";

  const std::vector<core::Order::Status> statusOptions = {
      core::Order::Status::REQUESTED, core::Order::Status::IN_PROGRESS,
      core::Order::Status::COMPLETED, core::Order::Status::VALIDATED,
      core::Order::Status::CANCELLED};

  std::cout << "Status-Optionen:\n";
  for (size_t i = 0; i < statusOptions.size(); ++i) {
    std::cout << "  [" << (i + 1) << "] "
              << core::Order::statusToString(statusOptions[i]) << "\n";
  }
  std::cout << "\n";

  int statusChoice =
      readInteger("Neuer Status (1-" + std::to_string(statusOptions.size()) +
                  ")");
  if (!running_)
    return;

  if (statusChoice < 1 ||
      static_cast<size_t>(statusChoice) > statusOptions.size()) {
    std::cout << "\nUngültige Auswahl.\n";
    waitForEnter();
    return;
  }
  core::Order::Status newStatus = statusOptions[statusChoice - 1];
  if (newStatus == core::Order::Status::COMPLETED) {
    order->setCompletedDate(std::time(nullptr));
  }

  order->setStatus(newStatus);

  if (database_->updateOrder(*order, getCurrentUsername())) {
    std::cout << "\n✓ Auftrag erfolgreich aktualisiert!\n";
  } else {
    std::cout << "\n✗ Fehler beim Aktualisieren: " << database_->getLastError()
              << "\n";
  }

  waitForEnter();
}

void CliInterface::handleDeleteOrder() {
  clearScreen();
  printSeparator();
  std::cout << "            AUFTRAG LÖSCHEN\n";
  printSeparator();
  std::cout << "\n";

  if (!canEdit()) {
    std::cout << "✗ Keine Berechtigung. Bitte anmelden.\n";
    waitForEnter();
    return;
  }

  int id = readInteger("Auftrags-ID (numerisch)");
  if (!running_)
    return;

  auto order = database_->getOrder(id);

  if (!order) {
    std::cout << "\n✗ Auftrag nicht gefunden: " << database_->getLastError()
              << "\n";
    waitForEnter();
    return;
  }

  std::cout << "\nAuftrag:\n";
  std::cout << "  Auftrags-ID: " << order->getOrderId() << "\n";
  std::cout << "  Testtyp: " << order->getTestType() << "\n";
  std::cout << "  Status: " << order->getStatusString() << "\n\n";

  std::string confirm = readInput("Auftrag wirklich stornieren? (ja/nein)");
  if (!running_)
    return;

  std::string confirmLower = confirm;
  for (char &c : confirmLower) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }

  if (confirmLower == "ja" || confirmLower == "j" || confirmLower == "yes" ||
      confirmLower == "y") {
    order->setStatus(core::Order::Status::CANCELLED);
    order->setCompletedDate(0);
    if (database_->updateOrder(*order, getCurrentUsername())) {
      std::cout << "\n✓ Auftrag erfolgreich storniert!\n";
    } else {
      std::cout << "\n✗ Fehler beim Stornieren: " << database_->getLastError()
                << "\n";
    }
  } else {
    std::cout << "\nStornieren abgebrochen.\n";
  }

  waitForEnter();
}

void CliInterface::handleOrdersForSample() {
  clearScreen();
  printSeparator();
  std::cout << "        AUFTRÄGE ZU PROBE ANZEIGEN\n";
  printSeparator();
  std::cout << "\n";

  std::string sampleId = readInput("Proben-ID (Barcode)");
  if (!running_)
    return;
  sampleId = trim(sampleId);

  if (isEmpty(sampleId)) {
    std::cout << "\n✗ Bitte geben Sie eine Proben-ID ein.\n";
    waitForEnter();
    return;
  }

  // Prüfen ob Probe existiert
  auto sample = database_->getSampleByBarcode(sampleId);
  if (!sample) {
    std::cout << "\n✗ Probe nicht gefunden.\n";
    waitForEnter();
    return;
  }

  auto orders = database_->getOrdersBySampleId(sampleId);

  if (database_->hasError()) {
    std::cout << "✗ Fehler beim Abrufen der Aufträge:\n";
    std::cout << "  " << database_->getLastError() << "\n";
  } else if (orders.empty()) {
    std::cout << "\nℹ Keine Aufträge für diese Probe vorhanden.\n";
  } else {
    std::cout << "\nAufträge für Probe " << sampleId << ":\n\n";
    std::cout << std::left << std::setw(5) << "ID" << std::setw(12)
              << "Auftrags-ID" << std::setw(15) << "Testtyp" << std::setw(14)
              << "Status" << std::setw(10) << "Priorität" << "\n";
    printSeparator();

    for (const auto &order : orders) {
      std::cout << std::left << std::setw(5) << order->getId() << std::setw(12)
                << order->getOrderId() << std::setw(15) << order->getTestType()
                << std::setw(14) << order->getStatusString() << std::setw(10)
                << order->getPriorityString() << "\n";
    }

    std::cout << "\nGesamt: " << orders.size() << " Aufträge\n";
  }

  waitForEnter();
}

// ============================================================================
// TestResult-Menü-Aktionen
// ============================================================================

void CliInterface::handleNewResult() {
  clearScreen();
  printSeparator();
  std::cout << "         NEUES ERGEBNIS ERFASSEN\n";
  printSeparator();
  std::cout << "\n";

  if (!canEdit()) {
    std::cout << "✗ Keine Berechtigung. Bitte anmelden.\n";
    waitForEnter();
    return;
  }

  // Auftrag-ID als Referenz
  int orderId = readInteger("Auftrags-ID (Datenbank-ID)");
  if (!running_)
    return;

  // Prüfen ob Auftrag existiert
  auto order = database_->getOrder(orderId);
  if (!order) {
    std::cout << "\n✗ Auftrag mit ID " << orderId << " nicht gefunden.\n";
    waitForEnter();
    return;
  }

  std::cout << "Auftrag gefunden: " << order->getOrderId() << " - "
            << order->getTestType() << "\n\n";

  std::string resultId = readValidatedInput("Ergebnis-ID", "Ergebnis-ID");
  if (!running_)
    return;

  std::string testParameter =
      readValidatedInput("Testparameter (z.B. Glucose)", "Testparameter");
  if (!running_)
    return;

  std::string value;
  while (true) {
    value = trim(readInput("Messwert"));
    if (!running_)
      return;
    if (isEmpty(value)) {
      std::cout << "✗ Messwert darf nicht leer sein!\n";
      continue;
    }
    break;
  }

  std::string unit;
  while (true) {
    unit = trim(readInput("Einheit (z.B. mg/dL)"));
    if (!running_)
      return;
    if (isEmpty(unit)) {
      std::cout << "✗ Einheit darf nicht leer sein!\n";
      continue;
    }
    break;
  }

  std::string referenceRange = readInput("Referenzbereich (z.B. 70-100)");
  if (!running_)
    return;
  referenceRange = trim(referenceRange);

  double referenceLow = 0.0;
  double referenceHigh = 0.0;

  while (true) {
    std::string refLowStr =
        trim(readInput("Unterer Referenzwert (numerisch, optional)"));
    if (!running_)
      return;
    if (isEmpty(refLowStr)) {
      referenceLow = 0.0;
      break;
    }
    try {
      referenceLow = std::stod(refLowStr);
      break;
    } catch (...) {
      std::cout << "✗ Unterer Referenzwert muss numerisch sein!\n";
    }
  }

  while (true) {
    std::string refHighStr =
        trim(readInput("Oberer Referenzwert (numerisch, optional)"));
    if (!running_)
      return;
    if (isEmpty(refHighStr)) {
      referenceHigh = 0.0;
      break;
    }
    try {
      referenceHigh = std::stod(refHighStr);
      break;
    } catch (...) {
      std::cout << "✗ Oberer Referenzwert muss numerisch sein!\n";
    }
  }

  std::string measuredBy = readInput("Gemessen von (Benutzer)");
  if (!running_)
    return;
  measuredBy = trim(measuredBy);

  std::string comment = readInput("Kommentar (optional)");
  if (!running_)
    return;
  comment = trim(comment);

  // TestResult erstellen
  core::TestResult result(resultId, orderId, testParameter);
  result.setValue(value);
  result.setUnit(unit);
  result.setReferenceRange(referenceRange);
  result.setReferenceLow(referenceLow);
  result.setReferenceHigh(referenceHigh);
  result.setStatus(core::TestResult::Status::ENTERED);
  result.setMeasuredDate(std::time(nullptr));
  result.setMeasuredBy(measuredBy);
  result.setComment(comment);

  // Flag automatisch berechnen
  result.setFlag(result.evaluateFlag());

  if (database_->createTestResult(result, getCurrentUsername())) {
    std::cout << "\n✓ Ergebnis erfolgreich erfasst!\n";
    std::cout << "  Flag: " << result.getFlagString() << "\n";
  } else {
    std::cout << "\n✗ Fehler beim Erfassen des Ergebnisses:\n";
    std::cout << "  " << database_->getLastError() << "\n";
  }

  waitForEnter();
}

void CliInterface::handleListResults() {
  clearScreen();
  printSeparator();
  std::cout << "            ALLE ERGEBNISSE\n";
  printSeparator();
  std::cout << "\n";

  auto printResults =
      [&](const std::vector<std::unique_ptr<core::TestResult>> &entries) {
        if (database_->hasError()) {
          std::cout << "✗ Fehler beim Abrufen der Ergebnisse:\n";
          std::cout << "  " << database_->getLastError() << "\n";
        } else if (entries.empty()) {
          std::cout << "ℹ Keine Ergebnisse in der Datenbank vorhanden.\n";
        } else {
          std::cout << std::left << std::setw(5) << "ID" << std::setw(12)
                    << "Ergebnis-ID" << std::setw(10) << "Auftrag"
                    << std::setw(15) << "Parameter" << std::setw(10) << "Wert"
                    << std::setw(8) << "Einheit" << std::setw(12) << "Status"
                    << std::setw(10) << "Flag" << "\n";
          printSeparator();

          for (const auto &result : entries) {
            std::cout << std::left << std::setw(5) << result->getId()
                      << std::setw(12) << result->getResultId()
                      << std::setw(10) << result->getOrderId()
                      << std::setw(15) << result->getTestParameter()
                      << std::setw(10) << result->getValue() << std::setw(8)
                      << result->getUnit() << std::setw(12)
                      << result->getStatusString() << std::setw(10)
                      << result->getFlagString() << "\n";
          }

          std::cout << "\nGesamt: " << entries.size() << " Ergebnisse\n";
        }
      };

  auto results = database_->getAllTestResults();

  printResults(results);

  std::string autoRefreshInput =
      readInput("\nAuto-Refresh alle 5s aktivieren? (j/n)");
  if (!running_)
    return;
  autoRefreshInput = trim(autoRefreshInput);
  if (!autoRefreshInput.empty() &&
      (autoRefreshInput == "j" || autoRefreshInput == "ja" ||
       autoRefreshInput == "y" || autoRefreshInput == "yes")) {
    const int refreshSeconds = autoRefreshIntervalSeconds();
    std::cout << "\nAuto-Refresh aktiv. 'q' + Enter beendet.\n";
    while (waitForAutoRefresh(refreshSeconds)) {
      clearScreen();
      printSeparator();
      std::cout << "            ALLE ERGEBNISSE\n";
      printSeparator();
      std::cout << "\n";
      auto refreshed = database_->getAllTestResults();
      printResults(refreshed);
      std::cout << "\nAuto-Refresh aktiv. 'q' + Enter beendet.\n";
    }
    return;
  }

  waitForEnter();
}

void CliInterface::handleSearchResult() {
  clearScreen();
  printSeparator();
  std::cout << "           ERGEBNIS SUCHEN\n";
  printSeparator();
  std::cout << "\n";

  std::string resultId = readInput("Ergebnis-ID");
  if (!running_)
    return;
  resultId = trim(resultId);

  if (isEmpty(resultId)) {
    std::cout << "\n✗ Bitte geben Sie eine Ergebnis-ID ein.\n";
    waitForEnter();
    return;
  }

  auto result = database_->getTestResultByResultId(resultId);

  if (database_->hasError() || !result) {
    std::cout << "\n✗ Ergebnis nicht gefunden.\n";
  } else {
    auto printResultDetail =
        [&](const core::TestResult &detail, bool supportView) {
          std::cout << "\n✓ Ergebnis gefunden:\n";
          printSeparator();
          std::cout << "  ID:              " << detail.getId() << "\n";
          std::cout << "  Ergebnis-ID:     " << detail.getResultId() << "\n";
          std::cout << "  Auftrags-ID:     " << detail.getOrderId() << "\n";
          std::cout << "  Testparameter:   " << detail.getTestParameter() << "\n";
          std::cout << "  Messwert:        " << detail.getValue() << " "
                    << detail.getUnit() << "\n";
          std::cout << "  Status:          " << detail.getStatusString() << "\n";
          std::cout << "  Flag:            " << detail.getFlagString() << "\n";
          if (!supportView) {
            std::cout << "  Referenzbereich: " << detail.getReferenceRange() << "\n";
            std::cout << "  Ref. niedrig:    " << detail.getReferenceLow() << "\n";
            std::cout << "  Ref. hoch:       " << detail.getReferenceHigh() << "\n";
            std::cout << "  Gemessen von:    " << detail.getMeasuredBy() << "\n";
            std::cout << "  Kommentar:       " << detail.getComment() << "\n";
          }
        };

    const bool supportView = canAccessSupportData() && !isAdmin();
    if (supportView) {
      const std::string actor =
          currentUser_ ? currentUser_->getUsername() : std::string("system");
      if (!database_->logSupportAccess(core::AuditEntry::EntityType::RESULT,
                                       result->getResultId(), actor)) {
        std::cout << "\n⚠ Zugriff konnte nicht protokolliert werden: "
                  << database_->getLastError() << "\n";
      }
    }
    printResultDetail(*result, supportView);

    std::string autoRefreshInput =
        readInput("\nAuto-Refresh alle 5s aktivieren? (j/n)");
    if (!running_)
      return;
    autoRefreshInput = trim(autoRefreshInput);
    if (!autoRefreshInput.empty() &&
        (autoRefreshInput == "j" || autoRefreshInput == "ja" ||
         autoRefreshInput == "y" || autoRefreshInput == "yes")) {
      const int refreshSeconds = autoRefreshIntervalSeconds();
      std::cout << "\nAuto-Refresh aktiv. 'q' + Enter beendet.\n";
      while (waitForAutoRefresh(refreshSeconds)) {
        clearScreen();
        printSeparator();
        std::cout << "           ERGEBNIS SUCHEN\n";
        printSeparator();
        std::cout << "\n";
        auto refreshed = database_->getTestResultByResultId(resultId);
        if (!refreshed) {
          std::cout << "\n✗ Ergebnis nicht gefunden.\n";
          break;
        }
        printResultDetail(*refreshed, supportView);
        std::cout << "\nAuto-Refresh aktiv. 'q' + Enter beendet.\n";
      }
      return;
    }
  }

  waitForEnter();
}

void CliInterface::handleUpdateResult() {
  clearScreen();
  printSeparator();
  std::cout << "        ERGEBNIS AKTUALISIEREN\n";
  printSeparator();
  std::cout << "\n";

  if (!canEdit()) {
    std::cout << "✗ Keine Berechtigung. Bitte anmelden.\n";
    waitForEnter();
    return;
  }

  int id = readInteger("Ergebnis-ID (Datenbank-ID)");
  if (!running_)
    return;

  auto result = database_->getTestResult(id);

  if (!result) {
    std::cout << "\n✗ Ergebnis nicht gefunden.\n";
    waitForEnter();
    return;
  }

  std::cout << "\nAktuelles Ergebnis: " << result->getResultId() << " - "
            << result->getTestParameter() << "\n";
  std::cout << "Aktueller Wert: " << result->getValue() << " "
            << result->getUnit() << "\n";
  std::cout << "\nNeue Werte eingeben (Enter = keine Änderung):\n\n";

  std::string value = readInput("Neuer Messwert [" + result->getValue() + "]");
  if (!running_)
    return;
  if (!isEmpty(value)) {
    result->setValue(trim(value));
  }

  std::string unit = readInput("Neue Einheit [" + result->getUnit() + "]");
  if (!running_)
    return;
  if (!isEmpty(unit)) {
    result->setUnit(trim(unit));
  }

  std::string comment =
      readInput("Neuer Kommentar [" + result->getComment() + "]");
  if (!running_)
    return;
  if (!isEmpty(comment)) {
    result->setComment(trim(comment));
  }

  // Flag neu berechnen
  result->setFlag(result->evaluateFlag());

  const std::string actor =
      currentUser_ ? currentUser_->getUsername() : std::string("system");

  if (database_->updateTestResultWithAudit(*result, actor)) {
    std::cout << "\n✓ Ergebnis erfolgreich aktualisiert!\n";
    std::cout << "  Neuer Flag: " << result->getFlagString() << "\n";
  } else {
    std::cout << "\n✗ Fehler beim Aktualisieren:\n";
    std::cout << "  " << database_->getLastError() << "\n";
  }

  waitForEnter();
}

void CliInterface::handleValidateResult() {
  clearScreen();
  printSeparator();
  std::cout << "        ERGEBNIS VALIDIEREN\n";
  printSeparator();
  std::cout << "\n";

  if (!canEdit()) {
    std::cout << "✗ Keine Berechtigung. Bitte anmelden.\n";
    waitForEnter();
    return;
  }

  int id = readInteger("Ergebnis-ID (Datenbank-ID)");
  if (!running_)
    return;

  auto result = database_->getTestResult(id);

  if (!result) {
    std::cout << "\n✗ Ergebnis nicht gefunden.\n";
    waitForEnter();
    return;
  }

  std::cout << "\nErgebnis: " << result->getResultId() << "\n";
  std::cout << "Parameter: " << result->getTestParameter() << "\n";
  std::cout << "Wert: " << result->getValue() << " " << result->getUnit()
            << "\n";
  std::cout << "Flag: " << result->getFlagString() << "\n";
  std::cout << "Aktueller Status: " << result->getStatusString() << "\n";

  std::cout << "\nValidierungsoptionen:\n";
  std::cout << "  [1] Validieren (Freigeben)\n";
  std::cout << "  [2] Ablehnen\n";
  std::cout << "  [3] Wiederholung anfordern\n";
  std::cout << "  [0] Abbrechen\n";

  int choice = readInteger("Ihre Wahl");
  if (!running_)
    return;

  core::TestResult::Status newStatus = result->getStatus();
  const std::string actor =
      currentUser_ ? currentUser_->getUsername() : std::string("system");

  switch (choice) {
  case 1:
    newStatus = core::TestResult::Status::VALIDATED;
    break;
  case 2:
    newStatus = core::TestResult::Status::REJECTED;
    break;
  case 3:
    newStatus = core::TestResult::Status::REPEATED;
    break;
  case 0:
    std::cout << "\nAbgebrochen.\n";
    waitForEnter();
    return;
  default:
    std::cout << "\nUngültige Auswahl.\n";
    waitForEnter();
    return;
  }

  if (choice == 1) {
    if (database_->validateTestResult(result->getResultId(), actor)) {
      result->setStatus(core::TestResult::Status::VALIDATED);
      std::cout << "\n✓ Status erfolgreich geändert auf: "
                << result->getStatusString() << "\n";
    } else {
      std::cout << "\n✗ Fehler beim Aktualisieren:\n";
      std::cout << "  " << database_->getLastError() << "\n";
    }
  } else {
    result->setStatus(newStatus);
    if (database_->updateTestResult(*result, getCurrentUsername())) {
      std::cout << "\n✓ Status erfolgreich geändert auf: "
                << result->getStatusString() << "\n";
    } else {
      std::cout << "\n✗ Fehler beim Aktualisieren:\n";
      std::cout << "  " << database_->getLastError() << "\n";
    }
  }

  waitForEnter();
}

void CliInterface::handleResultsForOrder() {
  clearScreen();
  printSeparator();
  std::cout << "      ERGEBNISSE ZU AUFTRAG ANZEIGEN\n";
  printSeparator();
  std::cout << "\n";

  int orderId = readInteger("Auftrags-ID (Datenbank-ID)");
  if (!running_)
    return;

  // Prüfen ob Auftrag existiert
  auto order = database_->getOrder(orderId);
  if (!order) {
    std::cout << "\n✗ Auftrag nicht gefunden.\n";
    waitForEnter();
    return;
  }

  std::cout << "Auftrag: " << order->getOrderId() << " - "
            << order->getTestType() << "\n\n";

  auto results = database_->getTestResultsByOrderId(orderId);

  if (database_->hasError()) {
    std::cout << "✗ Fehler beim Abrufen der Ergebnisse:\n";
    std::cout << "  " << database_->getLastError() << "\n";
  } else if (results.empty()) {
    std::cout << "ℹ Keine Ergebnisse für diesen Auftrag vorhanden.\n";
  } else {
    std::cout << std::left << std::setw(5) << "ID" << std::setw(12)
              << "Ergebnis-ID" << std::setw(15) << "Parameter" << std::setw(10)
              << "Wert" << std::setw(8) << "Einheit" << std::setw(12)
              << "Status" << std::setw(10) << "Flag" << "\n";
    printSeparator();

    for (const auto &result : results) {
      std::cout << std::left << std::setw(5) << result->getId() << std::setw(12)
                << result->getResultId() << std::setw(15)
                << result->getTestParameter() << std::setw(10)
                << result->getValue() << std::setw(8) << result->getUnit()
                << std::setw(12) << result->getStatusString() << std::setw(10)
                << result->getFlagString() << "\n";
    }

    std::cout << "\nGesamt: " << results.size() << " Ergebnisse\n";
  }

  waitForEnter();
}

void CliInterface::handleImportResultsCsv() {
  clearScreen();
  printSeparator();
  std::cout << "      CSV-ERGEBNISIMPORT\n";
  printSeparator();
  std::cout << "\n";

  std::cout << "Erwartetes CSV-Format:\n";
  std::cout << "result_id,order_id,test_parameter,value,unit,ref_low,ref_high,"
               "measured_by\n\n";

  std::string filePath = readInput("Pfad zur CSV-Datei");
  if (!running_)
    return;
  filePath = trim(filePath);

  if (isEmpty(filePath)) {
    std::cout << "\n✗ Bitte geben Sie einen Dateipfad ein.\n";
    waitForEnter();
    return;
  }

  auto buildRetryPath = [](const std::string &path) {
    std::string retryPath = path;
    const std::string suffix = ".csv";
    size_t pos = retryPath.rfind(suffix);
    if (pos != std::string::npos && pos == retryPath.size() - suffix.size()) {
      retryPath.insert(pos, "_retry");
    } else {
      retryPath += "_retry.csv";
    }
    return retryPath;
  };

  std::string currentPath = filePath;
  bool retryImport = false;

  do {
    std::cout << "\nStarte Import von: " << currentPath << "\n\n";

    CsvResultImport importer(database_);
    auto results = importer.importResults(currentPath);
    std::vector<CsvResultImport::FailedRecord> dbFailedRecords;
    const bool isRetryAttempt = (currentPath != filePath);

    if (results.empty()) {
      std::cout << "\n✗ Keine Ergebnisse importiert: "
                << importer.getLastError() << "\n";
    } else {
      int stored = 0;
      int failed = 0;
      std::vector<std::string> storedResultIds;

      for (const auto &result : results) {
        if (database_->createTestResult(result, getCurrentUsername())) {
          stored++;
          storedResultIds.push_back(result.getResultId());
        } else {
          failed++;
          const std::string error = database_->getLastError();
          dbFailedRecords.push_back({-1, result.getResultId() + "," +
                                             std::to_string(result.getOrderId()) +
                                             "," + result.getTestParameter() +
                                             "," + result.getValue() + "," +
                                             result.getUnit() + "," +
                                             std::to_string(result.getReferenceLow()) +
                                             "," +
                                             std::to_string(result.getReferenceHigh()) +
                                             "," + result.getMeasuredBy(),
                                     error});
        }
      }

      if (isRetryAttempt && !storedResultIds.empty()) {
        const std::string actor =
            currentUser_ ? currentUser_->getUsername() : std::string("system");
        database_->logResultRetryImport(storedResultIds, actor, currentPath);
      }

      std::cout << "\n✓ " << stored << " Ergebnisse erfolgreich in Datenbank "
                << "gespeichert.\n";

      if (failed > 0) {
        std::cout << "\n✗ " << failed
                  << " Ergebnisse konnten nicht importiert werden.\n";
      }
    }

    if (importer.getErrorCount() > 0 || !dbFailedRecords.empty()) {
      if (importer.getErrorCount() > 0) {
        std::cout << "\n✗ CSV-Fehler (" << importer.getErrorCount()
                  << " Zeilen):\n";
        for (const auto &failed : importer.getFailedRecords()) {
          std::cout << "  - Zeile " << failed.recordNumber << ": "
                    << failed.error << " (\"" << failed.record << "\")\n";
        }
      }

      if (!dbFailedRecords.empty()) {
        std::cout << "\n✗ DB-Fehler (" << dbFailedRecords.size()
                  << " Zeilen):\n";
        for (const auto &failed : dbFailedRecords) {
          std::cout << "  - " << failed.error << " (\"" << failed.record
                    << "\")\n";
        }
      }

      std::string retryPath = buildRetryPath(currentPath);
      if (importer.writeRetryCsv(retryPath, dbFailedRecords)) {
        std::cout << "\n✓ Retry-Datei erstellt: " << retryPath << "\n";
        std::cout << "  Tipp: Datei korrigieren und erneut importieren.\n";

        std::string choice = readInput("Retry-Datei jetzt importieren? (y/n)");
        if (!running_)
          return;

        if (!choice.empty() &&
            (choice[0] == 'y' || choice[0] == 'Y')) {
          currentPath = retryPath;
          retryImport = true;
          continue;
        }
      } else {
        std::cout << "\n✗ Konnte Retry-Datei nicht schreiben.\n";
      }
    }

    retryImport = false;
  } while (retryImport);

  waitForEnter();
}

void CliInterface::handleExportResults() {
  clearScreen();
  printSeparator();
  std::cout << "        ERGEBNISSE EXPORTIEREN\n";
  printSeparator();
  std::cout << "\n";

  if (!canEdit()) {
    std::cout << "✗ Keine Berechtigung. Bitte anmelden.\n";
    waitForEnter();
    return;
  }

  std::string orderInput =
      readInput("Auftrags-ID (optional, Enter = alle)");
  if (!running_)
    return;

  std::optional<int> orderId;
  orderInput = trim(orderInput);
  if (!isEmpty(orderInput)) {
    try {
      int parsed = std::stoi(orderInput);
      if (parsed <= 0) {
        throw std::invalid_argument("invalid");
      }
      orderId = parsed;
    } catch (const std::exception &) {
      std::cout << "\n✗ Ungültige Auftrags-ID.\n";
      waitForEnter();
      return;
    }
  }

  std::cout << "\nExportformat:\n";
  std::cout << "  [1] CSV\n";

  int format = readInteger("Ihre Wahl");
  if (!running_)
    return;
  if (format != 1) {
    std::cout << "\n✗ Ungültiges Format.\n";
    waitForEnter();
    return;
  }

  std::string filePath = readInput("Export-Dateipfad");
  if (!running_)
    return;
  filePath = trim(filePath);
  if (isEmpty(filePath)) {
    std::cout << "\n✗ Bitte geben Sie einen Dateipfad an.\n";
    waitForEnter();
    return;
  }

  const std::string actor =
      currentUser_ ? currentUser_->getUsername() : std::string("system");

  if (database_->exportValidatedResultsToCsv(filePath, actor, orderId)) {
    std::cout << "\n✓ Export erfolgreich!\n";
  } else {
    std::cout << "\n✗ Fehler beim Export:\n";
    std::cout << "  " << database_->getLastError() << "\n";
  }

  waitForEnter();
}

// ============================================================================
// Audit-Trail-Handler
// ============================================================================

void CliInterface::handleShowAuditLog() {
  clearScreen();
  printSeparator();
  std::cout << "              AUDIT-LOG\n";
  printSeparator();
  std::cout << "\n";

  std::string filterInput = readInput("Filter anwenden? (j/n)");
  if (!running_)
    return;
  filterInput = trim(filterInput);
  const bool applyFilters =
      (!filterInput.empty() &&
       (filterInput == "j" || filterInput == "ja" || filterInput == "y" ||
        filterInput == "yes"));

  db::Database::AuditLogFilter filter;

  if (applyFilters) {
    std::cout << "\nFilter (leer lassen = kein Filter):\n";

    std::string user = readInput("Benutzer");
    if (!running_)
      return;
    user = trim(user);
    if (!isEmpty(user)) {
      filter.user = user;
    }

    std::cout << "\nAktion filtern:\n";
    std::cout << "  [0] Alle\n";
    std::cout << "  [1] Erstellt\n";
    std::cout << "  [2] Aktualisiert\n";
    std::cout << "  [3] Gelöscht\n";
    std::cout << "  [4] Angemeldet\n";
    std::cout << "  [5] Abgemeldet\n";
    std::cout << "  [6] Validiert\n\n";

    int actionChoice = readInteger("Aktion (0-6)");
    if (!running_)
      return;
    switch (actionChoice) {
    case 0:
      break;
    case 1:
      filter.action = core::AuditEntry::ActionType::CREATE;
      break;
    case 2:
      filter.action = core::AuditEntry::ActionType::UPDATE;
      break;
    case 3:
      filter.action = core::AuditEntry::ActionType::DELETE;
      break;
    case 4:
      filter.action = core::AuditEntry::ActionType::LOGIN;
      break;
    case 5:
      filter.action = core::AuditEntry::ActionType::LOGOUT;
      break;
    case 6:
      filter.action = core::AuditEntry::ActionType::VALIDATE;
      break;
    default:
      std::cout << "\n✗ Ungültige Auswahl.\n";
      waitForEnter();
      return;
    }

    std::cout << "\nEntität filtern:\n";
    std::cout << "  [0] Alle\n";
    std::cout << "  [1] Probe\n";
    std::cout << "  [2] Auftrag\n";
    std::cout << "  [3] Ergebnis\n";
    std::cout << "  [4] Benutzer\n";
    std::cout << "  [5] Rolle\n";
    std::cout << "  [6] System\n\n";

    int entityChoice = readInteger("Entität (0-6)");
    if (!running_)
      return;
    switch (entityChoice) {
    case 0:
      break;
    case 1:
      filter.entity = core::AuditEntry::EntityType::SAMPLE;
      break;
    case 2:
      filter.entity = core::AuditEntry::EntityType::ORDER;
      break;
    case 3:
      filter.entity = core::AuditEntry::EntityType::RESULT;
      break;
    case 4:
      filter.entity = core::AuditEntry::EntityType::USER;
      break;
    case 5:
      filter.entity = core::AuditEntry::EntityType::ROLE;
      break;
    case 6:
      filter.entity = core::AuditEntry::EntityType::SYSTEM;
      break;
    default:
      std::cout << "\n✗ Ungültige Auswahl.\n";
      waitForEnter();
      return;
    }

    std::string fromTime = readInput("Von (Unix-Zeitstempel)");
    if (!running_)
      return;
    fromTime = trim(fromTime);
    if (!isEmpty(fromTime)) {
      try {
        filter.fromTime = static_cast<std::time_t>(std::stoll(fromTime));
      } catch (...) {
        std::cout << "\n✗ Ungültiger Zeitstempel.\n";
        waitForEnter();
        return;
      }
    }

    std::string toTime = readInput("Bis (Unix-Zeitstempel)");
    if (!running_)
      return;
    toTime = trim(toTime);
    if (!isEmpty(toTime)) {
      try {
        filter.toTime = static_cast<std::time_t>(std::stoll(toTime));
      } catch (...) {
        std::cout << "\n✗ Ungültiger Zeitstempel.\n";
        waitForEnter();
        return;
      }
    }
  }

  std::string limitStr = readInput("Anzahl der anzuzeigenden Einträge (50)");
  if (!running_)
    return;

  int limit = 50;
  if (!isEmpty(limitStr)) {
    try {
      limit = std::stoi(trim(limitStr));
      if (limit <= 0)
        limit = 50;
    } catch (...) {
      limit = 50;
    }
  }

  filter.limit = limit;

  auto printEntries =
      [&](const std::vector<std::unique_ptr<core::AuditEntry>> &entries) {
        if (database_->hasError()) {
          std::cout << "✗ Fehler beim Abrufen des Audit-Logs:\n";
          std::cout << "  " << database_->getLastError() << "\n";
          return;
        }
        if (entries.empty()) {
          std::cout << "ℹ Keine Audit-Einträge vorhanden.\n";
          return;
        }

        std::cout << std::left << std::setw(5) << "ID" << std::setw(20)
                  << "Zeitstempel" << std::setw(14) << "Aktion"
                  << std::setw(12) << "Entität" << std::setw(12)
                  << "Entität-ID" << std::setw(15) << "Benutzer" << "\n";
        printSeparator();

        for (const auto &entry : entries) {
          std::cout << std::left << std::setw(5) << entry->getId()
                    << std::setw(20) << entry->getTimestampString()
                    << std::setw(14) << entry->getActionString()
                    << std::setw(12) << entry->getEntityString()
                    << std::setw(12) << entry->getEntityId() << std::setw(15)
                    << entry->getUser() << "\n";

          if (!entry->getDetails().empty()) {
            std::cout << "      Details: " << entry->getDetails() << "\n";
          }
        }

        std::cout << "\nAngezeigt: " << entries.size() << " Einträge\n";
      };

  auto entries = database_->getAuditLogFiltered(filter);
  printEntries(entries);

  if (applyFilters) {
    std::string reset = readInput("Filter zurücksetzen und alle anzeigen? (j/n)");
    if (!running_)
      return;
    reset = trim(reset);
    if (!reset.empty() &&
        (reset == "j" || reset == "ja" || reset == "y" || reset == "yes")) {
      db::Database::AuditLogFilter resetFilter;
      resetFilter.limit = limit;
      auto resetEntries = database_->getAuditLogFiltered(resetFilter);
      printEntries(resetEntries);
    }
  }

  waitForEnter();
}

void CliInterface::handleAuditForEntity() {
  clearScreen();
  printSeparator();
  std::cout << "        AUDIT FÜR ENTITÄT ANZEIGEN\n";
  printSeparator();
  std::cout << "\n";

  std::cout << "Entitätstyp auswählen:\n";
  std::cout << "  [1] Probe\n";
  std::cout << "  [2] Auftrag\n";
  std::cout << "  [3] Ergebnis\n";
  std::cout << "  [0] Abbrechen\n\n";

  int choice = readInteger("Ihre Wahl");
  if (!running_)
    return;

  core::AuditEntry::EntityType entityType;
  std::string entityTypeName;

  switch (choice) {
  case 1:
    entityType = core::AuditEntry::EntityType::SAMPLE;
    entityTypeName = "Proben";
    break;
  case 2:
    entityType = core::AuditEntry::EntityType::ORDER;
    entityTypeName = "Auftrags";
    break;
  case 3:
    entityType = core::AuditEntry::EntityType::RESULT;
    entityTypeName = "Ergebnis";
    break;
  case 0:
    std::cout << "\nAbgebrochen.\n";
    waitForEnter();
    return;
  default:
    std::cout << "\nUngültige Auswahl.\n";
    waitForEnter();
    return;
  }

  std::string entityId = readInput(entityTypeName + "-ID");
  if (!running_)
    return;
  entityId = trim(entityId);

  if (isEmpty(entityId)) {
    std::cout << "\n✗ Bitte geben Sie eine ID ein.\n";
    waitForEnter();
    return;
  }

  auto entries = database_->getAuditLogByEntity(entityType, entityId);

  if (database_->hasError()) {
    std::cout << "✗ Fehler beim Abrufen der Audit-Einträge:\n";
    std::cout << "  " << database_->getLastError() << "\n";
  } else if (entries.empty()) {
    std::cout << "\nℹ Keine Audit-Einträge für " << entityTypeName << "-ID '"
              << entityId << "' gefunden.\n";
  } else {
    std::cout << "\nAudit-Historie für " << entityTypeName << "-ID '"
              << entityId << "':\n\n";
    std::cout << std::left << std::setw(5) << "ID" << std::setw(20)
              << "Zeitstempel" << std::setw(14) << "Aktion" << std::setw(15)
              << "Benutzer" << "\n";
    printSeparator();

    for (const auto &entry : entries) {
      std::cout << std::left << std::setw(5) << entry->getId() << std::setw(20)
                << entry->getTimestampString() << std::setw(14)
                << entry->getActionString() << std::setw(15) << entry->getUser()
                << "\n";

      if (!entry->getDetails().empty()) {
        std::cout << "      Details: " << entry->getDetails() << "\n";
      }
    }

    std::cout << "\nGesamt: " << entries.size() << " Einträge\n";
  }

  waitForEnter();
}

void CliInterface::handleConfigureRetention() {
  clearScreen();
  printSeparator();
  std::cout << "        RETENTION KONFIGURIEREN\n";
  printSeparator();
  std::cout << "\n";

  if (!isAdmin()) {
    std::cout << "✗ Nur Administratoren dürfen Retention ändern.\n";
    waitForEnter();
    return;
  }

  int currentDays = database_->getRetentionDays();
  if (database_->hasError()) {
    std::cout << "\n✗ Fehler beim Laden der Retention:\n";
    std::cout << "  " << database_->getLastError() << "\n";
    waitForEnter();
    return;
  }

  std::cout << "Aktuelle Retention: " << currentDays << " Tage\n";
  int newDays = readInteger("Neue Retention in Tagen (min. 180, 0=Abbruch)");
  if (!running_)
    return;

  if (newDays == 0) {
    std::cout << "\nAbgebrochen.\n";
    waitForEnter();
    return;
  }
  if (newDays < 0) {
    std::cout << "\n✗ Ungültiger Wert.\n";
    waitForEnter();
    return;
  }

  if (newDays < 180) {
    std::cout << "ℹ Mindestwert 180 Tage wird erzwungen.\n";
    newDays = 180;
  }

  if (database_->setRetentionDays(newDays)) {
    std::cout << "\n✓ Retention aktualisiert: " << newDays << " Tage\n";
  } else {
    std::cout << "\n✗ Fehler beim Speichern:\n";
    std::cout << "  " << database_->getLastError() << "\n";
  }

  waitForEnter();
}

void CliInterface::handleRunRetention() {
  clearScreen();
  printSeparator();
  std::cout << "        RETENTION AUSFÜHREN\n";
  printSeparator();
  std::cout << "\n";

  if (!isAdmin()) {
    std::cout << "✗ Nur Administratoren dürfen Retention ausführen.\n";
    waitForEnter();
    return;
  }

  int purged = 0;
  const std::string actor =
      currentUser_ ? currentUser_->getUsername() : std::string("system");

  if (database_->applyAuditRetention(actor, purged)) {
    if (purged > 0) {
      std::cout << "\n✓ Retention abgeschlossen: " << purged
                << " Einträge bereinigt.\n";
    } else {
      std::cout << "\nℹ Keine Einträge zum Bereinigen.\n";
    }
  } else {
    std::cout << "\n✗ Fehler beim Ausführen der Retention:\n";
    std::cout << "  " << database_->getLastError() << "\n";
  }

  waitForEnter();
}

void CliInterface::handleExportAuditLog() {
  clearScreen();
  printSeparator();
  std::cout << "        AUDIT-LOG EXPORTIEREN\n";
  printSeparator();
  std::cout << "\n";

  if (!isAdmin()) {
    std::cout << "✗ Nur Administratoren dürfen Audit-Logs exportieren.\n";
    waitForEnter();
    return;
  }

  std::string filePath = readInput("Dateipfad für Export");
  if (!running_)
    return;
  filePath = trim(filePath);
  if (isEmpty(filePath)) {
    std::cout << "\n✗ Bitte geben Sie einen Dateipfad an.\n";
    waitForEnter();
    return;
  }

  std::string filterInput = readInput("Filter anwenden? (j/n)");
  if (!running_)
    return;
  filterInput = trim(filterInput);
  const bool applyFilters =
      (!filterInput.empty() &&
       (filterInput == "j" || filterInput == "ja" || filterInput == "y" ||
        filterInput == "yes"));

  db::Database::AuditLogFilter filter;

  if (applyFilters) {
    std::cout << "\nFilter (leer lassen = kein Filter):\n";

    std::string user = readInput("Benutzer");
    if (!running_)
      return;
    user = trim(user);
    if (!isEmpty(user)) {
      filter.user = user;
    }

    std::cout << "\nAktion filtern:\n";
    std::cout << "  [0] Alle\n";
    std::cout << "  [1] Erstellt\n";
    std::cout << "  [2] Aktualisiert\n";
    std::cout << "  [3] Gelöscht\n";
    std::cout << "  [4] Angemeldet\n";
    std::cout << "  [5] Abgemeldet\n";
    std::cout << "  [6] Validiert\n\n";

    int actionChoice = readInteger("Aktion (0-6)");
    if (!running_)
      return;
    switch (actionChoice) {
    case 0:
      break;
    case 1:
      filter.action = core::AuditEntry::ActionType::CREATE;
      break;
    case 2:
      filter.action = core::AuditEntry::ActionType::UPDATE;
      break;
    case 3:
      filter.action = core::AuditEntry::ActionType::DELETE;
      break;
    case 4:
      filter.action = core::AuditEntry::ActionType::LOGIN;
      break;
    case 5:
      filter.action = core::AuditEntry::ActionType::LOGOUT;
      break;
    case 6:
      filter.action = core::AuditEntry::ActionType::VALIDATE;
      break;
    default:
      std::cout << "\n✗ Ungültige Auswahl.\n";
      waitForEnter();
      return;
    }

    std::cout << "\nEntität filtern:\n";
    std::cout << "  [0] Alle\n";
    std::cout << "  [1] Probe\n";
    std::cout << "  [2] Auftrag\n";
    std::cout << "  [3] Ergebnis\n";
    std::cout << "  [4] Benutzer\n";
    std::cout << "  [5] Rolle\n";
    std::cout << "  [6] System\n\n";

    int entityChoice = readInteger("Entität (0-6)");
    if (!running_)
      return;
    switch (entityChoice) {
    case 0:
      break;
    case 1:
      filter.entity = core::AuditEntry::EntityType::SAMPLE;
      break;
    case 2:
      filter.entity = core::AuditEntry::EntityType::ORDER;
      break;
    case 3:
      filter.entity = core::AuditEntry::EntityType::RESULT;
      break;
    case 4:
      filter.entity = core::AuditEntry::EntityType::USER;
      break;
    case 5:
      filter.entity = core::AuditEntry::EntityType::ROLE;
      break;
    case 6:
      filter.entity = core::AuditEntry::EntityType::SYSTEM;
      break;
    default:
      std::cout << "\n✗ Ungültige Auswahl.\n";
      waitForEnter();
      return;
    }

    std::string entityId = readInput("Entität-ID");
    if (!running_)
      return;
    entityId = trim(entityId);
    if (!isEmpty(entityId)) {
      filter.entityId = entityId;
    }

    std::string fromTime = readInput("Von (Unix-Zeitstempel)");
    if (!running_)
      return;
    fromTime = trim(fromTime);
    if (!isEmpty(fromTime)) {
      try {
        filter.fromTime = static_cast<std::time_t>(std::stoll(fromTime));
      } catch (...) {
        std::cout << "\n✗ Ungültiger Zeitstempel.\n";
        waitForEnter();
        return;
      }
    }

    std::string toTime = readInput("Bis (Unix-Zeitstempel)");
    if (!running_)
      return;
    toTime = trim(toTime);
    if (!isEmpty(toTime)) {
      try {
        filter.toTime = static_cast<std::time_t>(std::stoll(toTime));
      } catch (...) {
        std::cout << "\n✗ Ungültiger Zeitstempel.\n";
        waitForEnter();
        return;
      }
    }
  }

  std::string limitStr = readInput("Limit (100)");
  if (!running_)
    return;

  int limit = 100;
  if (!isEmpty(limitStr)) {
    try {
      limit = std::stoi(trim(limitStr));
    } catch (...) {
      std::cout << "\n✗ Ungültiges Limit.\n";
      waitForEnter();
      return;
    }
  }
  if (limit <= 0) {
    std::cout << "\n✗ Limit muss größer als 0 sein.\n";
    waitForEnter();
    return;
  }
  filter.limit = limit;

  int exported = 0;
  const std::string actor =
      currentUser_ ? currentUser_->getUsername() : std::string("system");
  if (database_->exportAuditLogToCsv(filePath, filter, actor, exported)) {
    std::cout << "\n✓ Export abgeschlossen: " << exported
              << " Einträge -> " << filePath << "\n";
  } else {
    std::cout << "\n✗ Fehler beim Export:\n";
    std::cout << "  " << database_->getLastError() << "\n";
  }

  waitForEnter();
}

// ============================================================================
// Benutzer-Handler (Authentifizierung)
// ============================================================================

void CliInterface::handleLogin() {
  clearScreen();
  printSeparator();
  std::cout << "               ANMELDEN\n";
  printSeparator();
  std::cout << "\n";

  if (isLoggedIn()) {
    std::cout << "Sie sind bereits angemeldet als: "
              << currentUser_->getUsername() << "\n";
    std::cout << "Bitte zuerst abmelden.\n";
    waitForEnter();
    return;
  }

  std::string username = readInput("Benutzername");
  if (!running_)
    return;
  username = trim(username);

  if (isEmpty(username)) {
    std::cout << "\n✗ Bitte geben Sie einen Benutzernamen ein.\n";
    waitForEnter();
    return;
  }

  std::string password = readInput("Passwort");
  if (!running_)
    return;

  auto user = database_->authenticateUser(username, password);

  if (!user) {
    const std::string err = database_->getLastError();
    if (err.find("MFA") != std::string::npos) {
      std::string mfaCode = readInput("MFA-Code");
      if (!running_)
        return;
      user = database_->authenticateUser(username, password, trim(mfaCode));
    }
  }

  if (user) {
    currentUser_ = std::move(user);
    std::cout << "\n✓ Erfolgreich angemeldet als: "
              << currentUser_->getUsername() << " ("
              << currentUser_->getRoleString() << ")\n";
  } else {
    std::cout << "\n✗ Anmeldung fehlgeschlagen: " << database_->getLastError()
              << "\n";
  }

  waitForEnter();
}

void CliInterface::handleLogout() {
  clearScreen();
  printSeparator();
  std::cout << "               ABMELDEN\n";
  printSeparator();
  std::cout << "\n";

  if (!isLoggedIn()) {
    std::cout << "Sie sind nicht angemeldet.\n";
    waitForEnter();
    return;
  }

  const int userId = currentUser_->getId();
  std::string username = currentUser_->getUsername();

  if (!database_->endSession(userId, username, "logout")) {
    std::cout << "✗ Sitzung konnte nicht sauber beendet werden: "
              << database_->getLastError() << "\n";
    database_->clearError();
  }

  currentUser_.reset();
  std::cout << "✓ Benutzer '" << username << "' erfolgreich abgemeldet.\n";

  waitForEnter();
}

void CliInterface::handleChangePassword() {
  clearScreen();
  printSeparator();
  std::cout << "          PASSWORT ÄNDERN\n";
  printSeparator();
  std::cout << "\n";

  if (!isLoggedIn()) {
    std::cout << "✗ Bitte zuerst anmelden.\n";
    waitForEnter();
    return;
  }

  std::string oldPassword = readInput("Aktuelles Passwort");
  if (!running_)
    return;

  if (!currentUser_->verifyPassword(oldPassword)) {
    std::cout << "\n✗ Aktuelles Passwort ist falsch.\n";
    waitForEnter();
    return;
  }

  std::string newPassword = readInput("Neues Passwort");
  if (!running_)
    return;

  if (newPassword.length() < 4) {
    std::cout << "\n✗ Passwort muss mindestens 4 Zeichen haben.\n";
    waitForEnter();
    return;
  }

  std::string confirmPassword = readInput("Neues Passwort bestätigen");
  if (!running_)
    return;

  if (newPassword != confirmPassword) {
    std::cout << "\n✗ Passwörter stimmen nicht überein.\n";
    waitForEnter();
    return;
  }

  currentUser_->setPassword(newPassword);

  if (database_->updateUser(*currentUser_, getCurrentUsername())) {
    std::cout << "\n✓ Passwort erfolgreich geändert.\n";
  } else {
    std::cout << "\n✗ Fehler beim Ändern des Passworts: "
              << database_->getLastError() << "\n";
  }

  waitForEnter();
}

void CliInterface::handleCreateUser() {
  clearScreen();
  printSeparator();
  std::cout << "          BENUTZER ANLEGEN\n";
  printSeparator();
  std::cout << "\n";

  if (!isAdmin()) {
    std::cout << "✗ Nur Administratoren können Benutzer anlegen.\n";
    waitForEnter();
    return;
  }

  std::string username = readValidatedInput("Benutzername", "Benutzername");
  if (!running_)
    return;

  // Prüfen ob Benutzer existiert
  auto existing = database_->getUserByUsername(username);
  if (existing) {
    std::cout << "\n✗ Benutzer '" << username << "' existiert bereits.\n";
    waitForEnter();
    return;
  }
  database_->clearError();

  std::string password = readInput("Passwort");
  if (!running_)
    return;

  if (password.length() < 4) {
    std::cout << "\n✗ Passwort muss mindestens 4 Zeichen haben.\n";
    waitForEnter();
    return;
  }

  auto roles = database_->getAllRoles();
  if (database_->hasError() || roles.empty()) {
    database_->clearError();
    roles = {"Administrator", "Operator", "Betrachter"};
  }

  std::cout << "\nRolle:\n";
  for (size_t i = 0; i < roles.size(); ++i) {
    std::cout << "  [" << (i + 1) << "] " << roles[i] << "\n";
  }
  std::cout << "\n";

  const std::string prompt =
      "Rolle (1-" + std::to_string(roles.size()) + ")";
  int roleChoice = readInteger(prompt);
  if (!running_)
    return;

  if (roleChoice < 1 || static_cast<size_t>(roleChoice) > roles.size()) {
    std::cout << "\nUngültige Auswahl. Verwende '" << roles.front() << "'.\n";
    roleChoice = 1;
  }

  const std::string selectedRole = roles[static_cast<size_t>(roleChoice - 1)];

  std::string fullName = readInput("Vollständiger Name (optional)");
  if (!running_)
    return;
  fullName = trim(fullName);

  std::string email = readInput("E-Mail (optional)");
  if (!running_)
    return;
  email = trim(email);

  core::User newUser(username, core::User::hashPassword(password),
                     core::User::Role::OPERATOR);
  newUser.setRoleName(selectedRole);
  newUser.setFullName(fullName);
  newUser.setEmail(email);

  if (database_->createUser(newUser, getCurrentUsername())) {
    std::cout << "\n✓ Benutzer '" << username << "' erfolgreich angelegt.\n";
  } else {
    std::cout << "\n✗ Fehler beim Anlegen: " << database_->getLastError()
              << "\n";
  }

  waitForEnter();
}

void CliInterface::handleListUsers() {
  clearScreen();
  printSeparator();
  std::cout << "          ALLE BENUTZER\n";
  printSeparator();
  std::cout << "\n";

  if (!isAdmin()) {
    std::cout << "✗ Nur Administratoren können Benutzer anzeigen.\n";
    waitForEnter();
    return;
  }

  auto users = database_->getAllUsers();

  if (database_->hasError()) {
    std::cout << "✗ Fehler beim Abrufen der Benutzer:\n";
    std::cout << "  " << database_->getLastError() << "\n";
  } else if (users.empty()) {
    std::cout << "ℹ Keine Benutzer in der Datenbank.\n";
  } else {
    std::cout << std::left << std::setw(5) << "ID" << std::setw(15)
              << "Benutzer" << std::setw(15) << "Rolle" << std::setw(8)
              << "Aktiv" << std::setw(25) << "Name" << std::setw(25) << "E-Mail"
              << "\n";
    printSeparator();

    for (const auto &user : users) {
      std::cout << std::left << std::setw(5) << user->getId() << std::setw(15)
                << user->getUsername() << std::setw(15) << user->getRoleString()
                << std::setw(8) << (user->isActive() ? "Ja" : "Nein")
                << std::setw(25) << user->getFullName() << std::setw(25)
                << user->getEmail() << "\n";
    }

    std::cout << "\nGesamt: " << users.size() << " Benutzer\n";
  }

  waitForEnter();
}

void CliInterface::handleUpdateUser() {
  clearScreen();
  printSeparator();
  std::cout << "        BENUTZER BEARBEITEN\n";
  printSeparator();
  std::cout << "\n";

  if (!isAdmin()) {
    std::cout << "✗ Nur Administratoren können Benutzer bearbeiten.\n";
    waitForEnter();
    return;
  }

  int id = readInteger("Benutzer-ID (numerisch)");
  if (!running_)
    return;

  auto user = database_->getUser(id);

  if (!user) {
    std::cout << "\n✗ Benutzer nicht gefunden: " << database_->getLastError()
              << "\n";
    waitForEnter();
    return;
  }

  std::cout << "\nAktueller Benutzer:\n";
  std::cout << "  Benutzername: " << user->getUsername() << "\n";
  std::cout << "  Rolle:        " << user->getRoleString() << "\n";
  std::cout << "  Aktiv:        " << (user->isActive() ? "Ja" : "Nein") << "\n";
  std::cout << "  Name:         " << user->getFullName() << "\n";
  std::cout << "  E-Mail:       " << user->getEmail() << "\n\n";

  std::cout << "Was möchten Sie ändern?\n";
  std::cout << "  [1] Rolle ändern\n";
  std::cout << "  [2] Aktivstatus ändern\n";
  std::cout << "  [3] Name/E-Mail ändern\n";
  std::cout << "  [4] Passwort zurücksetzen\n";
  std::cout << "  [0] Abbrechen\n\n";

  int choice = readInteger("Ihre Wahl");
  if (!running_)
    return;

  bool roleChanged = false;
  std::string selectedRoleName;

  switch (choice) {
  case 1: {
    auto roles = database_->getAllRoles();
    if (database_->hasError() || roles.empty()) {
      database_->clearError();
      roles = {"Administrator", "Operator", "Betrachter"};
    }

    std::cout << "\nNeue Rolle:\n";
    for (size_t i = 0; i < roles.size(); ++i) {
      std::cout << "  [" << (i + 1) << "] " << roles[i] << "\n";
    }
    std::cout << "\n";

    const std::string prompt =
        "Rolle (1-" + std::to_string(roles.size()) + ")";
    int roleChoice = readInteger(prompt);
    if (!running_)
      return;

    if (roleChoice < 1 || static_cast<size_t>(roleChoice) > roles.size()) {
      std::cout << "\nUngültige Auswahl.\n";
      waitForEnter();
      return;
    }

    selectedRoleName = roles[static_cast<size_t>(roleChoice - 1)];
    roleChanged = true;
    break;
  }
  case 2: {
    user->setActive(!user->isActive());
    std::cout << "\nNeuer Status: " << (user->isActive() ? "Aktiv" : "Inaktiv")
              << "\n";
    break;
  }
  case 3: {
    std::string newName = readInput("Neuer Name [" + user->getFullName() + "]");
    if (!running_)
      return;
    if (!isEmpty(newName)) {
      user->setFullName(trim(newName));
    }
    std::string newEmail = readInput("Neue E-Mail [" + user->getEmail() + "]");
    if (!running_)
      return;
    if (!isEmpty(newEmail)) {
      user->setEmail(trim(newEmail));
    }
    break;
  }
  case 4: {
    std::string newPassword = readInput("Neues Passwort");
    if (!running_)
      return;
    if (newPassword.length() < 4) {
      std::cout << "\n✗ Passwort muss mindestens 4 Zeichen haben.\n";
      waitForEnter();
      return;
    }
    user->setPassword(newPassword);
    break;
  }
  case 0:
    std::cout << "\nAbgebrochen.\n";
    waitForEnter();
    return;
  default:
    std::cout << "\nUngültige Auswahl.\n";
    waitForEnter();
    return;
  }

  if (roleChanged) {
    if (database_->assignUserRole(user->getId(), selectedRoleName,
                                  getCurrentUsername())) {
      user->setRoleName(selectedRoleName);
      std::cout << "\n✓ Rolle erfolgreich zugewiesen.\n";
    } else {
      std::cout << "\n✗ Fehler beim Zuweisen der Rolle: "
                << database_->getLastError() << "\n";
    }
  } else if (database_->updateUser(*user, getCurrentUsername())) {
    std::cout << "\n✓ Benutzer erfolgreich aktualisiert.\n";
  } else {
    std::cout << "\n✗ Fehler beim Aktualisieren: "
              << database_->getLastError() << "\n";
  }

  waitForEnter();
}

void CliInterface::handleDeleteUser() {
  clearScreen();
  printSeparator();
  std::cout << "        BENUTZER DEAKTIVIEREN\n";
  printSeparator();
  std::cout << "\n";

  if (!isAdmin()) {
    std::cout << "✗ Nur Administratoren können Benutzer deaktivieren.\n";
    waitForEnter();
    return;
  }

  int id = readInteger("Benutzer-ID (numerisch)");
  if (!running_)
    return;

  // Prüfen dass man sich nicht selbst deaktiviert
  if (currentUser_ && currentUser_->getId() == id) {
    std::cout << "\n✗ Sie können sich nicht selbst deaktivieren.\n";
    waitForEnter();
    return;
  }

  auto user = database_->getUser(id);

  if (!user) {
    std::cout << "\n✗ Benutzer nicht gefunden: " << database_->getLastError()
              << "\n";
    waitForEnter();
    return;
  }

  std::cout << "\nBenutzer:\n";
  std::cout << "  Benutzername: " << user->getUsername() << "\n";
  std::cout << "  Rolle:        " << user->getRoleString() << "\n\n";

  if (!user->isActive()) {
    std::cout << "\n✗ Benutzer ist bereits deaktiviert.\n";
    waitForEnter();
    return;
  }

  std::string confirm = readInput("Wirklich deaktivieren? (ja/nein)");
  if (!running_)
    return;

  std::string confirmLower = confirm;
  for (char &c : confirmLower) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }

  if (confirmLower == "ja" || confirmLower == "j" || confirmLower == "yes" ||
      confirmLower == "y") {
    user->setActive(false);
    if (database_->updateUser(*user, getCurrentUsername())) {
      std::cout << "\n✓ Benutzer erfolgreich deaktiviert.\n";
    } else {
      std::cout << "\n✗ Fehler beim Deaktivieren: "
                << database_->getLastError() << "\n";
    }
  } else {
    std::cout << "\nDeaktivieren abgebrochen.\n";
  }

  waitForEnter();
}

void CliInterface::handleManageRoles() {
  clearScreen();
  printSeparator();
  std::cout << "        ROLLEN VERWALTEN\n";
  printSeparator();
  std::cout << "\n";

  if (!isAdmin()) {
    std::cout << "✗ Nur Administratoren können Rollen verwalten.\n";
    waitForEnter();
    return;
  }

  auto roles = database_->getAllRoles();
  if (database_->hasError()) {
    std::cout << "✗ Fehler beim Laden der Rollen: " << database_->getLastError()
              << "\n";
    waitForEnter();
    return;
  }

  if (roles.empty()) {
    std::cout << "ℹ Keine Rollen gefunden.\n";
  } else {
    std::cout << "Verfügbare Rollen:\n";
    for (size_t i = 0; i < roles.size(); ++i) {
      std::cout << "  [" << (i + 1) << "] " << roles[i] << "\n";
    }
  }

  std::cout << "\nOptionen:\n";
  std::cout << "  [1] Neue Rolle erstellen\n";
  std::cout << "  [2] Rolle aktualisieren\n";
  std::cout << "  [0] Zurück\n\n";

  int choice = readInteger("Ihre Wahl");
  if (!running_)
    return;

  switch (choice) {
  case 1: {
    std::string roleName = readValidatedInput("Rollenname", "Rollenname");
    if (!running_)
      return;

    std::string permInput =
        readInput("Berechtigungen (kommagetrennt, optional)");
    if (!running_)
      return;

    auto permissions = splitCommaList(permInput);

    if (database_->createRole(roleName, permissions, getCurrentUsername())) {
      std::cout << "\n✓ Rolle '" << roleName << "' erstellt.\n";
    } else {
      std::cout << "\n✗ Fehler beim Erstellen: " << database_->getLastError()
                << "\n";
    }
    waitForEnter();
    return;
  }
  case 2: {
    if (roles.empty()) {
      std::cout << "\n✗ Keine Rollen verfügbar.\n";
      waitForEnter();
      return;
    }

    const std::string prompt =
        "Rolle wählen (1-" + std::to_string(roles.size()) + ")";
    int roleChoice = readInteger(prompt);
    if (!running_)
      return;

    if (roleChoice < 1 || static_cast<size_t>(roleChoice) > roles.size()) {
      std::cout << "\nUngültige Auswahl.\n";
      waitForEnter();
      return;
    }

    const std::string roleName = roles[static_cast<size_t>(roleChoice - 1)];
    auto currentPerms = database_->getRolePermissions(roleName);
    if (database_->hasError()) {
      std::cout << "\n✗ Fehler beim Laden der Berechtigungen: "
                << database_->getLastError() << "\n";
      waitForEnter();
      return;
    }

    std::cout << "\nAktuelle Berechtigungen:";
    if (currentPerms.empty()) {
      std::cout << " (keine)";
    }
    std::cout << "\n";
    for (const auto &perm : currentPerms) {
      std::cout << "  - " << perm << "\n";
    }

    std::string permInput = readInput(
        "Neue Berechtigungen (kommagetrennt, leer=beibehalten)");
    if (!running_)
      return;

    auto newPerms =
        isEmpty(permInput) ? currentPerms : splitCommaList(permInput);

    if (database_->updateRole(roleName, newPerms, getCurrentUsername())) {
      std::cout << "\n✓ Rolle '" << roleName << "' aktualisiert.\n";
    } else {
      std::cout << "\n✗ Fehler beim Aktualisieren: "
                << database_->getLastError() << "\n";
    }

    waitForEnter();
    return;
  }
  case 0:
    return;
  default:
    std::cout << "\nUngültige Auswahl.\n";
    waitForEnter();
    return;
  }
}

// Validierungsfunktionen
std::string CliInterface::trim(const std::string &str) {
  size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return ""; // String besteht nur aus Whitespace
  }

  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

std::vector<std::string> CliInterface::splitCommaList(
    const std::string &input) {
  std::vector<std::string> items;
  std::stringstream ss(input);
  std::string token;

  while (std::getline(ss, token, ',')) {
    auto trimmed = trim(token);
    if (!trimmed.empty()) {
      items.push_back(trimmed);
    }
  }

  std::sort(items.begin(), items.end());
  items.erase(std::unique(items.begin(), items.end()), items.end());
  return items;
}

bool CliInterface::isValidId(const std::string &id) {
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

bool CliInterface::isEmpty(const std::string &str) {
  return str.empty() || str.find_first_not_of(" \t\r\n") == std::string::npos;
}

bool CliInterface::parseDate(const std::string &input, std::time_t &out) {
  std::tm tm = {};
  std::istringstream ss(input);
  ss >> std::get_time(&tm, "%Y-%m-%d");
  if (ss.fail()) {
    return false;
  }
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;

  std::tm normalized = tm;
  std::time_t parsed = std::mktime(&normalized);
  if (parsed == static_cast<std::time_t>(-1)) {
    return false;
  }

  if (normalized.tm_year != tm.tm_year || normalized.tm_mon != tm.tm_mon ||
      normalized.tm_mday != tm.tm_mday) {
    return false;
  }

  out = parsed;
  return true;
}

} // namespace utils
} // namespace opensylab
