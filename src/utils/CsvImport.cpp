#include "utils/CsvImport.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
std::string trim(const std::string &value) {
  const size_t start = value.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  const size_t end = value.find_last_not_of(" \t\r\n");
  return value.substr(start, end - start + 1);
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
    std::cerr << "\nImport-Zusammenfassung:\n";
    std::cerr << "  ✓ Erfolgreich: " << importedCount << "\n";
    std::cerr << "  ✗ Fehler: " << errorCount << "\n";
  }
}
} // namespace

bool CsvImport::processRecord(const std::string &record, int recordNumber,
                              std::vector<core::Sample> &samples) {
  std::vector<std::string> fields = parseLine(record);

  if (fields.size() < 2) {
    std::cerr << "✗ Fehler Record " << recordNumber
              << ": Zu wenig Felder (erwartet mindestens 2)\n";
    return false;
  }

  try {
    core::Sample sample = parseRecord(fields);
    samples.push_back(sample);
    importedCount_++;
    return true;
  } catch (const std::invalid_argument &e) {
    std::cerr << "✗ Fehler Record " << recordNumber << ": " << e.what() << "\n";
  } catch (const std::exception &e) {
    std::cerr << "✗ Unerwarteter Fehler Record " << recordNumber << ": "
              << e.what() << "\n";
  }
  return false;
}

std::vector<core::Sample>
CsvImport::importSamples(const std::string &filePath) {
  std::vector<core::Sample> samples;
  importedCount_ = 0;
  lastError_ = "";

  std::ifstream file(filePath);
  if (!file.is_open()) {
    setError("Kann Datei nicht öffnen: " + filePath);
    return samples;
  }

  int recordNumber = 0;
  int errorCount = 0;

  // Header-Zeile überspringen wenn vorhanden
  if (hasHeader_) {
    std::string headerLine;
    if (std::getline(file, headerLine)) {
      std::cout << "Header: " << headerLine << std::endl;
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
      recordNumber++;
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
    std::cerr
        << "✗ Fehler: Datei endet mit nicht geschlossenem Anführungszeichen\n";
  }

  // Letzten Record verarbeiten (falls Datei nicht mit Newline endet)
  if (!isEmptyRecord(record)) {
    recordNumber++;
    if (!processRecord(record, recordNumber, samples)) {
      errorCount++;
    }
  }

  printErrorSummary(importedCount_, errorCount);
  file.close();

  if (importedCount_ > 0) {
    std::cout << "\n✓ CSV-Import erfolgreich: " << importedCount_
              << " Proben importiert\n";
  } else {
    setError(errorCount > 0
                 ? "Keine Proben importiert - alle Zeilen enthielten Fehler"
                 : "Keine Proben importiert - Datei enthielt keine "
                   "verwertbaren Zeilen");
  }

  return samples;
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

  sample.setSampleId(requireField(0, "sample_id"));
  sample.setPatientId(requireField(1, "patient_id"));

  // Optionale Felder
  if (fields.size() > 2) {
    sample.setPatientName(fields[2]);
  }

  if (fields.size() > 3) {
    sample.setDescription(fields[3]);
  }

  if (fields.size() > 4 && !fields[4].empty()) {
    try {
      sample.setStatus(core::Sample::stringToStatus(fields[4]));
    } catch (const std::exception &e) {
      std::cerr << "Warnung: Ungültiger Status '" << fields[4]
                << "', verwende Standard 'Erfasst'" << std::endl;
      sample.setStatus(core::Sample::Status::REGISTERED);
    }
  } else {
    sample.setStatus(core::Sample::Status::REGISTERED);
  }

  return sample;
}

void CsvImport::setError(const std::string &error) {
  lastError_ = error;
  std::cerr << "CSV-Import-Fehler: " << error << std::endl;
}

} // namespace utils
} // namespace opensylab
