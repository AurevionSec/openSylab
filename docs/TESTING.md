# OpenSylab - Testing Guide

## Übersicht

OpenSylab v1.0.0 enthält ein einfaches, selbst implementiertes Test-Framework ohne externe Abhängigkeiten. Dies ermöglicht schnelles Testen ohne komplexe Setup-Prozesse. Die Test-Suite umfasst 236 automatisierte Backend Unit-Tests (inkl. API-Layer-Compliance-Tests) sowie 46 Frontend-Tests via Vitest.

**Version:** 1.0.0
**Last Updated:** 2026-07-05
**Backend Tests:** 236 Unit-Tests
**Frontend Tests:** 46 Tests (Vitest + React Testing Library)

## Test-Framework

### Architektur

Das Test-Framework besteht aus:
- **test_runner.cpp**: Haupt-Test-Runner mit Makros für Tests
- **test_sample.cpp**: Unit-Tests für Sample-Klasse
- **test_order.cpp**: Unit-Tests für Order-Klasse
- **test_testresult.cpp**: Unit-Tests für TestResult-Klasse
- **test_database.cpp**: Unit-Tests für Database-Klasse
- **test_csvimport.cpp**: Unit-Tests für CsvImport-Klasse
- **test_csvresultimport.cpp**: Unit-Tests für CsvResultImport-Klasse

### Test-Makros

```cpp
TEST(TestName) {
    // Test-Code
    ASSERT_TRUE(expression);
    ASSERT_FALSE(expression);
    ASSERT_EQ(actual, expected);
    ASSERT_NE(actual, unexpected);
    return true;  // Test erfolgreich
}
```

## Tests ausführen

### Option 1: Build & Test-Skript (Empfohlen)

```bash
chmod +x test_and_build.sh
./test_and_build.sh
```

Dieses Skript:
1. Kompiliert das gesamte Projekt
2. Führt alle Unit-Tests aus
3. Gibt detaillierte Ergebnisse aus

### Option 2: Manuell

```bash
cd build
make
make test
# Oder für detaillierte Ausgabe:
ctest --output-on-failure
```

### Option 3: Test-Binary direkt ausführen

```bash
./build/opensylab_tests
```

## Test-Übersicht

### Sample-Tests (6 Tests)
- ✓ DefaultConstructor
- ✓ ParameterizedConstructor
- ✓ SettersAndGetters
- ✓ StatusToString
- ✓ StringToStatus
- ✓ StatusRoundtrip

### Order-Tests (8 Tests)
- ✓ DefaultConstructor
- ✓ ParameterizedConstructor
- ✓ SettersAndGetters
- ✓ StatusToString
- ✓ StringToStatus
- ✓ PriorityToString
- ✓ StringToPriority
- ✓ StatusRoundtrip

### TestResult-Tests (10 Tests)
- ✓ DefaultConstructor
- ✓ ParameterizedConstructor
- ✓ SettersAndGetters
- ✓ StatusToString
- ✓ StringToStatus
- ✓ FlagToString
- ✓ StringToFlag
- ✓ CalculateFlag_Normal
- ✓ CalculateFlag_Abnormal
- ✓ CalculateFlag_NoRange

### Database-Tests (26 Tests)
- ✓ OpenAndClose
- ✓ InitializeSchema
- ✓ CreateSample
- ✓ GetSampleByBarcode
- ✓ GetAllSamples
- ✓ GetAllSamples_EmptyDatabase
- ✓ UpdateSample
- ✓ CreateOrder
- ✓ GetOrder
- ✓ GetOrderByOrderId
- ✓ GetOrdersBySampleId
- ✓ GetAllOrders
- ✓ UpdateOrder
- ✓ DeleteOrder
- ✓ CreateTestResult
- ✓ GetTestResult
- ✓ GetTestResultByResultId
- ✓ GetTestResultsByOrderId
- ✓ GetAllTestResults
- ✓ UpdateTestResult
- ✓ DeleteTestResult
- ✓ LogAudit
- ✓ GetAuditLog
- ✓ GetAuditLogByEntity
- ✓ CreateUser
- ✓ AuthenticateUser

### CsvImport-Tests (5 Tests)
- ✓ ImportValidCsv
- ✓ ImportWithMissingFields
- ✓ ImportWithEmptyId
- ✓ ImportWithWhitespaceId
- ✓ ImportMixedValidAndInvalid

### CsvResultImport-Tests (5 Tests)
- ✓ ImportValidCsv
- ✓ ImportWithMissingFields
- ✓ ImportWithInvalidValue
- ✓ ImportWithFlags
- ✓ ImportMixedValidAndInvalid

## Erwartete Ausgabe

```
╔═══════════════════════════════════════════════════════════╗
║          OpenSylab v1.0.0 - Unit Test Suite               ║
╚═══════════════════════════════════════════════════════════╝

Running 236 tests...

TEST: Sample::DefaultConstructor... ✓ PASSED
TEST: Sample::ParameterizedConstructor... ✓ PASSED
...
TEST: CsvResultImport::ImportMixedValidAndInvalid... ✓ PASSED

═══════════════════════════════════════════════════════════
Results:
  ✓ Passed: 236
  ✗ Failed: 0
  Total:   236
═══════════════════════════════════════════════════════════
```

## Test-Coverage

Aktuell getestet:
- ✅ Datenmodelle (Sample, Order, TestResult, AuditEntry, User)
- ✅ Datenbank-Operationen (CRUD für alle Entitäten)
- ✅ CSV-Import von Probendaten
- ✅ CSV-Import von Analyseergebnissen
- ✅ Audit-Trail Logging
- ✅ Benutzer-Authentifizierung
- ✅ Fehlerbehandlung
- ✅ API-Endpoints (Read/Write via `ApiRouter::handleRequest`, inkl.
  ISO-15189-Compliance-Zweige: Immutability-409, Status-Transitionen,
  ADMIN-only-VALIDATE-403, Delete-Guards)
- ✅ JWT-Authentifizierung & RBAC (Auth-Required- und abgelehnte-Rollen-Fälle)
- ✅ HL7/FHIR-Parser, Statistik-Aggregation, Migrations, Audit-Hash-Chain
- ✅ Frontend Components (46 Tests via Vitest + React Testing Library)
- ✅ End-to-End Workflows (8 Playwright-Tests: Login, Navigation, API-Read,
  Mobile-Drawer — starten echtes Backend + Frontend, laufen im CI)

Noch nicht getestet:
- ⏳ Performance unter Last
- ⏳ CLI-Interface (schwierig automatisch zu testen)

## Neue Tests hinzufügen

### 1. Test-Datei erstellen

```cpp
// test/unit/test_mymodule.cpp
#include "mymodule.h"
extern void registerTest(const std::string& name, std::function<bool()> func);

#define TEST(name) \
    bool test_mymodule_##name(); \
    namespace { \
        struct TestRegistrar_mymodule_##name { \
            TestRegistrar_mymodule_##name() { \
                registerTest("MyModule::" #name, test_mymodule_##name); \
            } \
        } testRegistrar_mymodule_##name; \
    } \
    bool test_mymodule_##name()

TEST(MyFirstTest) {
    ASSERT_TRUE(1 + 1 == 2);
    return true;
}

void registerMyModuleTests() {}
```

### 2. In test/CMakeLists.txt hinzufügen

```cmake
set(TEST_SOURCES
    unit/test_sample.cpp
    unit/test_database.cpp
    unit/test_csvimport.cpp
    unit/test_mymodule.cpp  # Neu
    unit/test_runner.cpp
)
```

### 3. In test_runner.cpp registrieren

```cpp
extern void registerMyModuleTests();

int main() {
    registerSampleTests();
    registerDatabaseTests();
    registerCsvImportTests();
    registerMyModuleTests();  // Neu
    ...
}
```

## Debugging fehlgeschlagener Tests

Bei einem fehlgeschlagenen Test wird die genaue Zeile und der Grund ausgegeben:

```
TEST: Sample::InvalidTest... ✗ FAILED
  ✗ Assertion failed: sample.getId() == 42
    Expected: 42, Got: 0
```

## Best Practices

1. **Ein Test = Eine Funktionalität**: Jeder Test sollte nur eine Sache testen
2. **Aussagekräftige Namen**: `TEST(EmptyIdShouldBeRejected)` statt `TEST(Test1)`
3. **Cleanup**: Temporäre Dateien/DBs am Ende löschen
4. **Unabhängigkeit**: Tests sollten unabhängig voneinander laufen können
5. **Schnell**: Tests sollten schnell ausführbar sein (<1s pro Test)

## Kontinuierliche Integration

Für CI/CD-Pipelines:

```yaml
# .github/workflows/test.yml
- name: Build and Test
  run: |
    ./test_and_build.sh
```

Der Exit-Code ist 0 bei Erfolg, 1 bei Fehler.

## Bereits geliefert (Stand v1.0.0)

- **Frontend-Unit-Tests** — 46 Tests via Vitest + React Testing Library (Components, Auth-Context, Protected Routes).
- **RBAC- & JWT-Auth-Tests** — Auth-Required- und abgelehnte-Rollen-Fälle im Backend.
- **End-to-End-Tests** — 8 Playwright-Tests (Login, Navigation, API-Read, Mobile-Drawer), starten echtes Backend + Frontend.
- **CI/CD-Pipeline** — GitHub Actions führt Backend-, Frontend- und E2E-Tests bei jedem Push/PR aus.

### Frontend-Tests ausführen

```bash
cd frontend
npm test              # Vitest Unit-Tests
npm test -- --watch   # Watch-Mode
npm run test:e2e      # Playwright E2E (startet Backend + Frontend selbst)
```

### Manueller API-Smoke-Test

```bash
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin"}'
```

## Geplant

- **Code-Coverage-Reporting** (gcov/lcov für das C++-Backend, `npm run test:coverage` fürs Frontend).
- **Last-/Performance-Tests** (z. B. k6).
- **API-Contract-Tests** gegen `docs/openapi.yaml`.
