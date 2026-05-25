#ifndef OPENSYLAB_HL7_H
#define OPENSYLAB_HL7_H

#include "core/Order.h"
#include "core/Sample.h"
#include "core/TestResult.h"
#include "db/IDatabase.h"
#include <memory>
#include <string>
#include <vector>

namespace opensylab {
namespace utils {

class Hl7Parser {
public:
  struct Segment {
    std::string name;
    std::vector<std::string> fields;
    int line = 0;
  };

  struct Error {
    int line = 0;
    std::string segment;
    int fieldIndex = 0;
    std::string message;
  };

  struct SampleData {
    std::string sampleId;
    std::string patientId;
    std::string patientName;
    std::string description;
  };

  struct OrderData {
    std::string orderId;
    std::string sampleId;
    std::string testType;
  };

  struct ResultData {
    std::string resultId;
    std::string orderId;
    std::string parameter;
    std::string value;
    std::string unit;
    std::string referenceRange;
  };

  struct MappedData {
    SampleData sample;
    OrderData order;
    std::vector<ResultData> results;
  };

  bool parse(const std::string &rawMessage);
  bool validateOruR01();
  bool mapOruR01(MappedData &out);

  const std::vector<Error> &getErrors() const { return errors_; }
  const std::vector<Segment> &getSegments() const { return segments_; }
  const std::string &getMessageType() const { return messageType_; }
  const std::string &getTriggerEvent() const { return triggerEvent_; }
  const std::string &getMessageControlId() const { return messageControlId_; }
  const std::string &getVersion() const { return version_; }

private:
  std::vector<Segment> segments_;
  std::vector<Error> errors_;
  std::string messageType_;
  std::string triggerEvent_;
  std::string messageControlId_;
  std::string version_;

  static std::vector<std::string> splitFields(const std::string &line,
                                               char separator);
  const Segment *findFirst(const std::string &name) const;
  std::vector<const Segment *> findAll(const std::string &name) const;
  void addError(int line, const std::string &segment, int field,
                const std::string &message);
  static std::string fieldValue(const Segment &segment, size_t index);
};

class Hl7Exchange {
public:
  struct ImportSummary {
    int samplesCreated = 0;
    int ordersCreated = 0;
    int resultsCreated = 0;
    std::vector<Hl7Parser::Error> errors;
    std::string lastError;
  };

  explicit Hl7Exchange(std::shared_ptr<db::IDatabase> database);

  bool importOruR01Message(const std::string &message,
                           const std::string &actor,
                           ImportSummary &summary);
  std::string exportOruR01Message(const core::Sample &sample,
                                  const core::Order &order,
                                  const std::vector<core::TestResult> &results);

private:
  std::shared_ptr<db::IDatabase> database_;
  void logErrors(const std::vector<Hl7Parser::Error> &errors,
                 const std::string &actor,
                 const std::string &messageId,
                 const std::string &messageType,
                 const std::string &triggerEvent);
};

} // namespace utils
} // namespace opensylab

#endif // OPENSYLAB_HL7_H
