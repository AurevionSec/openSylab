#include "core/TestResult.h"
#include <cctype>
#include <cmath>
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
    return "PENDING";
  case Status::ENTERED:
    return "ENTERED";
  case Status::VALIDATED:
    return "VALIDATED";
  case Status::REJECTED:
    return "REJECTED";
  case Status::REPEATED:
    return "REPEATED";
  default:
    return "UNKNOWN";
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

  return Status::PENDING;
}

// Flag-Hilfsfunktionen
std::string TestResult::getFlagString() const { return flagToString(flag_); }

std::string TestResult::flagToString(Flag flag) {
  switch (flag) {
  case Flag::NORMAL:
    return "NORMAL";
  case Flag::LOW:
    return "LOW";
  case Flag::HIGH:
    return "HIGH";
  case Flag::CRITICAL:
    return "CRITICAL";
  case Flag::UNDEFINED:
    return "UNDEFINED";
  default:
    return "UNKNOWN";
  }
}

TestResult::Flag TestResult::stringToFlag(const std::string &flagStr) {
  static const std::unordered_map<std::string, Flag> flagMap = {
      {"Normal", Flag::NORMAL},
      {"NORMAL", Flag::NORMAL},
      {"Niedrig", Flag::LOW},
      {"LOW", Flag::LOW},
      {"Hoch", Flag::HIGH},
      {"Erhöht", Flag::HIGH},  // Alternative deutsche Bezeichnung
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

  return Flag::UNDEFINED;
}

// Plausibilitätsprüfung
bool TestResult::isNumeric() const {
  if (value_.empty()) {
    return false;
  }

  const char *str = value_.c_str();
  char *end = nullptr;
  const double parsed = std::strtod(str, &end);

  // Wert ist numerisch, wenn alle Zeichen verarbeitet wurden
  // (nach evtl. führenden Whitespace)
  while (*end != '\0' && std::isspace(*end)) {
    ++end;
  }

  // strtod also parses "nan"/"inf"; reject non-finite values — a NaN result
  // would otherwise pass every comparison in evaluateFlag() and be flagged
  // NORMAL.
  return end != str && *end == '\0' && std::isfinite(parsed);
}

double TestResult::getNumericValue() const {
  if (!isNumeric()) {
    throw std::invalid_argument("Wert ist nicht numerisch: " + value_);
  }
  return std::strtod(value_.c_str(), nullptr);
}

TestResult::Flag TestResult::evaluateFlag() const {
  if (!isNumeric()) {
    return Flag::UNDEFINED;
  }

  // Referenzwerte unvollständig oder ungueltig
  if (referenceHigh_ <= referenceLow_) {
    return Flag::UNDEFINED;
  }

  double numValue = getNumericValue();
  // Use absolute margin for critical bounds so negative reference values work correctly
  double margin = referenceHigh_ - referenceLow_;
  double criticalLow  = referenceLow_  - margin * 0.5;
  double criticalHigh = referenceHigh_ + margin * 0.5;

  // Prüfung gegen kritische Grenzen
  if (numValue < criticalLow || numValue > criticalHigh) {
    return Flag::CRITICAL;
  }

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
