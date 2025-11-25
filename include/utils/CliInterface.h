#ifndef OPENSYLAB_CLIINTERFACE_H
#define OPENSYLAB_CLIINTERFACE_H

#include <string>
#include <memory>
#include "db/Database.h"

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

    // Menü-Aktionen
    void handleNewSample();
    void handleListSamples();
    void handleSearchSample();
    void handleUpdateSample();
    void handleDeleteSample();
    void handleImportCsv();
    void handleStatistics();
    void handleExit();

    // Hilfsfunktionen
    std::string readInput(const std::string& prompt);
    std::string readValidatedInput(const std::string& prompt, const std::string& fieldName);
    int readInteger(const std::string& prompt);
    void clearScreen();
    void waitForEnter();
    void printSeparator();

    // Validierungsfunktionen
    static std::string trim(const std::string& str);
    static bool isValidId(const std::string& id);
    static bool isEmpty(const std::string& str);
};

} // namespace utils
} // namespace opensylab

#endif // OPENSYLAB_CLIINTERFACE_H
