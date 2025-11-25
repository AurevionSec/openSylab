/**
 * Demo: CSV-Import Validierung
 */

#include "utils/CsvImport.h"
#include <iostream>

int main() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  OpenSylab - CSV Import Validierungs-Demo\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    opensylab::utils::CsvImport importer;

    std::cout << "Importiere /tmp/test_validation.csv...\n\n";
    auto samples = importer.importSamples("/tmp/test_validation.csv");

    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "Ergebnis:\n";
    std::cout << "  Erfolgreich importiert: " << importer.getImportedCount() << "\n";
    std::cout << "  Fehler: " << (5 - importer.getImportedCount() - 1) << "\n";
    std::cout << "  (Header + 5 Datenzeilen in CSV)\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    std::cout << "Gültige Proben:\n";
    for (const auto& sample : samples) {
        std::cout << "  ✓ " << sample.getSampleId() << " | "
                  << sample.getPatientId() << " | "
                  << sample.getPatientName() << "\n";
    }

    return 0;
}
