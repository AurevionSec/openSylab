#ifndef OPENSYLAB_CLIINTERFACE_H
#define OPENSYLAB_CLIINTERFACE_H

#include "core/User.h"
#include "db/Database.h"
#include <memory>
#include <string>

namespace opensylab {
namespace utils {

/**
 * @brief Command-Line Interface für OpenSylab
 *
 * Bietet eine interaktive Kommandozeilenschnittstelle
 * zur Verwaltung von Laborproben und Systemfunktionen.
 */
class CliInterface {
public:
  /**
   * @brief Konstruktor
   * @param database Zeiger auf Datenbank-Instanz
   */
  explicit CliInterface(std::shared_ptr<db::Database> database);

  /**
   * @brief Destruktor
   */
  ~CliInterface() = default;

  /**
   * @brief Startet die Hauptschleife der CLI
   */
  void run();

  /**
   * @brief Zeigt Willkommensnachricht
   */
  void showWelcome();

  /**
   * @brief Zeigt Hauptmenü
   */
  void showMainMenu();

private:
  std::shared_ptr<db::Database> database_;
  bool running_;
  std::unique_ptr<core::User> currentUser_;

  // Sample-Menü-Aktionen
  void handleNewSample();
  void handleListSamples();
  void handleSearchSample();
  void handleUpdateSample();
  void handleDeleteSample();
  void handleImportCsv();
  void handleStatistics();
  void handleExit();

  // Order-Menü-Aktionen
  void handleNewOrder();
  void handleListOrders();
  void handleSearchOrder();
  void handleUpdateOrder();
  void handleDeleteOrder();
  void handleOrdersForSample();

  // TestResult-Menü-Aktionen
  void handleNewResult();
  void handleListResults();
  void handleSearchResult();
  void handleUpdateResult();
  void handleValidateResult();
  void handleResultsForOrder();
  void handleImportResultsCsv();
  void handleExportResults();

  // Audit-Menü-Aktionen
  void handleShowAuditLog();
  void handleAuditForEntity();

  // User-Menü-Aktionen (Authentifizierung)
  void handleLogin();
  void handleLogout();
  void handleCreateUser();
  void handleListUsers();
  void handleUpdateUser();
  void handleDeleteUser();
  void handleChangePassword();

  // Berechtigungsprüfung
  bool isLoggedIn() const { return currentUser_ != nullptr; }
  bool isAdmin() const {
    return currentUser_ && currentUser_->getRole() == core::User::Role::ADMIN;
  }
  bool canEdit() const {
    return currentUser_ &&
           (currentUser_->getRole() == core::User::Role::ADMIN ||
            currentUser_->getRole() == core::User::Role::OPERATOR);
  }
  std::string getCurrentUsername() const {
    return currentUser_ ? currentUser_->getUsername() : "nicht angemeldet";
  }

  // Hilfsfunktionen
  std::string readInput(const std::string &prompt);
  std::string readValidatedInput(const std::string &prompt,
                                 const std::string &fieldName);
  int readInteger(const std::string &prompt);
  void clearScreen();
  void waitForEnter();
  void printSeparator();

  // Validierungsfunktionen
  static std::string trim(const std::string &str);
  static bool isValidId(const std::string &id);
  static bool isEmpty(const std::string &str);
  static bool parseDate(const std::string &input, std::time_t &out);
};

} // namespace utils
} // namespace opensylab

#endif // OPENSYLAB_CLIINTERFACE_H
