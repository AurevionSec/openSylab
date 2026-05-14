#include "core/Order.h"
#include <stdexcept>
#include <unordered_map>

namespace opensylab {
namespace core {

// Konstruktoren
Order::Order()
    : id_(0), orderId_(""), sampleId_(""), testType_(""),
      status_(Status::REQUESTED), priority_(Priority::NORMAL),
      requestedDate_(std::time(nullptr)), completedDate_(0), requestedBy_(""),
      notes_("") {}

Order::Order(const std::string &orderId, const std::string &sampleId,
             const std::string &testType)
    : id_(0), orderId_(orderId), sampleId_(sampleId), testType_(testType),
      status_(Status::REQUESTED), priority_(Priority::NORMAL),
      requestedDate_(std::time(nullptr)), completedDate_(0), requestedBy_(""),
      notes_("") {}

// Status-Hilfsfunktionen
std::string Order::getStatusString() const { return statusToString(status_); }

std::string Order::statusToString(Status status) {
  switch (status) {
  case Status::REQUESTED:
    return "REQUESTED";
  case Status::IN_PROGRESS:
    return "IN_PROGRESS";
  case Status::COMPLETED:
    return "COMPLETED";
  case Status::VALIDATED:
    return "VALIDATED";
  case Status::CANCELLED:
    return "CANCELLED";
  default:
    return "UNKNOWN";
  }
}

Order::Status Order::stringToStatus(const std::string &statusStr) {
  static const std::unordered_map<std::string, Status> statusMap = {
      {"Angefordert", Status::REQUESTED},
      {"REQUESTED", Status::REQUESTED},
      {"In Bearbeitung", Status::IN_PROGRESS},
      {"IN_PROGRESS", Status::IN_PROGRESS},
      {"Abgeschlossen", Status::COMPLETED},
      {"COMPLETED", Status::COMPLETED},
      {"Validiert", Status::VALIDATED},
      {"VALIDATED", Status::VALIDATED},
      {"Storniert", Status::CANCELLED},
      {"CANCELLED", Status::CANCELLED},
  };

  auto it = statusMap.find(statusStr);
  if (it != statusMap.end()) {
    return it->second;
  }

  return Status::REQUESTED;
}

// Priorität-Hilfsfunktionen
std::string Order::getPriorityString() const {
  return priorityToString(priority_);
}

std::string Order::priorityToString(Priority priority) {
  switch (priority) {
  case Priority::NORMAL:
    return "NORMAL";
  case Priority::URGENT:
    return "URGENT";
  case Priority::EMERGENCY:
    return "EMERGENCY";
  default:
    return "UNKNOWN";
  }
}

Order::Priority Order::stringToPriority(const std::string &priorityStr) {
  static const std::unordered_map<std::string, Priority> priorityMap = {
      {"Normal", Priority::NORMAL},     {"NORMAL", Priority::NORMAL},
      {"Dringend", Priority::URGENT},   {"URGENT", Priority::URGENT},
      {"Notfall", Priority::EMERGENCY}, {"EMERGENCY", Priority::EMERGENCY},
  };

  auto it = priorityMap.find(priorityStr);
  if (it != priorityMap.end()) {
    return it->second;
  }

  return Priority::NORMAL;
}

} // namespace core
} // namespace opensylab
