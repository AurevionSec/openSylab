#include "core/Sample.h"
#include <stdexcept>
#include <unordered_map>

namespace opensylab {
namespace core {

// Konstruktoren
Sample::Sample()
    : id_(0), sampleId_(""), patientId_(""), patientName_(""), description_(""),
      status_(Status::REGISTERED), registrationDate_(std::time(nullptr)) {}

Sample::Sample(const std::string &sampleId, const std::string &patientId)
    : id_(0), sampleId_(sampleId), patientId_(patientId), patientName_(""),
      description_(""), status_(Status::REGISTERED),
      registrationDate_(std::time(nullptr)) {}

// Hilfsfunktionen
std::string Sample::getStatusString() const { return statusToString(status_); }

std::string Sample::statusToString(Status status) {
  switch (status) {
  case Status::REGISTERED:
    return "REGISTERED";
  case Status::IN_ANALYSIS:
    return "IN_ANALYSIS";
  case Status::ANALYZED:
    return "ANALYZED";
  case Status::VALIDATED:
    return "VALIDATED";
  case Status::ARCHIVED:
    return "ARCHIVED";
  default:
    return "UNKNOWN";
  }
}

Sample::Status Sample::stringToStatus(const std::string &statusStr) {
  static const std::unordered_map<std::string, Status> statusMap = {
      {"Erfasst", Status::REGISTERED},     {"REGISTERED", Status::REGISTERED},
      {"In Analyse", Status::IN_ANALYSIS}, {"IN_ANALYSIS", Status::IN_ANALYSIS},
      {"Analysiert", Status::ANALYZED},    {"ANALYZED", Status::ANALYZED},
      {"Validiert", Status::VALIDATED},    {"VALIDATED", Status::VALIDATED},
      {"Archiviert", Status::ARCHIVED},    {"ARCHIVED", Status::ARCHIVED},
  };

  auto it = statusMap.find(statusStr);
  if (it != statusMap.end()) {
    return it->second;
  }

  return Status::REGISTERED;
}

} // namespace core
} // namespace opensylab
