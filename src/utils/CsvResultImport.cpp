#include "utils/CsvResultImport.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
constexpr size_t kMaxImportBytes = 10 * 1024 * 1024;

std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}
} // namespace

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
  importedRecords_.clear();
  failedRecords_.clear();
  lastError_.clear();
  headerLine_.clear();

  std::error_code ec;
  const auto size = std::filesystem::file_size(filePath, ec);
  if (!ec && size > kMaxImportBytes) {
    setError("CSV-Datei zu groß für Import");
    return results;
  }

  std::ifstream file(filePath);
  if (!file.is_open()) {
    setError("Kann Datei nicht öffnen: " + filePath);
    return results;
  }

  std::string line;
  int lineNumber = hasHeader_ ? 1 : 0;
  bool inQuotedField = false;
  std::string currentRecord;

  // Header überspringen wenn aktiviert
  if (hasHeader_ && std::getline(file, line)) {
    headerLine_ = line;
    if (!validateHeader(headerLine_)) {
      setError("Ungueltiger CSV-Header. Erwartet: result_id,order_id,"
               "test_parameter,value,unit,ref_low,ref_high,measured_by");
      errorCount_++;
      return results;
    }
  }

  while (std::getline(file, line)) {
    lineNumber++;

    // Multiline-Handling für zitierte Felder
    if (inQuotedField) {
      currentRecord += "\n" + line;
    } else {
      currentRecord = line;
    }

    // Track whether accumulated record ends inside an open quoted field.
    // State machine handles escaped double-quotes ("") correctly.
    {
      bool inQ = false;
      for (size_t ci = 0; ci < currentRecord.size(); ++ci) {
        if (inQ) {
          if (currentRecord[ci] == '"') {
            if (ci + 1 < currentRecord.size() && currentRecord[ci + 1] == '"') {
              ++ci; // skip escaped double-quote
            } else {
              inQ = false; // closing quote
            }
          }
        } else if (currentRecord[ci] == '"') {
          inQ = true; // opening quote
        }
      }
      inQuotedField = inQ;
    }

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
    std::cerr << "\nImport-Zusammenfassung:\n";
    std::cerr << "  ✓ Erfolgreich: " << importedCount_ << "\n";
    std::cerr << "  ✗ Fehler: " << errorCount_ << "\n";
  }

  if (importedCount_ == 0 && errorCount_ > 0) {
    setError("Keine Ergebnisse importiert - alle Zeilen enthielten Fehler");
  } else if (importedCount_ > 0) {
    std::cerr << "\n✓ CSV-Import erfolgreich: " << importedCount_
              << " Ergebnisse importiert\n";
  } else {
    setError("Keine Ergebnisse importiert - Datei enthielt keine "
             "verwertbaren Zeilen");
  }

  return results;
}

int CsvResultImport::importAndStore(const std::string &filePath) {
  auto results = importResults(filePath);

  auto batch = database_->createTestResultsBatch(results);
  for (const auto &failure : batch.failures) {
    if (failure.index >= results.size()) {
      continue;
    }
    std::cerr << "✗ Fehler beim Speichern von "
              << results[failure.index].getResultId() << ": "
              << failure.message << "\n";
  }

  return static_cast<int>(batch.inserted);
}

bool CsvResultImport::processRecord(const std::string &record, int recordNumber,
                                    std::vector<core::TestResult> &results) {
  auto fields = parseLine(record);

  // Mindestens 4 Felder erforderlich: result_id, order_id, test_parameter,
  // value
  if (fields.size() < 4) {
    const std::string error =
        "Zu wenig Felder (erwartet mindestens 4)";
    std::cerr << "✗ Fehler Record " << recordNumber << ": " << error << "\n";
    addFailedRecord(recordNumber, record, error);
    return false;
  }

  // result_id prüfen (Pflichtfeld)
  std::string resultId = trim(fields[0]);
  if (resultId.empty()) {
    const std::string error = "result_id ist ein Pflichtfeld";
    std::cerr << "✗ Fehler Record " << recordNumber << ": " << error << "\n";
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
    std::cerr << "✗ Fehler Record " << recordNumber << ": " << error << "\n";
    addFailedRecord(recordNumber, record, error);
    return false;
  }

  // Auftrags-Validierung
  if (validateOrders_ && !validateOrderExists(orderId)) {
    const std::string error = "Auftrag " + std::to_string(orderId) +
                              " existiert nicht";
    std::cerr << "✗ Fehler Record " << recordNumber << ": " << error << "\n";
    addFailedRecord(recordNumber, record, error);
    return false;
  }

  // test_parameter prüfen (Pflichtfeld)
  std::string testParameter = trim(fields[2]);
  if (testParameter.empty()) {
    const std::string error = "test_parameter ist ein Pflichtfeld";
    std::cerr << "✗ Fehler Record " << recordNumber << ": " << error << "\n";
    addFailedRecord(recordNumber, record, error);
    return false;
  }

  // value prüfen (Pflichtfeld)
  std::string value = trim(fields[3]);
  if (value.empty()) {
    const std::string error = "value ist ein Pflichtfeld";
    std::cerr << "✗ Fehler Record " << recordNumber << ": " << error << "\n";
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
  importedRecords_.push_back({result, recordNumber, record});
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

  if (hasHeader_ && !headerLine_.empty()) {
    file << headerLine_ << "\n";
  } else {
    file << "result_id,order_id,test_parameter,value,unit,ref_low,ref_high,"
            "measured_by\n";
  }

  for (const auto &failed : failedRecords_) {
    file << failed.record << "\n";
  }
  for (const auto &failed : extraFailed) {
    file << failed.record << "\n";
  }

  file.close();
  if (!file) {
    std::remove(filePath.c_str());
    return false;
  }
  return true;
}

bool CsvResultImport::validateHeader(const std::string &header) {
  const std::vector<std::string> expected = {
      "result_id", "order_id", "test_parameter", "value",
      "unit",      "ref_low",  "ref_high",       "measured_by"};
  auto fields = parseLine(header);
  if (fields.size() < 4 || fields.size() > expected.size()) {
    return false;
  }
  if (!fields.empty()) {
    const std::string bom = "\xEF\xBB\xBF";
    if (fields[0].rfind(bom, 0) == 0) {
      fields[0] = fields[0].substr(bom.size());
    }
  }
  for (size_t i = 0; i < fields.size(); ++i) {
    if (toLower(trim(fields[i])) != expected[i]) {
      return false;
    }
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
