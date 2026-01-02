#ifndef OPENSYLAB_TESTRESULT_H
#define OPENSYLAB_TESTRESULT_H

#include <ctime>
#include <string>

namespace opensylab {
namespace core {

/**
 * @brief Klasse repräsentiert ein Testergebnis im LIMS
 *
 * Ein TestResult speichert das Ergebnis einer Laboruntersuchung.
 * Es ist mit einem Order verknüpft und enthält Messwert, Einheit,
 * Referenzbereich und Validierungsstatus.
 */
class TestResult {
public:
  /**
   * @brief Status eines Testergebnisses
   */
  enum class Status {
    PENDING,   // Ausstehend (noch kein Ergebnis)
    ENTERED,   // Eingegeben (manuell oder automatisch)
    VALIDATED, // Validiert/Freigegeben
    REJECTED,  // Abgelehnt (ungültig)
    REPEATED   // Wiederholung erforderlich
  };

  /**
   * @brief Flag für Plausibilitätsprüfung
   */
  enum class Flag {
    NORMAL,   // Im Normalbereich
    LOW,      // Unter Referenzbereich
    HIGH,     // Über Referenzbereich
    CRITICAL, // Kritisch (Panic Value)
    UNDEFINED // Kein Referenzbereich definiert
  };

  // Konstruktoren
  TestResult();
  TestResult(const std::string &resultId, int orderId,
             const std::string &testParameter);

  // Destruktor
  ~TestResult() = default;

  // Getter
  int getId() const { return id_; }
  const std::string &getResultId() const { return resultId_; }
  int getOrderId() const { return orderId_; }
  const std::string &getTestParameter() const { return testParameter_; }
  const std::string &getValue() const { return value_; }
  const std::string &getUnit() const { return unit_; }
  const std::string &getReferenceRange() const { return referenceRange_; }
  double getReferenceLow() const { return referenceLow_; }
  double getReferenceHigh() const { return referenceHigh_; }
  Status getStatus() const { return status_; }
  Flag getFlag() const { return flag_; }
  std::time_t getMeasuredDate() const { return measuredDate_; }
  const std::string &getMeasuredBy() const { return measuredBy_; }
  const std::string &getComment() const { return comment_; }

  // Setter
  void setId(int id) { id_ = id; }
  void setResultId(const std::string &resultId) { resultId_ = resultId; }
  void setOrderId(int orderId) { orderId_ = orderId; }
  void setTestParameter(const std::string &param) { testParameter_ = param; }
  void setValue(const std::string &value) { value_ = value; }
  void setUnit(const std::string &unit) { unit_ = unit; }
  void setReferenceRange(const std::string &range) { referenceRange_ = range; }
  void setReferenceLow(double low) { referenceLow_ = low; }
  void setReferenceHigh(double high) { referenceHigh_ = high; }
  void setStatus(Status status) { status_ = status; }
  void setFlag(Flag flag) { flag_ = flag; }
  void setMeasuredDate(std::time_t date) { measuredDate_ = date; }
  void setMeasuredBy(const std::string &user) { measuredBy_ = user; }
  void setComment(const std::string &comment) { comment_ = comment; }

  // Hilfsfunktionen für Status
  std::string getStatusString() const;
  static std::string statusToString(Status status);
  static Status stringToStatus(const std::string &statusStr);

  // Hilfsfunktionen für Flag
  std::string getFlagString() const;
  static std::string flagToString(Flag flag);
  static Flag stringToFlag(const std::string &flagStr);

  // Plausibilitätsprüfung
  Flag evaluateFlag() const;
  bool isNumeric() const;
  double getNumericValue() const;

private:
  int id_;                     // Datenbank-ID
  std::string resultId_;       // Eindeutige Ergebnis-ID
  int orderId_;                // Referenz zum Auftrag (Order.id)
  std::string testParameter_;  // Testparameter (z.B. "Glucose", "Hämoglobin")
  std::string value_;          // Messwert (als String für Flexibilität)
  std::string unit_;           // Einheit (z.B. "mg/dL", "mmol/L")
  std::string referenceRange_; // Anzeigetext (z.B. "70-100 mg/dL")
  double referenceLow_;        // Unterer Referenzwert
  double referenceHigh_;       // Oberer Referenzwert
  Status status_;              // Validierungsstatus
  Flag flag_;                  // Plausibilitäts-Flag
  std::time_t measuredDate_;   // Messdatum
  std::string measuredBy_;     // Messender Benutzer
  std::string comment_;        // Kommentar
};

} // namespace core
} // namespace opensylab

#endif // OPENSYLAB_TESTRESULT_H
