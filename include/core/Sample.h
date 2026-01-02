#ifndef OPENSYLAB_SAMPLE_H
#define OPENSYLAB_SAMPLE_H

#include <ctime>
#include <string>

namespace opensylab {
namespace core {

/**
 * @brief Klasse repräsentiert eine Laborprobe im LIMS
 *
 * Eine Sample (Probe) ist die zentrale Entität im Laborsystem.
 * Jede Probe hat eine eindeutige ID, Patientendaten, Status und Metadaten.
 */
class Sample {
public:
  /**
   * @brief Status einer Probe im Labor-Workflow
   */
  enum class Status {
    REGISTERED,  // Erfasst
    IN_ANALYSIS, // In Analyse
    ANALYZED,    // Analysiert
    VALIDATED,   // Validiert/Freigegeben
    ARCHIVED     // Archiviert
  };

  // Konstruktoren
  Sample();
  Sample(const std::string &sampleId, const std::string &patientId);

  // Destruktor
  ~Sample() = default;

  // Getter
  int getId() const { return id_; }
  const std::string &getSampleId() const { return sampleId_; }
  const std::string &getPatientId() const { return patientId_; }
  const std::string &getPatientName() const { return patientName_; }
  const std::string &getDescription() const { return description_; }
  Status getStatus() const { return status_; }
  std::time_t getRegistrationDate() const { return registrationDate_; }

  // Setter
  void setId(int id) { id_ = id; }
  void setSampleId(const std::string &sampleId) { sampleId_ = sampleId; }
  void setPatientId(const std::string &patientId) { patientId_ = patientId; }
  void setPatientName(const std::string &name) { patientName_ = name; }
  void setDescription(const std::string &desc) { description_ = desc; }
  void setStatus(Status status) { status_ = status; }
  void setRegistrationDate(std::time_t date) { registrationDate_ = date; }

  // Hilfsfunktionen
  std::string getStatusString() const;
  static std::string statusToString(Status status);
  static Status stringToStatus(const std::string &statusStr);

private:
  int id_;                       // Datenbank-ID
  std::string sampleId_;         // Eindeutige Proben-ID (z.B. Barcode)
  std::string patientId_;        // Patienten-ID
  std::string patientName_;      // Patientenname
  std::string description_;      // Beschreibung der Probe
  Status status_;                // Aktueller Status
  std::time_t registrationDate_; // Registrierungsdatum
};

} // namespace core
} // namespace opensylab

#endif // OPENSYLAB_SAMPLE_H
