#include "core/TestResult.h"
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <unordered_map>

namespace opensylab {
namespace core {

// Konstruktoren
TestResult::TestResult()
    : id_(0), resultId_(""), orderId_(0), testParameter_(""), value_(""),
      unit_(""), referenceRange_(""), referenceLow_(0.0), referenceHigh_(0.0),
      status_(Status::PENDING), flag_(Flag::UNDEFINED), measuredDate_(0),
      measuredBy_(""), comment_("") {}

TestResult::TestResult(const std::string &resultId, int orderId,
                       const std::string &testParameter)
    : id_(0), resultId_(resultId), orderId_(orderId),
      testParameter_(testParameter), value_(""), unit_(""), referenceRange_(""),
      referenceLow_(0.0), referenceHigh_(0.0), status_(Status::PENDING),
      flag_(Flag::UNDEFINED), measuredDate_(0), measuredBy_(""), comment_("") {}

// Status-Hilfsfunktionen
std::string TestResult::getStatusString() const {
  return statusToString(status_);
}

std::string TestResult::statusToString(Status status) {
  switch (status) {
  case Status::PENDING:
    return "Ausstehend";
  case Status::ENTERED:
    return "Eingegeben";
  case Status::VALIDATED:
    return "Validiert";
  case Status::REJECTED:
    return "Abgelehnt";
  case Status::REPEATED:
    return "Wiederholung";
  default:
    return "Unbekannt";
  }
}

TestResult::Status TestResult::stringToStatus(const std::string &statusStr) {
  static const std::unordered_map<std::string, Status> statusMap = {
      {"Ausstehend", Status::PENDING},    {"PENDING", Status::PENDING},
      {"Eingegeben", Status::ENTERED},    {"ENTERED", Status::ENTERED},
      {"Validiert", Status::VALIDATED},   {"VALIDATED", Status::VALIDATED},
      {"Abgelehnt", Status::REJECTED},    {"REJECTED", Status::REJECTED},
      {"Wiederholung", Status::REPEATED}, {"REPEATED", Status::REPEATED},
  };

  auto it = statusMap.find(statusStr);
  if (it != statusMap.end()) {
    return it->second;
  }

  throw std::invalid_argument("Ungültiger Status-String: " + statusStr);
}

// Flag-Hilfsfunktionen
std::string TestResult::getFlagString() const { return flagToString(flag_); }

std::string TestResult::flagToString(Flag flag) {
  switch (flag) {
  case Flag::NORMAL:
    return "Normal";
  case Flag::LOW:
    return "Niedrig";
  case Flag::HIGH:
    return "Hoch";
  case Flag::CRITICAL:
    return "Kritisch";
  case Flag::UNDEFINED:
    return "Undefiniert";
  default:
    return "Unbekannt";
  }
}

TestResult::Flag TestResult::stringToFlag(const std::string &flagStr) {
  static const std::unordered_map<std::string, Flag> flagMap = {
      {"Normal", Flag::NORMAL},
      {"NORMAL", Flag::NORMAL},
      {"Niedrig", Flag::LOW},
      {"LOW", Flag::LOW},
      {"Hoch", Flag::HIGH},
      {"HIGH", Flag::HIGH},
      {"Kritisch", Flag::CRITICAL},
      {"CRITICAL", Flag::CRITICAL},
      {"Undefiniert", Flag::UNDEFINED},
      {"UNDEFINED", Flag::UNDEFINED},
  };

  auto it = flagMap.find(flagStr);
  if (it != flagMap.end()) {
    return it->second;
  }

  throw std::invalid_argument("Ungültiger Flag-String: " + flagStr);
}

// Plausibilitätsprüfung
bool TestResult::isNumeric() const {
  if (value_.empty()) {
    return false;
  }

  const char *str = value_.c_str();
  char *end = nullptr;
  std::strtod(str, &end);

  // Wert ist numerisch, wenn alle Zeichen verarbeitet wurden
  // (nach evtl. führenden Whitespace)
  while (*end != '\0' && std::isspace(*end)) {
    ++end;
  }

  return end != str && *end == '\0';
}

double TestResult::getNumericValue() const {
  if (!isNumeric()) {
    throw std::invalid_argument("Wert ist nicht numerisch: " + value_);
  }
  return std::strtod(value_.c_str(), nullptr);
}

TestResult::Flag TestResult::evaluateFlag() const {
  // Wenn kein Referenzbereich definiert ist
  if (referenceLow_ == 0.0 && referenceHigh_ == 0.0) {
    return Flag::UNDEFINED;
  }

  // Wenn Wert nicht numerisch ist
  if (!isNumeric()) {
    return Flag::UNDEFINED;
  }

  double numValue = getNumericValue();

  // Prüfung gegen Referenzbereich
  if (numValue < referenceLow_) {
    return Flag::LOW;
  } else if (numValue > referenceHigh_) {
    return Flag::HIGH;
  } else {
    return Flag::NORMAL;
  }
}

} // namespace core
} // namespace opensylab
