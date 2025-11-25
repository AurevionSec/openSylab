#ifndef OPENSYLAB_DATABASE_H
#define OPENSYLAB_DATABASE_H

#include <string>
#include <vector>
#include <memory>
#include "core/Sample.h"

// Forward declaration für SQLite
struct sqlite3;

namespace opensylab {
namespace db {

/**
 * @brief Datenbank-Zugriffsschicht für OpenSylab
 *
 * Diese Klasse kapselt alle Datenbankoperationen und bietet
 * eine typsichere Schnittstelle für CRUD-Operationen.
 */
class Database {
public:
    /**
     * @brief Konstruktor
     * @param dbPath Pfad zur SQLite-Datenbankdatei
     */
    explicit Database(const std::string& dbPath);

    /**
     * @brief Destruktor - schließt die Datenbankverbindung
     */
    ~Database();

    // Datenbankoperationen
    bool open();
    bool close();
    bool isOpen() const { return isOpen_; }

    /**
     * @brief Initialisiert das Datenbankschema
     * @return true bei Erfolg
     */
    bool initializeSchema();

    // Sample-Operationen (CRUD)
    bool createSample(const core::Sample& sample);
    std::unique_ptr<core::Sample> getSample(int id);
    std::unique_ptr<core::Sample> getSampleByBarcode(const std::string& barcode);
    std::vector<std::unique_ptr<core::Sample>> getAllSamples();
    bool updateSample(const core::Sample& sample);
    bool deleteSample(int id);

    // Fehlerbehandlung
    std::string getLastError() const { return lastError_; }
    bool hasError() const { return !lastError_.empty(); }
    void clearError() { lastError_.clear(); }

private:
    std::string dbPath_;
    sqlite3* db_;
    bool isOpen_;
    std::string lastError_;

    // Hilfsfunktionen
    void setError(const std::string& error);
};

} // namespace db
} // namespace opensylab

#endif // OPENSYLAB_DATABASE_H
