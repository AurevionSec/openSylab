#ifndef OPENSYLAB_ORDER_H
#define OPENSYLAB_ORDER_H

#include <ctime>
#include <string>
#include <vector>

namespace opensylab {
namespace core {

/**
 * @brief Klasse repräsentiert einen Laborauftrag im LIMS
 *
 * Ein Order (Auftrag) verknüpft eine Probe mit angeforderten Tests.
 * Aufträge durchlaufen einen definierten Workflow von der Anforderung
 * bis zur Freigabe der Ergebnisse.
 */
class Order {
public:
  /**
   * @brief Status eines Auftrags im Workflow
   */
  enum class Status {
    REQUESTED,   // Angefordert
    IN_PROGRESS, // In Bearbeitung
    COMPLETED,   // Abgeschlossen (Ergebnisse eingegeben)
    VALIDATED,   // Validiert/Freigegeben
    CANCELLED    // Storniert
  };

  /**
   * @brief Priorität eines Auftrags
   */
  enum class Priority {
    NORMAL,   // Normale Priorität
    URGENT,   // Dringend
    EMERGENCY // Notfall (höchste Priorität)
  };

  // Konstruktoren
  Order();
  Order(const std::string &orderId, const std::string &sampleId,
        const std::string &testType);

  // Destruktor
  ~Order() = default;

  // Getter
  int getId() const { return id_; }
  const std::string &getOrderId() const { return orderId_; }
  const std::string &getSampleId() const { return sampleId_; }
  const std::string &getTestType() const { return testType_; }
  Status getStatus() const { return status_; }
  Priority getPriority() const { return priority_; }
  std::time_t getRequestedDate() const { return requestedDate_; }
  std::time_t getCompletedDate() const { return completedDate_; }
  const std::string &getRequestedBy() const { return requestedBy_; }
  const std::string &getNotes() const { return notes_; }

  // Setter
  void setId(int id) { id_ = id; }
  void setOrderId(const std::string &orderId) { orderId_ = orderId; }
  void setSampleId(const std::string &sampleId) { sampleId_ = sampleId; }
  void setTestType(const std::string &testType) { testType_ = testType; }
  void setStatus(Status status) { status_ = status; }
  void setPriority(Priority priority) { priority_ = priority; }
  void setRequestedDate(std::time_t date) { requestedDate_ = date; }
  void setCompletedDate(std::time_t date) { completedDate_ = date; }
  void setRequestedBy(const std::string &user) { requestedBy_ = user; }
  void setNotes(const std::string &notes) { notes_ = notes; }

  // Hilfsfunktionen für Status
  std::string getStatusString() const;
  static std::string statusToString(Status status);
  static Status stringToStatus(const std::string &statusStr);
  static bool isValidStatusString(const std::string &statusStr);

  // Hilfsfunktionen für Priorität
  std::string getPriorityString() const;
  static std::string priorityToString(Priority priority);
  static Priority stringToPriority(const std::string &priorityStr);
  static bool isValidPriorityString(const std::string &priorityStr);

private:
  int id_;                    // Datenbank-ID
  std::string orderId_;       // Eindeutige Auftrags-ID
  std::string sampleId_;      // Referenz zur Probe (Sample.sampleId)
  std::string testType_;      // Art des Tests (z.B. "Blutbild", "Glucose")
  Status status_;             // Aktueller Status
  Priority priority_;         // Priorität
  std::time_t requestedDate_; // Anforderungsdatum
  std::time_t completedDate_; // Abschlussdatum (0 wenn nicht abgeschlossen)
  std::string requestedBy_;   // Anfordernder Benutzer (für spätere Auth)
  std::string notes_;         // Notizen/Kommentare
};

} // namespace core
} // namespace opensylab

#endif // OPENSYLAB_ORDER_H
