#include "utils/Fhir.h"
#include <cctype>
#include <ctime>
#include <sstream>

namespace opensylab {
namespace utils {
namespace {

constexpr size_t kMaxImportBytes = 10 * 1024 * 1024;

bool isWhitespace(char ch) { return std::isspace(static_cast<unsigned char>(ch)); }

std::string trim(const std::string &value) {
  const size_t start = value.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  const size_t end = value.find_last_not_of(" \t\r\n");
  return value.substr(start, end - start + 1);
}

std::string jsonEscape(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (char ch : value) {
    switch (ch) {
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\b':
      out += "\\b";
      break;
    case '\f':
      out += "\\f";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      out += ch;
      break;
    }
  }
  return out;
}

std::string mapIssueType(const std::string &code) {
  const std::string lower = trim(code);
  if (lower == "validation_error") {
    return "invalid";
  }
  if (lower == "not_found") {
    return "not-found";
  }
  if (lower == "conflict") {
    return "conflict";
  }
  if (lower == "unauthorized" || lower == "forbidden") {
    return "forbidden";
  }
  if (lower == "internal_error") {
    return "exception";
  }
  return "processing";
}

size_t findKey(const std::string &json, const std::string &key,
               size_t start = 0) {
  const std::string needle = "\"" + key + "\"";
  return json.find(needle, start);
}

size_t skipWhitespace(const std::string &json, size_t pos) {
  while (pos < json.size() && isWhitespace(json[pos])) {
    ++pos;
  }
  return pos;
}

bool parseJsonString(const std::string &json, size_t &pos,
                     std::string &out) {
  if (pos >= json.size() || json[pos] != '"') {
    return false;
  }
  ++pos;
  std::string value;
  while (pos < json.size()) {
    char ch = json[pos++];
    if (ch == '\\' && pos < json.size()) {
      char esc = json[pos++];
      switch (esc) {
      case '"':
        value.push_back('"');
        break;
      case '\\':
        value.push_back('\\');
        break;
      case 'n':
        value.push_back('\n');
        break;
      case 'r':
        value.push_back('\r');
        break;
      case 't':
        value.push_back('\t');
        break;
      default:
        value.push_back(esc);
        break;
      }
      continue;
    }
    if (ch == '"') {
      out = value;
      return true;
    }
    value.push_back(ch);
  }
  return false;
}

std::string extractStringValue(const std::string &json, size_t keyPos) {
  size_t pos = json.find(':', keyPos);
  if (pos == std::string::npos) {
    return "";
  }
  pos = skipWhitespace(json, pos + 1);
  std::string value;
  if (pos < json.size() && json[pos] == '"' &&
      parseJsonString(json, pos, value)) {
    return value;
  }
  return "";
}

std::string extractNumberValue(const std::string &json, size_t keyPos) {
  size_t pos = json.find(':', keyPos);
  if (pos == std::string::npos) {
    return "";
  }
  pos = skipWhitespace(json, pos + 1);
  size_t start = pos;
  while (pos < json.size() &&
         (std::isdigit(static_cast<unsigned char>(json[pos])) ||
          json[pos] == '.' || json[pos] == '-' || json[pos] == '+')) {
    ++pos;
  }
  if (start == pos) {
    return "";
  }
  return trim(json.substr(start, pos - start));
}

std::string findStringValue(const std::string &json, const std::string &key,
                            size_t start = 0) {
  size_t pos = findKey(json, key, start);
  if (pos == std::string::npos) {
    return "";
  }
  return extractStringValue(json, pos);
}

std::string findNumberOrStringValue(const std::string &json,
                                    const std::string &key, size_t start = 0) {
  size_t pos = findKey(json, key, start);
  if (pos == std::string::npos) {
    return "";
  }
  std::string value = extractStringValue(json, pos);
  if (!value.empty()) {
    return value;
  }
  return extractNumberValue(json, pos);
}

size_t findObjectStart(const std::string &json, size_t pos) {
  for (size_t i = pos; i-- > 0;) {
    if (json[i] == '{') {
      return i;
    }
  }
  return std::string::npos;
}

std::string extractObject(const std::string &json, size_t start) {
  if (start == std::string::npos || start >= json.size() ||
      json[start] != '{') {
    return "";
  }
  int depth = 0;
  bool inString = false;
  bool escape = false;
  for (size_t i = start; i < json.size(); ++i) {
    char ch = json[i];
    if (inString) {
      if (escape) {
        escape = false;
      } else if (ch == '\\') {
        escape = true;
      } else if (ch == '"') {
        inString = false;
      }
      continue;
    }
    if (ch == '"') {
      inString = true;
      continue;
    }
    if (ch == '{') {
      depth++;
    } else if (ch == '}') {
      depth--;
      if (depth == 0) {
        return json.substr(start, i - start + 1);
      }
    }
  }
  return "";
}

std::vector<std::string> extractResources(const std::string &json,
                                          const std::string &resourceType) {
  std::vector<std::string> resources;
  const std::string needle = "\"resourceType\":\"" + resourceType + "\"";
  size_t pos = 0;
  while ((pos = json.find(needle, pos)) != std::string::npos) {
    size_t start = findObjectStart(json, pos);
    std::string obj = extractObject(json, start);
    // Validate that the extracted object actually contains the expected resourceType
    // (findObjectStart may return a wrong '{' if a preceding string value contains '{')
    if (!obj.empty() && obj.find(needle) != std::string::npos) {
      resources.push_back(obj);
      pos = start + obj.size();
    } else {
      pos += needle.size();
    }
  }
  return resources;
}

std::string findIdentifierValue(const std::string &json, size_t start) {
  size_t idPos = json.find("\"identifier\"", start);
  if (idPos == std::string::npos) {
    return "";
  }
  return findStringValue(json, "value", idPos);
}

std::string findCodeText(const std::string &json, size_t start) {
  size_t codePos = json.find("\"code\"", start);
  if (codePos == std::string::npos) {
    return "";
  }
  std::string text = findStringValue(json, "text", codePos);
  if (!text.empty()) {
    return text;
  }
  size_t codingPos = json.find("\"coding\"", codePos);
  if (codingPos != std::string::npos) {
    return findStringValue(json, "code", codingPos);
  }
  return "";
}

std::string extractReferenceRange(const std::string &json, size_t start) {
  size_t rangePos = json.find("\"referenceRange\"", start);
  if (rangePos == std::string::npos) {
    return "";
  }
  size_t lowPos = json.find("\"low\"", rangePos);
  size_t highPos = json.find("\"high\"", rangePos);
  std::string low = lowPos == std::string::npos
                        ? ""
                        : findNumberOrStringValue(json, "value", lowPos);
  std::string high = highPos == std::string::npos
                         ? ""
                         : findNumberOrStringValue(json, "value", highPos);
  if (low.empty() && high.empty()) {
    return "";
  }
  if (high.empty()) {
    return low;
  }
  if (low.empty()) {
    return high;
  }
  return low + "-" + high;
}

} // namespace

bool FhirParser::parse(const std::string &rawJson) {
  rawJson_ = rawJson;
  errors_.clear();
  if (rawJson_.empty()) {
    addError("Bundle", "validation_error", "FHIR payload is empty");
    return false;
  }
  if (rawJson_.find("\"resourceType\"") == std::string::npos) {
    addError("Bundle", "validation_error", "Missing resourceType");
    return false;
  }
  return true;
}

bool FhirParser::mapBundle(MappedData &out) {
  out = MappedData{};
  errors_.clear();

  if (rawJson_.empty()) {
    addError("Bundle", "validation_error", "FHIR payload is empty");
    return false;
  }

  const auto patients = extractResources(rawJson_, "Patient");
  const auto specimens = extractResources(rawJson_, "Specimen");
  const auto serviceRequests = extractResources(rawJson_, "ServiceRequest");
  const auto observations = extractResources(rawJson_, "Observation");

  if (patients.empty()) {
    addError("Patient", "validation_error", "Missing Patient resource");
  }
  if (specimens.empty()) {
    addError("Specimen", "validation_error", "Missing Specimen resource");
  }
  if (serviceRequests.empty()) {
    addError("ServiceRequest", "validation_error",
             "Missing ServiceRequest resource");
  }
  if (observations.empty()) {
    addError("Observation", "validation_error", "Missing Observation resource");
  }

  if (!errors_.empty()) {
    return false;
  }

  const std::string patientId = findIdentifierValue(patients[0], 0);
  if (patientId.empty()) {
    addError("Patient.identifier", "validation_error",
             "Missing patient identifier");
  }
  out.sample.patientId = patientId;
  out.sample.patientName = findStringValue(patients[0], "text");

  const std::string sampleId = findIdentifierValue(specimens[0], 0);
  if (sampleId.empty()) {
    addError("Specimen.identifier", "validation_error",
             "Missing specimen identifier");
  }
  out.sample.sampleId = sampleId;

  const std::string orderId = findIdentifierValue(serviceRequests[0], 0);
  if (orderId.empty()) {
    addError("ServiceRequest.identifier", "validation_error",
             "Missing order identifier");
  }
  const std::string testType = findCodeText(serviceRequests[0], 0);
  out.order.orderId = orderId;
  out.order.sampleId = sampleId;
  out.order.testType = testType;
  out.sample.description = testType;

  if (!errors_.empty()) {
    return false;
  }

  int index = 1;
  for (const auto &obs : observations) {
    ResultData result;
    const std::string parameter = findCodeText(obs, 0);
    const size_t valueQuantityPos = obs.find("\"valueQuantity\"");
    const size_t searchStart =
        valueQuantityPos == std::string::npos ? 0 : valueQuantityPos;
    const std::string value = findNumberOrStringValue(obs, "value", searchStart);
    const std::string unit = findStringValue(obs, "unit", searchStart);
    const std::string referenceRange = extractReferenceRange(obs, 0);

    if (parameter.empty()) {
      addError("Observation.code", "validation_error",
               "Missing observation code");
    }
    if (value.empty()) {
      addError("Observation.value", "validation_error",
               "Missing observation value");
    }

    result.orderId = orderId;
    result.parameter = parameter;
    result.value = value;
    result.unit = unit;
    result.referenceRange = referenceRange;
    result.resultId =
        orderId.empty() ? "result-" + std::to_string(index)
                        : orderId + "-" + std::to_string(index);
    out.results.push_back(std::move(result));
    index++;
  }

  return errors_.empty();
}

void FhirParser::addError(const std::string &path, const std::string &code,
                          const std::string &message) {
  Error error;
  error.path = path;
  error.code = code;
  error.message = message;
  errors_.push_back(std::move(error));
}

FhirExchange::FhirExchange(std::shared_ptr<db::IDatabase> database)
    : database_(std::move(database)) {}

bool FhirExchange::importBundle(const std::string &payload,
                                const std::string &actor,
                                ImportSummary &summary) {
  summary = ImportSummary{};
  if (!database_) {
    summary.lastError = "Database unavailable";
    return false;
  }
  if (payload.size() > kMaxImportBytes) {
    FhirParser::Error err;
    err.path = "Bundle";
    err.code = "validation_error";
    err.message = "Payload too large";
    summary.errors.push_back(err);
    logErrors(summary.errors, actor);
    summary.operationOutcome = buildOperationOutcome(summary.errors);
    summary.lastError = "FHIR payload exceeds size limit";
    return false;
  }

  FhirParser parser;
  if (!parser.parse(payload)) {
    summary.errors = parser.getErrors();
    logErrors(summary.errors, actor);
    summary.operationOutcome = buildOperationOutcome(summary.errors);
    summary.lastError = "Failed to parse FHIR payload";
    return false;
  }

  FhirParser::MappedData mapped;
  if (!parser.mapBundle(mapped)) {
    summary.errors = parser.getErrors();
    logErrors(summary.errors, actor);
    summary.operationOutcome = buildOperationOutcome(summary.errors);
    summary.lastError = "Failed to map FHIR payload";
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

  std::vector<core::TestResult> results;
  results.reserve(mapped.results.size());

  for (const auto &resultData : mapped.results) {
    if (resultData.parameter.empty() || resultData.value.empty()) {
      FhirParser::Error err;
      err.path = "Observation";
      err.code = "validation_error";
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

    results.push_back(result);
  }

  if (!results.empty()) {
    auto batch = database_->createTestResultsBatch(results, actor);
    summary.resultsCreated += static_cast<int>(batch.inserted);
    for (const auto &failure : batch.failures) {
      FhirParser::Error err;
      err.path = "Observation";
      err.code = "conflict";
      err.message = failure.message;
      summary.errors.push_back(err);
    }
  }

  if (!summary.errors.empty()) {
    logErrors(summary.errors, actor);
    summary.operationOutcome = buildOperationOutcome(summary.errors);
    summary.lastError = "FHIR import completed with errors";
    return false;
  }

  return true;
}

std::string FhirExchange::exportBundle(
    const core::Sample &sample, const core::Order &order,
    const std::vector<core::TestResult> &results) {
  std::ostringstream out;
  out << "{";
  out << "\"resourceType\":\"Bundle\",";
  out << "\"type\":\"collection\",";
  out << "\"entry\":[";

  out << "{\"resource\":{\"resourceType\":\"Patient\",\"identifier\":[{\"value\":\""
      << jsonEscape(sample.getPatientId())
      << "\"}],\"name\":[{\"text\":\"" << jsonEscape(sample.getPatientName())
      << "\"}]}}";

  out << ",{\"resource\":{\"resourceType\":\"Specimen\",\"identifier\":[{\"value\":\""
      << jsonEscape(sample.getSampleId()) << "\"}]}}";

  out << ",{\"resource\":{\"resourceType\":\"ServiceRequest\",\"identifier\":[{\"value\":\""
      << jsonEscape(order.getOrderId()) << "\"}],\"code\":{\"text\":\""
      << jsonEscape(order.getTestType()) << "\"}}}";

  out << ",{\"resource\":{\"resourceType\":\"DiagnosticReport\",\"status\":\"final\",\"result\":[";
  for (size_t i = 0; i < results.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << "{\"reference\":\"Observation/obs-" << (i + 1) << "\"}";
  }
  out << "]}}";

  for (size_t i = 0; i < results.size(); ++i) {
    const auto &result = results[i];
    out << ",{\"resource\":{\"resourceType\":\"Observation\",\"id\":\"obs-"
        << (i + 1) << "\",\"code\":{\"text\":\""
        << jsonEscape(result.getTestParameter())
        << "\"},\"valueQuantity\":{\"value\":\""
        << jsonEscape(result.getValue()) << "\",\"unit\":\""
        << jsonEscape(result.getUnit()) << "\"}";

    const double refLow = result.getReferenceLow();
    const double refHigh = result.getReferenceHigh();
    const bool hasLow = refLow != 0.0;
    const bool hasHigh = refHigh != 0.0;
    if (hasLow || hasHigh) {
      out << ",\"referenceRange\":[{";
      bool wrote = false;
      if (hasLow) {
        out << "\"low\":{\"value\":" << refLow;
        if (!result.getUnit().empty()) {
          out << ",\"unit\":\"" << jsonEscape(result.getUnit()) << "\"";
        }
        out << "}";
        wrote = true;
      }
      if (hasHigh) {
        if (wrote) {
          out << ",";
        }
        out << "\"high\":{\"value\":" << refHigh;
        if (!result.getUnit().empty()) {
          out << ",\"unit\":\"" << jsonEscape(result.getUnit()) << "\"";
        }
        out << "}";
      }
      out << "}]";
    }
    out << "}}";
  }

  out << "]}";
  return out.str();
}

void FhirExchange::logErrors(const std::vector<FhirParser::Error> &errors,
                             const std::string &actor) {
  if (!database_) {
    return;
  }

  for (const auto &error : errors) {
    std::ostringstream details;
    details << "FHIR error: path=" << error.path << " code=" << error.code
            << " message=" << error.message;
    core::AuditEntry entry(core::AuditEntry::ActionType::UPDATE,
                           core::AuditEntry::EntityType::SYSTEM,
                           "fhir", actor, details.str());
    (void)database_->logAudit(entry);
  }
}

std::string FhirExchange::buildOperationOutcome(
    const std::vector<FhirParser::Error> &errors) const {
  std::ostringstream out;
  out << "{";
  out << "\"resourceType\":\"OperationOutcome\",";
  out << "\"issue\":[";
  for (size_t i = 0; i < errors.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << "{";
    out << "\"severity\":\"error\",";
    out << "\"code\":\"" << jsonEscape(mapIssueType(errors[i].code)) << "\",";
    out << "\"details\":{\"text\":\"" << jsonEscape(errors[i].message)
        << "\"},";
    out << "\"diagnostics\":\""
        << jsonEscape(errors[i].path + " (" + errors[i].code + ")") << "\"";
    out << "}";
  }
  out << "]}";
  return out.str();
}

} // namespace utils
} // namespace opensylab
