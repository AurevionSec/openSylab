#include "utils/Hl7.h"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <sstream>

namespace opensylab {
namespace utils {
namespace {

std::string trim(const std::string &value) {
  const size_t start = value.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  const size_t end = value.find_last_not_of(" \t\r\n");
  return value.substr(start, end - start + 1);
}

std::string toUpper(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return value;
}

std::string firstNonEmpty(const std::string &a, const std::string &b) {
  if (!a.empty()) {
    return a;
  }
  return b;
}

} // namespace

Hl7Exchange::Hl7Exchange(std::shared_ptr<db::Database> database)
    : database_(std::move(database)) {}

bool Hl7Exchange::importOruR01Message(const std::string &message,
                                      const std::string &actor,
                                      ImportSummary &summary) {
  summary = ImportSummary{};

  if (!database_) {
    summary.lastError = "Database unavailable";
    return false;
  }

  Hl7Parser parser;
  if (!parser.parse(message)) {
    summary.errors = parser.getErrors();
    logErrors(summary.errors, actor, parser.getMessageControlId(),
              parser.getMessageType(), parser.getTriggerEvent());
    summary.lastError = "Failed to parse HL7 message";
    return false;
  }
  if (!parser.validateOruR01()) {
    summary.errors = parser.getErrors();
    logErrors(summary.errors, actor, parser.getMessageControlId(),
              parser.getMessageType(), parser.getTriggerEvent());
    summary.lastError = "Invalid ORU^R01 message";
    return false;
  }

  Hl7Parser::MappedData mapped;
  if (!parser.mapOruR01(mapped)) {
    summary.errors = parser.getErrors();
    logErrors(summary.errors, actor, parser.getMessageControlId(),
              parser.getMessageType(), parser.getTriggerEvent());
    summary.lastError = "Failed to map ORU^R01 message";
    return false;
  }

  auto findLine = [&](const std::string &segmentName) -> int {
    for (const auto &segment : parser.getSegments()) {
      if (segment.name == segmentName) {
        return segment.line;
      }
    }
    return 0;
  };

  if (mapped.sample.sampleId.empty() || mapped.sample.patientId.empty()) {
    Hl7Parser::Error err;
    err.segment = "PID";
    err.line = findLine("PID");
    err.fieldIndex = mapped.sample.patientId.empty() ? 3 : 0;
    err.message = "Missing sample_id or patient_id";
    summary.errors.push_back(err);
    logErrors(summary.errors, actor, parser.getMessageControlId(),
              parser.getMessageType(), parser.getTriggerEvent());
    summary.lastError = "Missing required sample identifiers";
    return false;
  }

  core::Sample sample(mapped.sample.sampleId, mapped.sample.patientId);
  sample.setPatientName(mapped.sample.patientName);
  sample.setDescription(mapped.sample.description);
  sample.setStatus(core::Sample::Status::REGISTERED);

  if (!database_->createSample(sample, actor)) {
    auto existing = database_->getSampleByBarcode(mapped.sample.sampleId);
    if (!existing) {
      summary.lastError = database_->getLastError();
      return false;
    }
  } else {
    summary.samplesCreated++;
  }

  if (mapped.order.orderId.empty() || mapped.order.sampleId.empty()) {
    Hl7Parser::Error err;
    err.segment = "OBR";
    err.line = findLine("OBR");
    err.fieldIndex = mapped.order.orderId.empty() ? 2 : 3;
    err.message = "Missing order_id or sample_id";
    summary.errors.push_back(err);
    logErrors(summary.errors, actor, parser.getMessageControlId(),
              parser.getMessageType(), parser.getTriggerEvent());
    summary.lastError = "Missing required order identifiers";
    return false;
  }

  core::Order order(mapped.order.orderId, mapped.order.sampleId,
                    mapped.order.testType);
  order.setStatus(core::Order::Status::REQUESTED);
  order.setPriority(core::Order::Priority::NORMAL);
  order.setRequestedDate(std::time(nullptr));

  if (!database_->createOrder(order, actor)) {
    auto existing = database_->getOrderByOrderId(mapped.order.orderId);
    if (!existing) {
      summary.lastError = database_->getLastError();
      return false;
    }
  } else {
    summary.ordersCreated++;
  }

  auto storedOrder = database_->getOrderByOrderId(mapped.order.orderId);
  if (!storedOrder) {
    summary.lastError = database_->getLastError();
    return false;
  }
  const int orderDbId = storedOrder->getId();

  for (const auto &resultData : mapped.results) {
    if (resultData.parameter.empty() || resultData.value.empty()) {
      Hl7Parser::Error err;
      err.segment = "OBX";
      err.fieldIndex = resultData.parameter.empty() ? 3 : 5;
      err.message = "Missing observation parameter/value";
      summary.errors.push_back(err);
      continue;
    }

    std::string resultId = resultData.resultId;
    if (resultId.empty()) {
      resultId = mapped.order.orderId + "-" + resultData.parameter;
    }

    core::TestResult result(resultId, orderDbId, resultData.parameter);
    result.setValue(resultData.value);
    result.setUnit(resultData.unit);
    result.setReferenceRange(resultData.referenceRange);
    result.setStatus(core::TestResult::Status::ENTERED);
    result.setMeasuredDate(std::time(nullptr));
    result.setFlag(result.evaluateFlag());

    if (database_->createTestResult(result, actor)) {
      summary.resultsCreated++;
    } else {
      Hl7Parser::Error err;
      err.segment = "OBX";
      err.fieldIndex = 0;
      err.message = database_->getLastError();
      summary.errors.push_back(err);
    }
  }

  if (!summary.errors.empty()) {
    logErrors(summary.errors, actor, parser.getMessageControlId(),
              parser.getMessageType(), parser.getTriggerEvent());
    summary.lastError = "HL7 import completed with errors";
    return false;
  }

  return true;
}

std::string Hl7Exchange::exportOruR01Message(
    const core::Sample &sample, const core::Order &order,
    const std::vector<core::TestResult> &results) {
  std::ostringstream out;
  const std::string timestamp = std::to_string(std::time(nullptr));

  out << "MSH|^~\\&|OPENSYLAB|LAB|OPENSYLAB|LAB|" << timestamp
      << "||ORU^R01|OSY" << timestamp << "|P|2.5.1\r";
  out << "PID|1||" << sample.getPatientId() << "||"
      << sample.getPatientName() << "\r";
  out << "OBR|1|" << order.getOrderId() << "|" << order.getSampleId()
      << "|" << order.getTestType() << "\r";

  int index = 1;
  for (const auto &result : results) {
    out << "OBX|" << index++ << "|ST|" << result.getTestParameter()
        << "||" << result.getValue() << "|" << result.getUnit()
        << "|" << result.getReferenceRange() << "\r";
  }

  return out.str();
}

void Hl7Exchange::logErrors(const std::vector<Hl7Parser::Error> &errors,
                            const std::string &actor,
                            const std::string &messageId,
                            const std::string &messageType,
                            const std::string &triggerEvent) {
  if (!database_) {
    return;
  }

  for (const auto &error : errors) {
    std::ostringstream details;
    details << "HL7 error: message_id="
            << (messageId.empty() ? "unknown" : messageId)
            << " type=" << (messageType.empty() ? "unknown" : messageType)
            << "^"
            << (triggerEvent.empty() ? "unknown" : triggerEvent)
            << " segment=" << error.segment
            << " field=" << error.fieldIndex
            << " line=" << error.line
            << " message=" << error.message;
    core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                           core::AuditEntry::EntityType::SYSTEM,
                           "hl7", actor, details.str());
    (void)database_->logAudit(entry);
  }
}

bool Hl7Parser::parse(const std::string &rawMessage) {
  segments_.clear();
  errors_.clear();
  messageType_.clear();
  triggerEvent_.clear();
  messageControlId_.clear();
  version_.clear();

  if (rawMessage.empty()) {
    addError(0, "HL7", 0, "Message is empty");
    return false;
  }

  std::string normalized = rawMessage;
  for (char &ch : normalized) {
    if (ch == '\r') {
      ch = '\n';
    }
  }

  std::string line;
  std::istringstream input(normalized);
  int lineNumber = 0;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty()) {
      continue;
    }
    lineNumber++;

    char separator = '|';
    if (line.size() >= 4 && line.rfind("MSH", 0) == 0) {
      separator = line[3];
    }

    auto fields = splitFields(line, separator);
    if (fields.empty()) {
      continue;
    }

    Segment segment;
    segment.name = toUpper(trim(fields[0]));
    segment.fields = std::move(fields);
    segment.line = lineNumber;
    segments_.push_back(std::move(segment));
  }

  if (segments_.empty()) {
    addError(0, "HL7", 0, "No segments found");
    return false;
  }

  const Segment *msh = findFirst("MSH");
  if (!msh) {
    addError(0, "MSH", 0, "Missing MSH segment");
    return false;
  }

  const std::string mshType = fieldValue(*msh, 8);
  if (!mshType.empty()) {
    const size_t sep = mshType.find('^');
    if (sep != std::string::npos) {
      messageType_ = mshType.substr(0, sep);
      triggerEvent_ = mshType.substr(sep + 1);
    } else {
      messageType_ = mshType;
    }
  }
  messageControlId_ = fieldValue(*msh, 9);
  version_ = fieldValue(*msh, 12);

  return true;
}

bool Hl7Parser::validateOruR01() {
  errors_.clear();

  if (segments_.empty()) {
    addError(0, "HL7", 0, "No parsed segments available");
    return false;
  }

  if (messageType_.empty() || triggerEvent_.empty()) {
    const Segment *msh = findFirst("MSH");
    if (msh) {
      const std::string mshType = fieldValue(*msh, 8);
      if (!mshType.empty()) {
        const size_t sep = mshType.find('^');
        if (sep != std::string::npos) {
          messageType_ = mshType.substr(0, sep);
          triggerEvent_ = mshType.substr(sep + 1);
        } else {
          messageType_ = mshType;
        }
      }
      if (messageControlId_.empty()) {
        messageControlId_ = fieldValue(*msh, 9);
      }
    }
  }

  if (toUpper(messageType_) != "ORU" || toUpper(triggerEvent_) != "R01") {
    const Segment *msh = findFirst("MSH");
    const std::string mshType = msh ? fieldValue(*msh, 8) : "";
    if (toUpper(mshType).find("ORU^R01") != std::string::npos) {
      messageType_ = "ORU";
      triggerEvent_ = "R01";
    } else {
      addError(0, "MSH", 9, "Unsupported message type (expected ORU^R01)");
    }
  }

  if (!findFirst("PID")) {
    addError(0, "PID", 0, "Missing PID segment");
  }
  if (!findFirst("OBR")) {
    addError(0, "OBR", 0, "Missing OBR segment");
  }
  if (findAll("OBX").empty()) {
    addError(0, "OBX", 0, "Missing OBX segment");
  }

  return errors_.empty();
}

bool Hl7Parser::mapOruR01(MappedData &out) {
  out = MappedData{};
  errors_.clear();

  if (segments_.empty()) {
    addError(0, "HL7", 0, "No parsed segments available");
    return false;
  }

  if (toUpper(messageType_) != "ORU" || toUpper(triggerEvent_) != "R01") {
    addError(0, "MSH", 9, "Unsupported message type (expected ORU^R01)");
    return false;
  }

  const Segment *pid = findFirst("PID");
  const Segment *obr = findFirst("OBR");
  const auto obxSegments = findAll("OBX");

  if (!pid || !obr || obxSegments.empty()) {
    addError(0, "HL7", 0, "Missing required segments for ORU^R01");
    return false;
  }

  const std::string patientId = fieldValue(*pid, 3);
  if (patientId.empty()) {
    addError(pid->line, "PID", 3, "Missing patient identifier (PID-3)");
  }
  const std::string patientName = fieldValue(*pid, 5);

  const std::string obrPlacer = fieldValue(*obr, 2);
  const std::string obrFiller = fieldValue(*obr, 3);
  const std::string orderId = firstNonEmpty(obrPlacer, obrFiller);
  if (orderId.empty()) {
    addError(obr->line, "OBR", 2, "Missing order identifier (OBR-2/OBR-3)");
  }
  const std::string sampleId = firstNonEmpty(obrFiller, obrPlacer);
  if (sampleId.empty()) {
    addError(obr->line, "OBR", 3, "Missing sample identifier (OBR-3/OBR-2)");
  }
  const std::string testType = fieldValue(*obr, 4);

  out.sample.sampleId = sampleId;
  out.sample.patientId = patientId;
  out.sample.patientName = patientName;
  out.sample.description = testType;

  out.order.orderId = orderId;
  out.order.sampleId = sampleId;
  out.order.testType = testType;

  for (const auto *obx : obxSegments) {
    ResultData result;
    const std::string setId = fieldValue(*obx, 1);
    const std::string obsId = fieldValue(*obx, 3);
    const std::string value = fieldValue(*obx, 5);
    const std::string unit = fieldValue(*obx, 6);
    const std::string refRange = fieldValue(*obx, 7);

    if (obsId.empty()) {
      addError(obx->line, "OBX", 3, "Missing observation identifier (OBX-3)");
    }
    if (value.empty()) {
      addError(obx->line, "OBX", 5, "Missing observation value (OBX-5)");
    }

    std::string suffix = !setId.empty() ? setId : obsId;
    if (suffix.empty()) {
      suffix = "result";
    }

    result.resultId = orderId.empty() ? suffix : orderId + "-" + suffix;
    result.orderId = orderId;
    result.parameter = obsId;
    result.value = value;
    result.unit = unit;
    result.referenceRange = refRange;

    out.results.push_back(std::move(result));
  }

  return errors_.empty();
}

std::vector<std::string> Hl7Parser::splitFields(const std::string &line,
                                                char separator) {
  std::vector<std::string> fields;
  std::string current;
  for (char ch : line) {
    if (ch == separator) {
      fields.push_back(current);
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  fields.push_back(current);
  return fields;
}

const Hl7Parser::Segment *Hl7Parser::findFirst(const std::string &name) const {
  for (const auto &segment : segments_) {
    if (segment.name == name) {
      return &segment;
    }
  }
  return nullptr;
}

std::vector<const Hl7Parser::Segment *>
Hl7Parser::findAll(const std::string &name) const {
  std::vector<const Segment *> matches;
  for (const auto &segment : segments_) {
    if (segment.name == name) {
      matches.push_back(&segment);
    }
  }
  return matches;
}

void Hl7Parser::addError(int line, const std::string &segment, int field,
                         const std::string &message) {
  Error error;
  error.line = line;
  error.segment = segment;
  error.fieldIndex = field;
  error.message = message;
  errors_.push_back(std::move(error));
}

std::string Hl7Parser::fieldValue(const Segment &segment, size_t index) {
  if (index >= segment.fields.size()) {
    return "";
  }
  return trim(segment.fields[index]);
}

} // namespace utils
} // namespace opensylab
