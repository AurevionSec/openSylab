/**
 * @file main.cpp
 * @brief Haupteinstiegspunkt für OpenSylab LIMS
 *
 * OpenSylab - Open Source Laboratory Information Management System
 * Version: siehe include/version.h (SSOT: CMakeLists.txt)
 *
 * Dieses Programm bietet LIMS-Funktionalität für kleine
 * medizinische Diagnostiklabore:
 * - Probenverwaltung (CRUD-Operationen)
 * - Auftragsverwaltung
 * - Ergebniseingabe mit Plausibilitätsprüfung
 * - Gerätedatenschnittstelle (CSV/HL7)
 * - Audit-Trail
 * - Benutzerauthentifizierung
 * - SQLite-Datenbank
 * - Command-Line Interface
 * - Config-Datei (opensylab.conf) mit INI-Format
 */

#include "version.h"
#include "api/ApiServer.h"
#include "db/Database.h"
#include "db/IDatabase.h"
#include "utils/CliInterface.h"
#include "utils/Config.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

// Reject paths containing directory-traversal sequences.
static bool isSafeConfigPath(const std::string& path) {
    // Look for ".." as a path component (handles /../, leading ../, trailing /..)
    if (path.find("..") != std::string::npos) {
        std::cerr << "FEHLER: Konfigurationspfad enthält unerlaubte Sequenzen (..): "
                  << path << "\n";
        return false;
    }
    return true;
}

struct ConfigResolution {
    std::string path;
    bool isExplicit; // true = --config flag or OPENSYLAB_CONFIG env var
};

/**
 * @brief Resolve config-file path using the standard search order:
 *   1. CLI flag --config <path>  (explicit — missing file is a hard error)
 *   2. Environment variable OPENSYLAB_CONFIG  (explicit — same)
 *   3. ./opensylab.conf  (auto-discovered — silently skipped if absent)
 *   4. /etc/opensylab/opensylab.conf  (auto-discovered — silently skipped)
 */
static ConfigResolution resolveConfigPath(int argc, char* argv[]) {
    // 1. CLI flag (peek before full parse)
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--config") {
            return {std::string(argv[i + 1]), true};
        }
    }

    // 2. Environment variable
    const char* envCfg = std::getenv("OPENSYLAB_CONFIG");
    if (envCfg && *envCfg) {
        return {std::string(envCfg), true};
    }

    // 3. Current directory (auto-discovered)
    if (opensylab::utils::Config::fileExists("./opensylab.conf")) {
        return {"./opensylab.conf", false};
    }

    // 4. System-wide path (auto-discovered)
    if (opensylab::utils::Config::fileExists("/etc/opensylab/opensylab.conf")) {
        return {"/etc/opensylab/opensylab.conf", false};
    }

    return {{}, false};
}

/**
 * @brief Hauptfunktion
 */
int main(int argc, char* argv[]) {
    std::cout << OPENSYLAB_VERSION_STRING << " wird gestartet...\n" << std::endl;

    // ------------------------------------------------------------------
    // 1. Load config file (CLI flag / env var / default paths)
    // ------------------------------------------------------------------
    const ConfigResolution cfgResolution = resolveConfigPath(argc, argv);
    const std::string& cfgPath = cfgResolution.path;

    if (!cfgPath.empty() && !isSafeConfigPath(cfgPath)) {
        return 1;
    }

    if (cfgResolution.isExplicit && !cfgPath.empty() &&
        !opensylab::utils::Config::fileExists(cfgPath)) {
        std::cerr << "FEHLER: Konfigurationsdatei nicht gefunden: " << cfgPath << "\n";
        return 1;
    }

    opensylab::utils::AppConfig config =
        cfgPath.empty()
            ? opensylab::utils::Config::defaults()
            : opensylab::utils::Config::loadFromFile(cfgPath);

    if (!cfgPath.empty() && opensylab::utils::Config::fileExists(cfgPath)) {
        std::cout << "Konfiguration geladen: " << cfgPath << "\n";
    }

    // Apply env-var overrides for TLS (kept for backward-compatibility)
    {
        const char* envCert = std::getenv("OPENSYLAB_TLS_CERT");
        const char* envKey  = std::getenv("OPENSYLAB_TLS_KEY");
        if (envCert && *envCert) { config.tlsCert = envCert; }
        if (envKey  && *envKey)  { config.tlsKey  = envKey;  }
    }
    {
        const char* envDb = std::getenv("OPENSYLAB_DB_PATH");
        if (envDb && *envDb) { config.dbPath = envDb; }
    }

    // ------------------------------------------------------------------
    // 2. Parse CLI flags — override config values where flags are given
    // ------------------------------------------------------------------
    bool runApi = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--config" && i + 1 < argc) {
            ++i; // already consumed in resolveConfigPath — skip value
            continue;
        }
        if (arg == "--api") {
            runApi = true;
            continue;
        }
        if (arg == "--api-port" && i + 1 < argc) {
            try {
                config.apiPort = std::stoi(argv[++i]);
            } catch (...) {
                config.apiPort = 8080;
            }
            continue;
        }
        if (arg == "--db" && i + 1 < argc) {
            config.dbPath = argv[++i];
            continue;
        }
        if (arg == "--tls-cert" && i + 1 < argc) {
            config.tlsCert = argv[++i];
            continue;
        }
        if (arg == "--tls-key" && i + 1 < argc) {
            config.tlsKey = argv[++i];
            continue;
        }
        if (arg == "--force-https") {
            config.forceHttps = true;
            continue;
        }
        // Positional argument: treat as db path if not yet set
        if (config.dbPath.empty() && arg[0] != '-') {
            config.dbPath = arg;
        }
    }

    // ------------------------------------------------------------------
    // 3. Apply final fallback for db path
    // ------------------------------------------------------------------
    if (config.dbPath.empty()) {
        config.dbPath = "opensylab.db";
    }

    std::cout << "Verwende Datenbank: " << config.dbPath << "\n" << std::endl;

    // ------------------------------------------------------------------
    // 4. Open database
    // ------------------------------------------------------------------
    std::shared_ptr<opensylab::db::IDatabase> database;

    // Audit HMAC key — must be set before open() to ensure persistent chain.
    const char* auditKeyEnv = std::getenv("OPENSYLAB_AUDIT_HMAC_KEY");
    const std::string auditHmacKey = (auditKeyEnv && *auditKeyEnv)
                                     ? std::string(auditKeyEnv) : "";

    if (auditHmacKey.empty()) {
        std::cerr << "FEHLER: OPENSYLAB_AUDIT_HMAC_KEY ist nicht gesetzt.\n"
                  << "  Setzen Sie: export OPENSYLAB_AUDIT_HMAC_KEY=<sicherer-key-min-32-zeichen>\n"
                  << "  Generieren mit: openssl rand -hex 32\n";
        return 1;
    }
    if (auditHmacKey.size() < 32) {
        std::cerr << "FEHLER: OPENSYLAB_AUDIT_HMAC_KEY ist zu kurz (min. 32 Zeichen erforderlich).\n"
                  << "  Generieren mit: openssl rand -hex 32\n";
        return 1;
    }

    if (config.dbBackend == "postgresql") {
        std::cerr << "FEHLER: PostgreSQL-Backend unterstuetzt noch kein Audit-HMAC (geplant fuer v1.1).\n"
                  << "  Verwenden Sie das SQLite-Backend (Standard) bis zur vollstaendigen Implementierung.\n";
        return 1;
    } else {
        database = std::make_shared<opensylab::db::Database>(config.dbPath);
    }

    database->setAuditHmacKey(auditHmacKey);

    if (!database->open()) {
        std::cerr << "FEHLER: Kann Datenbank nicht öffnen!\n";
        std::cerr << "Details: " << database->getLastError() << std::endl;
        return 1;
    }

    if (!database->initializeSchema()) {
        std::cerr << "FEHLER: Kann Datenbankschema nicht initialisieren!\n";
        std::cerr << "Details: " << database->getLastError() << std::endl;
        database->close();
        return 1;
    }

    std::cout << "Datenbank erfolgreich initialisiert.\n" << std::endl;

    // ------------------------------------------------------------------
    // 5. Run API server or CLI
    // ------------------------------------------------------------------
    if (runApi) {
        if (config.forceHttps && (config.tlsCert.empty() || config.tlsKey.empty())) {
            std::cerr << "FEHLER: --force-https requires TLS configuration.\n"
                      << "  Provide --tls-cert and --tls-key or set\n"
                      << "  OPENSYLAB_TLS_CERT and OPENSYLAB_TLS_KEY.\n";
            database->close();
            return 1;
        }

        std::cout << "API-Server wird auf Port " << config.apiPort
                  << " gestartet...\n"
                  << std::endl;
        opensylab::api::ApiServer server(database, config.apiPort);

        if (!config.tlsCert.empty() && !config.tlsKey.empty()) {
            if (!server.enableTls(config.tlsCert, config.tlsKey)) {
                std::cerr << "WARNUNG: TLS konnte nicht aktiviert werden. Starte ohne TLS.\n";
                if (config.forceHttps) {
                    std::cerr << "FEHLER: --force-https erfordert funktionierendes TLS.\n";
                    database->close();
                    return 1;
                }
            } else {
                std::cout << "TLS aktiviert.\n" << std::endl;
            }
        }

        if (!server.run()) {
            std::cerr << "FEHLER: API-Server konnte nicht gestartet werden.\n";
            database->close();
            return 1;
        }
    } else {
        try {
            auto cli = std::make_unique<opensylab::utils::CliInterface>(database);
            cli->run();
        } catch (const std::exception& e) {
            std::cerr << "FEHLER: Unerwarteter Fehler im CLI: " << e.what()
                      << std::endl;
            database->close();
            return 1;
        }
    }

    database->close();
    std::cout << "\nDatenbank geschlossen. OpenSylab wurde beendet.\n"
              << std::endl;

    return 0;
}
