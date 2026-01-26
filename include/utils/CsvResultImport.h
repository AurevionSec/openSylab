#ifndef OPENSYLAB_CSVRESULTIMPORT_H
#define OPENSYLAB_CSVRESULTIMPORT_H

#include "core/TestResult.h"
#include "db/Database.h"
#include <memory>
#include <string>
#include <vector>

namespace opensylab {
namespace utils {

/**
 * @brief CSV-Import-Modul für Laborergebnisse von Analysegeräten
 *
 * Ermöglicht den Import von Testergebnissen aus CSV-Dateien,
 * wie sie von Laborgeräten ausgegeben werden.
 *
 * Erwartetes CSV-Format:
 * result_id,order_id,test_parameter,value,unit,reference_low,reference_high,measured_by
 */
class CsvResultImport {
public:
  struct FailedRecord {
    int recordNumber;
    std::string record;
    std::string error;
  };

  /**
   * @brief Konstruktor
   * @param database Zeiger auf Datenbank für Validierung
   */
  explicit CsvResultImport(std::shared_ptr<db::Database> database);

  /**
   * @brief Destruktor
   */
  ~CsvResultImport() = default;

  /**
   * @brief Importiert Ergebnisse aus einer CSV-Datei
   * @param filePath Pfad zur CSV-Datei
   * @return Vector mit importierten TestResult-Objekten
   */
  std::vector<core::TestResult> importResults(const std::string &filePath);

  /**
   * @brief Importiert und speichert Ergebnisse direkt in der Datenbank
   * @param filePath Pfad zur CSV-Datei
   * @return Anzahl erfolgreich gespeicherter Ergebnisse
   */
  int importAndStore(const std::string &filePath);

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
   * @brief Aktiviert/Deaktiviert Auftrags-Validierung
   * @param validate true wenn Order-Existenz geprüft werden soll
   */
  void setValidateOrders(bool validate) { validateOrders_ = validate; }

  /**
   * @brief Gibt die letzte Fehlermeldung zurück
   */
  const std::string &getLastError() const { return lastError_; }

  /**
   * @brief Gibt die Anzahl erfolgreich importierter Datensätze zurück
   */
  int getImportedCount() const { return importedCount_; }

  /**
   * @brief Gibt die Anzahl fehlgeschlagener Datensätze zurück
   */
  int getErrorCount() const { return errorCount_; }

  /**
   * @brief Gibt alle fehlgeschlagenen CSV-Records zurück
   */
  const std::vector<FailedRecord> &getFailedRecords() const {
    return failedRecords_;
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
  std::shared_ptr<db::Database> database_;
  char delimiter_;
  bool hasHeader_;
  bool validateOrders_;
  std::string lastError_;
  int importedCount_;
  int errorCount_;
  std::vector<FailedRecord> failedRecords_;

  // Hilfsfunktionen
  std::vector<std::string> parseLine(const std::string &line);
  bool processRecord(const std::string &record, int recordNumber,
                     std::vector<core::TestResult> &results);
  core::TestResult parseRecord(const std::vector<std::string> &fields);
  void addFailedRecord(int recordNumber, const std::string &record,
                       const std::string &error);
  bool validateOrderExists(int orderId);
  void setError(const std::string &error);
  static std::string trim(const std::string &str);
};

} // namespace utils
} // namespace opensylab

#endif // OPENSYLAB_CSVRESULTIMPORT_H
