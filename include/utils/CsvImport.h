#ifndef OPENSYLAB_CSVIMPORT_H
#define OPENSYLAB_CSVIMPORT_H

#include "core/Sample.h"
#include <string>
#include <vector>

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
  struct FailedRecord {
    int recordNumber;
    std::string record;
    std::string error;
  };

  struct ImportedRecord {
    core::Sample sample;
    int recordNumber;
    std::string record;
  };

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
  std::vector<core::Sample> importSamples(const std::string &filePath);

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
  const std::string &getLastError() const { return lastError_; }

  /**
   * @brief Gibt die Anzahl erfolgreich importierter Datensätze zurück
   */
  int getImportedCount() const { return importedCount_; }

  /**
   * @brief Gibt alle fehlgeschlagenen CSV-Records zurück
   */
  const std::vector<FailedRecord> &getFailedRecords() const {
    return failedRecords_;
  }

  /**
   * @brief Gibt alle erfolgreich importierten Records mit Metadaten zurück
   */
  const std::vector<ImportedRecord> &getImportedRecords() const {
    return importedRecords_;
  }

  /**
   * @brief Gibt die Anzahl fehlgeschlagener Records zurück
   */
  int getFailedCount() const {
    return static_cast<int>(failedRecords_.size());
  }

  /**
   * @brief Schreibt fehlgeschlagene Records in eine Retry-CSV
   * @param filePath Zielpfad für Retry-Datei
   * @return true wenn Datei geschrieben wurde
   */
  bool writeRetryCsv(const std::string &filePath) const;
  bool writeRetryCsv(const std::string &filePath,
                     const std::vector<FailedRecord> &extraFailed) const;

private:
  char delimiter_;
  bool hasHeader_;
  std::string lastError_;
  int importedCount_;
  std::vector<ImportedRecord> importedRecords_;
  std::vector<FailedRecord> failedRecords_;
  std::string headerLine_;

  // Hilfsfunktionen
  std::vector<std::string> parseLine(const std::string &line);
  core::Sample parseRecord(const std::vector<std::string> &fields);
  bool processRecord(const std::string &record, int recordNumber,
                     std::vector<core::Sample> &samples);
  void addFailedRecord(int recordNumber, const std::string &record,
                       const std::string &error);
  void setError(const std::string &error);
};

} // namespace utils
} // namespace opensylab

#endif // OPENSYLAB_CSVIMPORT_H
