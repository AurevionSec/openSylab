#include "db/Database.h"
#include <sqlite3.h>
#include <iostream>
#include <memory>

namespace {
// Stellt sicher, dass vorbereitete Statements immer finalisiert werden.
using StatementPtr = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

StatementPtr makeStatement(sqlite3_stmt* stmt) {
    return StatementPtr(stmt, sqlite3_finalize);
}

std::string columnText(sqlite3_stmt* stmt, int index) {
    const unsigned char* text = sqlite3_column_text(stmt, index);
    return text ? reinterpret_cast<const char*>(text) : "";
}

std::unique_ptr<opensylab::core::Sample> sampleFromRow(sqlite3_stmt* stmt) {
    auto sample = std::make_unique<opensylab::core::Sample>();
    sample->setId(sqlite3_column_int(stmt, 0));
    sample->setSampleId(columnText(stmt, 1));
    sample->setPatientId(columnText(stmt, 2));
    sample->setPatientName(columnText(stmt, 3));
    sample->setDescription(columnText(stmt, 4));
    sample->setStatus(opensylab::core::Sample::stringToStatus(columnText(stmt, 5)));
    sample->setRegistrationDate(static_cast<std::time_t>(sqlite3_column_int64(stmt, 6)));
    return sample;
}
} // namespace

namespace opensylab {
namespace db {

Database::Database(const std::string& dbPath)
    : dbPath_(dbPath)
    , db_(nullptr)
    , isOpen_(false)
    , lastError_("")
{
}

Database::~Database() {
    close();
}

bool Database::open() {
    clearError();

    if (isOpen_) {
        return true;
    }

    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        setError("Kann Datenbank nicht öffnen: " + std::string(sqlite3_errmsg(db_)));
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    isOpen_ = true;
    std::cout << "Datenbank erfolgreich geöffnet: " << dbPath_ << std::endl;
    return true;
}

bool Database::close() {
    clearError();

    if (!isOpen_ || db_ == nullptr) {
        return true;
    }

    int rc = sqlite3_close(db_);
    if (rc != SQLITE_OK) {
        setError("Fehler beim Schließen der Datenbank: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    db_ = nullptr;
    isOpen_ = false;
    return true;
}

bool Database::initializeSchema() {
    clearError();

    if (!isOpen_) {
        setError("Datenbank ist nicht geöffnet");
        return false;
    }

    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS samples (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sample_id TEXT NOT NULL UNIQUE,
            patient_id TEXT NOT NULL,
            patient_name TEXT,
            description TEXT,
            status TEXT NOT NULL,
            registration_date INTEGER NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_sample_id ON samples(sample_id);
        CREATE INDEX IF NOT EXISTS idx_patient_id ON samples(patient_id);
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, createTableSQL, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::string error = "SQL-Fehler beim Erstellen des Schemas: " + std::string(errMsg);
        sqlite3_free(errMsg);
        setError(error);
        return false;
    }

    std::cout << "Datenbankschema erfolgreich initialisiert" << std::endl;
    return true;
}

bool Database::createSample(const core::Sample& sample) {
    clearError();

    if (!isOpen_) {
        setError("Datenbank ist nicht geöffnet");
        return false;
    }

    const char* insertSQL = R"(
        INSERT INTO samples (sample_id, patient_id, patient_name, description, status, registration_date)
        VALUES (?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* rawStmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, insertSQL, -1, &rawStmt, nullptr);
    if (rc != SQLITE_OK) {
        setError("Fehler beim Vorbereiten des INSERT: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }
    auto stmt = makeStatement(rawStmt);

    sqlite3_bind_text(stmt.get(), 1, sample.getSampleId().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, sample.getPatientId().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 3, sample.getPatientName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 4, sample.getDescription().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 5, sample.getStatusString().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt.get(), 6, static_cast<sqlite3_int64>(sample.getRegistrationDate()));

    rc = sqlite3_step(stmt.get());

    if (rc != SQLITE_DONE) {
        setError("Fehler beim Einfügen der Probe: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    return true;
}

std::unique_ptr<core::Sample> Database::getSample(int id) {
    clearError();

    if (!isOpen_) {
        setError("Datenbank ist nicht geöffnet");
        return nullptr;
    }

    const char* selectSQL = R"(
        SELECT id, sample_id, patient_id, patient_name, description, status, registration_date
        FROM samples WHERE id = ?;
    )";

    sqlite3_stmt* rawStmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

    if (rc != SQLITE_OK) {
        setError("Fehler beim Vorbereiten des SELECT: " + std::string(sqlite3_errmsg(db_)));
        return nullptr;
    }

    auto stmt = makeStatement(rawStmt);

    sqlite3_bind_int(stmt.get(), 1, id);
    rc = sqlite3_step(stmt.get());

    if (rc == SQLITE_DONE) {
        // Kein Ergebnis - Probe existiert nicht
        setError("Probe mit ID " + std::to_string(id) + " nicht gefunden");
        return nullptr;
    } else if (rc != SQLITE_ROW) {
        // Tatsächlicher SQL-Fehler
        setError("SQL-Fehler beim Abrufen der Probe: " + std::string(sqlite3_errmsg(db_)));
        return nullptr;
    }

    try {
        return sampleFromRow(stmt.get());
    } catch (const std::exception& e) {
        setError("Fehler beim Verarbeiten der Probe: " + std::string(e.what()));
        return nullptr;
    }
}

std::unique_ptr<core::Sample> Database::getSampleByBarcode(const std::string& barcode) {
    clearError();

    if (!isOpen_) {
        setError("Datenbank ist nicht geöffnet");
        return nullptr;
    }

    const char* selectSQL = R"(
        SELECT id, sample_id, patient_id, patient_name, description, status, registration_date
        FROM samples WHERE sample_id = ?;
    )";

    sqlite3_stmt* rawStmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

    if (rc != SQLITE_OK) {
        setError("Fehler beim Vorbereiten des SELECT: " + std::string(sqlite3_errmsg(db_)));
        return nullptr;
    }

    auto stmt = makeStatement(rawStmt);

    sqlite3_bind_text(stmt.get(), 1, barcode.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt.get());

    if (rc == SQLITE_DONE) {
        // Kein Ergebnis - Probe existiert nicht
        setError("Probe mit Barcode '" + barcode + "' nicht gefunden");
        return nullptr;
    } else if (rc != SQLITE_ROW) {
        // Tatsächlicher SQL-Fehler
        setError("SQL-Fehler beim Abrufen der Probe: " + std::string(sqlite3_errmsg(db_)));
        return nullptr;
    }

    try {
        return sampleFromRow(stmt.get());
    } catch (const std::exception& e) {
        setError("Fehler beim Verarbeiten der Probe: " + std::string(e.what()));
        return nullptr;
    }
}

std::vector<std::unique_ptr<core::Sample>> Database::getAllSamples() {
    std::vector<std::unique_ptr<core::Sample>> samples;

    clearError();

    if (!isOpen_) {
        setError("Datenbank ist nicht geöffnet");
        return samples;
    }

    const char* selectSQL = R"(
        SELECT id, sample_id, patient_id, patient_name, description, status, registration_date
        FROM samples ORDER BY registration_date DESC;
    )";

    sqlite3_stmt* rawStmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, selectSQL, -1, &rawStmt, nullptr);

    if (rc != SQLITE_OK) {
        setError("Fehler beim Vorbereiten des SELECT: " + std::string(sqlite3_errmsg(db_)));
        return samples;
    }

    auto stmt = makeStatement(rawStmt);

    while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
        try {
            samples.push_back(sampleFromRow(stmt.get()));
        } catch (const std::exception& e) {
            setError("Fehler beim Verarbeiten der Probe: " + std::string(e.what()));
            return samples;
        }
    }

    // Prüfen ob Fehler beim Iterieren aufgetreten ist
    if (rc != SQLITE_DONE) {
        setError("Fehler beim Abrufen der Proben: " + std::string(sqlite3_errmsg(db_)));
    }

    // Kein Fehler - erfolgreich (auch wenn samples leer ist)
    return samples;
}

bool Database::updateSample(const core::Sample& sample) {
    clearError();

    if (!isOpen_) {
        setError("Datenbank ist nicht geöffnet");
        return false;
    }

    const char* updateSQL = R"(
        UPDATE samples SET
            sample_id = ?,
            patient_id = ?,
            patient_name = ?,
            description = ?,
            status = ?,
            registration_date = ?
        WHERE id = ?;
    )";

    sqlite3_stmt* rawStmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, updateSQL, -1, &rawStmt, nullptr);

    if (rc != SQLITE_OK) {
        setError("Fehler beim Vorbereiten des UPDATE: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    auto stmt = makeStatement(rawStmt);

    sqlite3_bind_text(stmt.get(), 1, sample.getSampleId().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, sample.getPatientId().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 3, sample.getPatientName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 4, sample.getDescription().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 5, sample.getStatusString().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt.get(), 6, static_cast<sqlite3_int64>(sample.getRegistrationDate()));
    sqlite3_bind_int(stmt.get(), 7, sample.getId());

    rc = sqlite3_step(stmt.get());

    if (rc != SQLITE_DONE) {
        setError("Fehler beim Aktualisieren der Probe: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    // Prüfen, ob überhaupt ein Datensatz mit der ID existiert.
    // sqlite3_changes ist 0, wenn entweder kein Datensatz existiert ODER alle Werte unverändert wären.
    if (sqlite3_changes(db_) == 0) {
        static const char* existsSQL = "SELECT 1 FROM samples WHERE id = ? LIMIT 1;";
        sqlite3_stmt* existsStmtRaw = nullptr;
        int existsRc = sqlite3_prepare_v2(db_, existsSQL, -1, &existsStmtRaw, nullptr);
        if (existsRc != SQLITE_OK) {
            setError("Fehler beim Prüfen der Probe: " + std::string(sqlite3_errmsg(db_)));
            return false;
        }
        auto existsStmt = makeStatement(existsStmtRaw);
        sqlite3_bind_int(existsStmt.get(), 1, sample.getId());
        existsRc = sqlite3_step(existsStmt.get());

        if (existsRc == SQLITE_ROW) {
            // Datensatz existiert, Werte waren unverändert -> trotzdem Erfolg
            return true;
        }

        if (existsRc == SQLITE_DONE) {
            setError("Probe mit ID " + std::to_string(sample.getId()) + " nicht gefunden");
        } else {
            setError("Fehler beim Prüfen der Probe: " + std::string(sqlite3_errmsg(db_)));
        }
        return false;
    }

    return true;
}

bool Database::deleteSample(int id) {
    clearError();

    if (!isOpen_) {
        setError("Datenbank ist nicht geöffnet");
        return false;
    }

    const char* deleteSQL = "DELETE FROM samples WHERE id = ?;";

    sqlite3_stmt* rawStmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, deleteSQL, -1, &rawStmt, nullptr);

    if (rc != SQLITE_OK) {
        setError("Fehler beim Vorbereiten des DELETE: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    auto stmt = makeStatement(rawStmt);

    sqlite3_bind_int(stmt.get(), 1, id);
    rc = sqlite3_step(stmt.get());

    if (rc != SQLITE_DONE) {
        setError("Fehler beim Löschen der Probe: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    // Prüfen ob tatsächlich eine Zeile gelöscht wurde
    int changes = sqlite3_changes(db_);
    if (changes == 0) {
        setError("Probe mit ID " + std::to_string(id) + " nicht gefunden");
        return false;
    }

    return true;
}

void Database::setError(const std::string& error) {
    lastError_ = error;
    std::cerr << "Datenbankfehler: " << error << std::endl;
}

} // namespace db
} // namespace opensylab
