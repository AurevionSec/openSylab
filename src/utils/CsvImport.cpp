#include "utils/CsvImport.h"
#include "utils/Logger.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>

namespace {
constexpr size_t kMaxImportBytes = 10 * 1024 * 1024;
// 24-hour tolerance for registration_date validation (accommodates clock skew)
constexpr std::time_t kMaxFutureDateTolerance = 86400;

std::string trim(const std::string &value) {
  const size_t start = value.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  const size_t end = value.find_last_not_of(" \t\r\n");
  return value.substr(start, end - start + 1);
}

std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

bool isWhitespaceOnly(const std::string &value) {
  return value.find_first_not_of(" \t\r\n") == std::string::npos;
}
} // namespace

namespace opensylab {
namespace utils {

CsvImport::CsvImport()
    : delimiter_(','), hasHeader_(true), lastError_(""), importedCount_(0) {}

namespace {
bool isEmptyRecord(const std::string &record) {
  return record.empty() ||
         record.find_first_not_of(" \t\r\n") == std::string::npos;
}

void printErrorSummary(int importedCount, int errorCount) {
  if (errorCount > 0) {
    LOG_WARN("Import-Zusammenfassung: Erfolgreich={}, Fehler={}", importedCount, errorCount);
  }
}
} // namespace

bool CsvImport::processRecord(const std::string &record, int recordNumber,
                              std::vector<core::Sample> &samples) {
  std::vector<std::string> fields = parseLine(record);

  if (fields.size() < 2) {
    const std::string error =
        "Zu wenig Felder (erwartet mindestens 2)";
    LOG_ERROR("Fehler Record {}: {}", recordNumber, error);
    addFailedRecord(recordNumber, record, error);
    return false;
  }

  try {
    core::Sample sample = parseRecord(fields);
    samples.push_back(sample);
    importedCount_++;
    importedRecords_.push_back({sample, recordNumber, record});
    return true;
  } catch (const std::invalid_argument &e) {
    LOG_ERROR("Fehler Record {}: {}", recordNumber, e.what());
    addFailedRecord(recordNumber, record, e.what());
  } catch (const std::exception &e) {
    LOG_ERROR("Unerwarteter Fehler Record {}: {}", recordNumber, e.what());
    addFailedRecord(recordNumber, record, e.what());
  }
  return false;
}

std::vector<core::Sample>
CsvImport::importSamples(const std::string &filePath) {
  std::vector<core::Sample> samples;
  importedCount_ = 0;
  lastError_ = "";
  importedRecords_.clear();
  failedRecords_.clear();
  headerLine_.clear();

  std::ifstream file(filePath);
  if (!file.is_open()) {
    setError("Kann Datei nicht öffnen: " + filePath);
    return samples;
  }
  file.seekg(0, std::ios::end);
  const auto size = file.tellg();
  file.seekg(0, std::ios::beg);
  if (size < 0 || static_cast<std::uintmax_t>(size) > kMaxImportBytes) {
    setError("CSV-Datei zu groß für Import");
    return samples;
  }

  int recordNumber = 0;
  int lineNumber = hasHeader_ ? 1 : 0;
  int errorCount = 0;

  // Header-Zeile überspringen wenn vorhanden
  if (hasHeader_) {
    std::string headerLine;
    if (std::getline(file, headerLine)) {
      headerLine_ = headerLine;
      LOG_DEBUG("Header: {}", headerLine);
      if (!validateHeader(headerLine_)) {
        setError("Ungueltiger CSV-Header. Erwartet: sample_id,patient_id,"
                 "patient_name,description,status");
        return samples;
      }
    }
  }

  // CSV-Records einlesen (mit Multiline-Support)
  std::string record;
  bool inQuotes = false;
  char c;

  while (file.get(c)) {
    if (c == '"') {
      inQuotes = !inQuotes;
      record += c;
    } else if (c == '\n' && !inQuotes) {
      lineNumber++;
      recordNumber = lineNumber;
      if (!isEmptyRecord(record)) {
        if (!processRecord(record, recordNumber, samples)) {
          errorCount++;
        }
      }
      record.clear();
    } else {
      record += c;
    }
  }

  // Prüfen auf nicht geschlossene Anführungszeichen
  if (inQuotes) {
    errorCount++;
    LOG_ERROR("Fehler: Datei endet mit nicht geschlossenem Anführungszeichen");
  }

  // Letzten Record verarbeiten (falls Datei nicht mit Newline endet)
  if (!isEmptyRecord(record)) {
    recordNumber = lineNumber + 1;
    if (!processRecord(record, recordNumber, samples)) {
      errorCount++;
    }
  }

  printErrorSummary(importedCount_, errorCount);
  file.close();

  if (importedCount_ > 0) {
    LOG_INFO("CSV-Import erfolgreich: {} Proben importiert", importedCount_);
  } else {
    setError(errorCount > 0
                 ? "Keine Proben importiert - alle Zeilen enthielten Fehler"
                 : "Keine Proben importiert - Datei enthielt keine "
                   "verwertbaren Zeilen");
  }

  return samples;
}

bool CsvImport::validateHeader(const std::string &header) {
  const std::vector<std::string> expected = {
      "sample_id", "patient_id", "patient_name", "description", "status"};
  auto fields = parseLine(header);
  if (fields.size() < 2) {
    return false;
  }
  if (!fields.empty()) {
    const std::string bom = "\xEF\xBB\xBF";
    if (fields[0].rfind(bom, 0) == 0) {
      fields[0] = fields[0].substr(bom.size());
    }
  }
  const size_t checkCount = std::min(fields.size(), expected.size());
  for (size_t i = 0; i < checkCount; ++i) {
    if (toLower(trim(fields[i])) != expected[i]) {
      return false;
    }
  }
  return true;
}

bool CsvImport::writeRetryCsv(const std::string &filePath) const {
  return writeRetryCsv(filePath, {});
}

bool CsvImport::writeRetryCsv(
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

std::vector<std::string> CsvImport::parseLine(const std::string &line) {
  std::vector<std::string> fields;
  std::string field;
  bool inQuotes = false;

  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];

    if (c == '"') {
      // Toggle quote mode
      if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
        // Escaped quote ("") -> add single quote
        field += '"';
        ++i;
      } else {
        inQuotes = !inQuotes;
      }
    } else if (c == delimiter_ && !inQuotes) {
      // End of field
      fields.push_back(trim(field));
      field.clear();
    } else {
      field += c;
    }
  }

  // Add last field
  fields.push_back(trim(field));

  return fields;
}

core::Sample CsvImport::parseRecord(const std::vector<std::string> &fields) {
  // CSV-Format: sample_id, patient_id, patient_name, description, status
  core::Sample sample;

  auto requireField = [&](size_t index,
                          const std::string &name) -> std::string {
    if (fields.size() <= index || fields[index].empty()) {
      throw std::invalid_argument(name + " ist ein Pflichtfeld");
    }
    if (isWhitespaceOnly(fields[index])) {
      throw std::invalid_argument(name + " darf nicht leer sein");
    }
    return fields[index];
  };

  const std::string sampleId = requireField(0, "sample_id");
  if (sampleId.size() > 64) {
    throw std::invalid_argument("sample_id ueberschreitet Maximallänge von 64 Zeichen");
  }
  sample.setSampleId(sampleId);

  const std::string patientId = requireField(1, "patient_id");
  if (patientId.size() > 64) {
    throw std::invalid_argument("patient_id ueberschreitet Maximallänge von 64 Zeichen");
  }
  sample.setPatientId(patientId);

  // Optionale Felder
  if (fields.size() > 2) {
    sample.setPatientName(fields[2]);
  }

  if (fields.size() > 3) {
    sample.setDescription(fields[3]);
  }

  if (fields.size() > 4 && !fields[4].empty()) {
    if (!core::Sample::isValidStatusString(fields[4])) {
      throw std::invalid_argument("Ungueltiger Status: " + fields[4]);
    }
    sample.setStatus(core::Sample::stringToStatus(fields[4]));
  } else {
    sample.setStatus(core::Sample::Status::REGISTERED);
  }

  if (fields.size() > 5 && !fields[5].empty()) {
    bool isNumeric = !fields[5].empty() &&
                     fields[5].find_first_not_of("0123456789") == std::string::npos;
    if (isNumeric) {
      try {
        const long long ts = std::stoll(fields[5]);
        const long long maxTs =
            static_cast<long long>(std::time(nullptr)) + kMaxFutureDateTolerance;
        if (ts <= 0 || ts > maxTs) {
          throw std::invalid_argument(
              "registration_date liegt in der Zukunft oder ist ungueltig");
        }
        sample.setRegistrationDate(static_cast<std::time_t>(ts));
      } catch (const std::out_of_range &) {
        throw std::invalid_argument("registration_date ist ungueltig");
      }
    }
    // Non-numeric field at position 5 is an unknown extra column — silently ignored.
  }

  return sample;
}

void CsvImport::addFailedRecord(int recordNumber, const std::string &record,
                                const std::string &error) {
  failedRecords_.push_back({recordNumber, record, error});
}

void CsvImport::setError(const std::string &error) {
  lastError_ = error;
  LOG_ERROR("CSV-Import-Fehler: {}", error);
}

} // namespace utils
} // namespace opensylab
