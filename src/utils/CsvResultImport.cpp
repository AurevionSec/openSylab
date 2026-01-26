#include "utils/CsvResultImport.h"
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

namespace opensylab {
namespace utils {

CsvResultImport::CsvResultImport(std::shared_ptr<db::Database> database)
    : database_(database), delimiter_(','), hasHeader_(true),
      validateOrders_(true), lastError_(""), importedCount_(0), errorCount_(0) {
}

std::vector<core::TestResult>
CsvResultImport::importResults(const std::string &filePath) {
  std::vector<core::TestResult> results;
  importedCount_ = 0;
  errorCount_ = 0;
  failedRecords_.clear();
  lastError_.clear();

  std::ifstream file(filePath);
  if (!file.is_open()) {
    setError("Kann Datei nicht öffnen: " + filePath);
    return results;
  }

  std::string line;
  int lineNumber = 0;
  bool inQuotedField = false;
  std::string currentRecord;

  // Header überspringen wenn aktiviert
  if (hasHeader_ && std::getline(file, line)) {
    std::cout << "Header: " << line << "\n";
  }

  while (std::getline(file, line)) {
    lineNumber++;

    // Multiline-Handling für zitierte Felder
    if (inQuotedField) {
      currentRecord += "\n" + line;
    } else {
      currentRecord = line;
    }

    // Prüfen ob wir noch in einem zitierten Feld sind
    int quoteCount = 0;
    for (char c : currentRecord) {
      if (c == '"') {
        quoteCount++;
      }
    }
    inQuotedField = (quoteCount % 2 != 0);

    if (inQuotedField) {
      continue; // Mehr Zeilen sammeln
    }

    // Record verarbeiten
    if (!currentRecord.empty()) {
      if (processRecord(currentRecord, lineNumber, results)) {
        importedCount_++;
      } else {
        errorCount_++;
      }
    }
  }

  // Prüfen auf unvollständige zitierte Felder
  if (inQuotedField) {
    setError("Datei endet mit nicht geschlossenem Anführungszeichen");
    errorCount_++;
  }

  file.close();

  // Zusammenfassung ausgeben
  if (errorCount_ > 0) {
    std::cout << "\nImport-Zusammenfassung:\n";
    std::cout << "  ✓ Erfolgreich: " << importedCount_ << "\n";
    std::cout << "  ✗ Fehler: " << errorCount_ << "\n";
  }

  if (importedCount_ == 0 && errorCount_ > 0) {
    setError("Keine Ergebnisse importiert - alle Zeilen enthielten Fehler");
  } else if (importedCount_ > 0) {
    std::cout << "\n✓ CSV-Import erfolgreich: " << importedCount_
              << " Ergebnisse importiert\n";
  } else {
    setError("Keine Ergebnisse importiert - Datei enthielt keine "
             "verwertbaren Zeilen");
  }

  return results;
}

int CsvResultImport::importAndStore(const std::string &filePath) {
  auto results = importResults(filePath);

  int stored = 0;
  for (const auto &result : results) {
    if (database_->createTestResult(result)) {
      stored++;
    } else {
      std::cout << "✗ Fehler beim Speichern von " << result.getResultId()
                << ": " << database_->getLastError() << "\n";
    }
  }

  return stored;
}

bool CsvResultImport::processRecord(const std::string &record, int recordNumber,
                                    std::vector<core::TestResult> &results) {
  auto fields = parseLine(record);

  // Mindestens 4 Felder erforderlich: result_id, order_id, test_parameter,
  // value
  if (fields.size() < 4) {
    const std::string error =
        "Zu wenig Felder (erwartet mindestens 4)";
    std::cout << "✗ Fehler Record " << recordNumber << ": " << error << "\n";
    addFailedRecord(recordNumber, record, error);
    return false;
  }

  // result_id prüfen (Pflichtfeld)
  std::string resultId = trim(fields[0]);
  if (resultId.empty()) {
    const std::string error = "result_id ist ein Pflichtfeld";
    std::cout << "✗ Fehler Record " << recordNumber << ": " << error << "\n";
    addFailedRecord(recordNumber, record, error);
    return false;
  }

  // order_id prüfen (Pflichtfeld, muss numerisch sein)
  std::string orderIdStr = trim(fields[1]);
  int orderId = 0;
  try {
    orderId = std::stoi(orderIdStr);
  } catch (...) {
    const std::string error = "order_id muss numerisch sein";
    std::cout << "✗ Fehler Record " << recordNumber << ": " << error << "\n";
    addFailedRecord(recordNumber, record, error);
    return false;
  }

  // Auftrags-Validierung
  if (validateOrders_ && !validateOrderExists(orderId)) {
    const std::string error = "Auftrag " + std::to_string(orderId) +
                              " existiert nicht";
    std::cout << "✗ Fehler Record " << recordNumber << ": " << error << "\n";
    addFailedRecord(recordNumber, record, error);
    return false;
  }

  // test_parameter prüfen (Pflichtfeld)
  std::string testParameter = trim(fields[2]);
  if (testParameter.empty()) {
    const std::string error = "test_parameter ist ein Pflichtfeld";
    std::cout << "✗ Fehler Record " << recordNumber << ": " << error << "\n";
    addFailedRecord(recordNumber, record, error);
    return false;
  }

  // value prüfen (Pflichtfeld)
  std::string value = trim(fields[3]);
  if (value.empty()) {
    const std::string error = "value ist ein Pflichtfeld";
    std::cout << "✗ Fehler Record " << recordNumber << ": " << error << "\n";
    addFailedRecord(recordNumber, record, error);
    return false;
  }

  // TestResult erstellen
  core::TestResult result(resultId, orderId, testParameter);
  result.setValue(value);
  result.setStatus(core::TestResult::Status::ENTERED);
  result.setMeasuredDate(std::time(nullptr));

  // Optionale Felder
  if (fields.size() > 4) {
    result.setUnit(trim(fields[4]));
  }

  if (fields.size() > 5) {
    try {
      result.setReferenceLow(std::stod(trim(fields[5])));
    } catch (...) {
      result.setReferenceLow(0.0);
    }
  }

  if (fields.size() > 6) {
    try {
      result.setReferenceHigh(std::stod(trim(fields[6])));
    } catch (...) {
      result.setReferenceHigh(0.0);
    }
  }

  if (fields.size() > 7) {
    result.setMeasuredBy(trim(fields[7]));
  }

  // Referenzbereich-String erstellen
  if (result.getReferenceLow() > 0 || result.getReferenceHigh() > 0) {
    std::ostringstream oss;
    oss << result.getReferenceLow() << "-" << result.getReferenceHigh();
    if (!result.getUnit().empty()) {
      oss << " " << result.getUnit();
    }
    result.setReferenceRange(oss.str());
  }

  // Flag automatisch berechnen
  result.setFlag(result.evaluateFlag());

  results.push_back(result);
  return true;
}

std::vector<std::string> CsvResultImport::parseLine(const std::string &line) {
  std::vector<std::string> fields;
  std::string currentField;
  bool inQuotes = false;
  bool prevWasQuote = false;

  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];

    if (c == '"') {
      if (inQuotes && prevWasQuote) {
        // Escaped quote ("") -> ein Anführungszeichen
        currentField += '"';
        prevWasQuote = false;
      } else if (inQuotes) {
        prevWasQuote = true;
      } else {
        inQuotes = true;
        prevWasQuote = false;
      }
    } else if (c == delimiter_ && !inQuotes) {
      if (prevWasQuote) {
        inQuotes = false;
        prevWasQuote = false;
      }
      fields.push_back(currentField);
      currentField.clear();
    } else {
      if (prevWasQuote) {
        inQuotes = false;
        prevWasQuote = false;
      }
      currentField += c;
    }
  }

  // Letztes Feld hinzufügen
  fields.push_back(currentField);

  return fields;
}

bool CsvResultImport::validateOrderExists(int orderId) {
  if (!database_) {
    return true; // Ohne Datenbank keine Validierung möglich
  }

  auto order = database_->getOrder(orderId);
  database_->clearError(); // Fehler löschen falls Auftrag nicht gefunden
  return order != nullptr;
}

void CsvResultImport::addFailedRecord(int recordNumber,
                                      const std::string &record,
                                      const std::string &error) {
  failedRecords_.push_back({recordNumber, record, error});
}

bool CsvResultImport::writeRetryCsv(const std::string &filePath) const {
  return writeRetryCsv(filePath, {});
}

bool CsvResultImport::writeRetryCsv(
    const std::string &filePath,
    const std::vector<FailedRecord> &extraFailed) const {
  if (failedRecords_.empty() && extraFailed.empty()) {
    return false;
  }

  std::ofstream file(filePath);
  if (!file.is_open()) {
    return false;
  }

  file << "result_id,order_id,test_parameter,value,unit,ref_low,ref_high,"
          "measured_by\n";

  for (const auto &failed : failedRecords_) {
    file << failed.record << "\n";
  }
  for (const auto &failed : extraFailed) {
    file << failed.record << "\n";
  }

  return true;
}

void CsvResultImport::setError(const std::string &error) {
  lastError_ = error;
  std::cerr << "CSV-Import-Fehler: " << error << std::endl;
}

std::string CsvResultImport::trim(const std::string &str) {
  size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

} // namespace utils
} // namespace opensylab
