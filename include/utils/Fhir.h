#ifndef OPENSYLAB_FHIR_H
#define OPENSYLAB_FHIR_H

#include "core/Order.h"
#include "core/Sample.h"
#include "core/TestResult.h"
#include "db/IDatabase.h"
#include <memory>
#include <string>
#include <vector>

namespace opensylab {
namespace utils {

class FhirParser {
public:
  struct Error {
    std::string path;
    std::string code;
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

  bool parse(const std::string &rawJson);
  bool mapBundle(MappedData &out);

  const std::vector<Error> &getErrors() const { return errors_; }
  const std::string &getRawJson() const { return rawJson_; }

private:
  std::string rawJson_;
  std::vector<Error> errors_;

  void addError(const std::string &path, const std::string &code,
                const std::string &message);
};

class FhirExchange {
public:
  struct ImportSummary {
    int samplesCreated = 0;
    int ordersCreated = 0;
    int resultsCreated = 0;
    std::vector<FhirParser::Error> errors;
    std::string operationOutcome;
    std::string lastError;
  };

  explicit FhirExchange(std::shared_ptr<db::IDatabase> database);

  bool importBundle(const std::string &payload, const std::string &actor,
                    ImportSummary &summary);
  std::string exportBundle(const core::Sample &sample, const core::Order &order,
                           const std::vector<core::TestResult> &results);

private:
  std::shared_ptr<db::IDatabase> database_;

  void logErrors(const std::vector<FhirParser::Error> &errors,
                 const std::string &actor);
  std::string buildOperationOutcome(
      const std::vector<FhirParser::Error> &errors) const;
};

} // namespace utils
} // namespace opensylab

#endif // OPENSYLAB_FHIR_H
