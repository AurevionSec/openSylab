#ifndef OPENSYLAB_CSVIMPORT_H
#define OPENSYLAB_CSVIMPORT_H

#include <string>
#include <vector>
#include "core/Sample.h"

namespace opensylab {
namespace utils {

/**
 * @brief CSV-Import-Modul für Proben- und Ergebnisdaten
 *
 * Ermöglicht den Import von Laborproben und Ergebnissen
 * aus CSV-Dateien in das OpenSylab-System.
 */
class CsvImport {
public:
    /**
     * @brief Konstruktor
     */
    CsvImport();

    /**
     * @brief Destruktor
     */
    ~CsvImport() = default;

    /**
     * @brief Importiert Proben aus einer CSV-Datei
     * @param filePath Pfad zur CSV-Datei
     * @return Vector mit importierten Sample-Objekten
     */
    std::vector<core::Sample> importSamples(const std::string& filePath);

    /**
     * @brief Setzt das Trennzeichen für CSV-Parsing
     * @param delimiter Trennzeichen (Standard: ',')
     */
    void setDelimiter(char delimiter) { delimiter_ = delimiter; }

    /**
     * @brief Aktiviert/Deaktiviert Header-Zeile
     * @param hasHeader true wenn erste Zeile Header ist
     */
    void setHasHeader(bool hasHeader) { hasHeader_ = hasHeader; }

    /**
     * @brief Gibt die letzte Fehlermeldung zurück
     */
    std::string getLastError() const { return lastError_; }

    /**
     * @brief Gibt die Anzahl erfolgreich importierter Datensätze zurück
     */
    int getImportedCount() const { return importedCount_; }

private:
    char delimiter_;
    bool hasHeader_;
    std::string lastError_;
    int importedCount_;

    // Hilfsfunktionen
    std::vector<std::string> parseLine(const std::string& line);
    core::Sample parseRecord(const std::vector<std::string>& fields);
    void setError(const std::string& error);
};

} // namespace utils
} // namespace opensylab

#endif // OPENSYLAB_CSVIMPORT_H
