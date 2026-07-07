# OpenSylab - Testing Guide

## Overview

OpenSylab v1.1.0 includes a simple, self-implemented test framework with no external dependencies. This enables fast testing without complex setup processes. The test suite comprises 236 automated backend unit tests (including API-layer compliance tests) as well as 46 frontend tests via Vitest.

**Version:** 1.1.0
**Last Updated:** 2026-07-07
**Backend Tests:** 236 unit tests
**Frontend Tests:** 46 tests (Vitest + React Testing Library)

## Test Framework

### Architecture

The test framework consists of:
- **test_runner.cpp**: Main test runner with macros for tests
- **test_sample.cpp**: Unit tests for the Sample class
- **test_order.cpp**: Unit tests for the Order class
- **test_testresult.cpp**: Unit tests for the TestResult class
- **test_database.cpp**: Unit tests for the Database class
- **test_csvimport.cpp**: Unit tests for the CsvImport class
- **test_csvresultimport.cpp**: Unit tests for the CsvResultImport class

### Test Macros

```cpp
TEST(TestName) {
    // Test code
    ASSERT_TRUE(expression);
    ASSERT_FALSE(expression);
    ASSERT_EQ(actual, expected);
    ASSERT_NE(actual, unexpected);
    return true;  // Test successful
}
```

## Running Tests

### Option 1: Build & Test Script (Recommended)

```bash
chmod +x test_and_build.sh
./test_and_build.sh
```

This script:
1. Compiles the entire project
2. Runs all unit tests
3. Prints detailed results

### Option 2: Manual

```bash
cd build
make
make test
# Or for detailed output:
ctest --output-on-failure
```

### Option 3: Run the Test Binary Directly

```bash
./build/opensylab_tests
```

## Test Overview

### Sample Tests (6 tests)
- ✓ DefaultConstructor
- ✓ ParameterizedConstructor
- ✓ SettersAndGetters
- ✓ StatusToString
- ✓ StringToStatus
- ✓ StatusRoundtrip

### Order Tests (8 tests)
- ✓ DefaultConstructor
- ✓ ParameterizedConstructor
- ✓ SettersAndGetters
- ✓ StatusToString
- ✓ StringToStatus
- ✓ PriorityToString
- ✓ StringToPriority
- ✓ StatusRoundtrip

### TestResult Tests (10 tests)
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

### Database Tests (26 tests)
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

### CsvImport Tests (5 tests)
- ✓ ImportValidCsv
- ✓ ImportWithMissingFields
- ✓ ImportWithEmptyId
- ✓ ImportWithWhitespaceId
- ✓ ImportMixedValidAndInvalid

### CsvResultImport Tests (5 tests)
- ✓ ImportValidCsv
- ✓ ImportWithMissingFields
- ✓ ImportWithInvalidValue
- ✓ ImportWithFlags
- ✓ ImportMixedValidAndInvalid

## Expected Output

```
╔═══════════════════════════════════════════════════════════╗
║          OpenSylab v1.1.0 - Unit Test Suite               ║
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

## Test Coverage

Currently tested:
- ✅ Data models (Sample, Order, TestResult, AuditEntry, User)
- ✅ Database operations (CRUD for all entities)
- ✅ CSV import of sample data
- ✅ CSV import of analysis results
- ✅ Audit trail logging
- ✅ User authentication
- ✅ Error handling
- ✅ API endpoints (Read/Write via `ApiRouter::handleRequest`, incl.
  ISO 15189 compliance branches: immutability 409, status transitions,
  ADMIN-only VALIDATE 403, delete guards)
- ✅ JWT authentication & RBAC (auth-required and rejected-role cases)
- ✅ HL7/FHIR parser, statistics aggregation, migrations, audit hash chain
- ✅ Frontend components (46 tests via Vitest + React Testing Library)
- ✅ End-to-end workflows (8 Playwright tests: login, navigation, API read,
  mobile drawer — start a real backend + frontend, run in CI)

Not yet tested:
- ⏳ Performance under load
- ⏳ CLI interface (hard to test automatically)

## Adding New Tests

### 1. Create a Test File

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

### 2. Add to test/CMakeLists.txt

```cmake
set(TEST_SOURCES
    unit/test_sample.cpp
    unit/test_database.cpp
    unit/test_csvimport.cpp
    unit/test_mymodule.cpp  # New
    unit/test_runner.cpp
)
```

### 3. Register in test_runner.cpp

```cpp
extern void registerMyModuleTests();

int main() {
    registerSampleTests();
    registerDatabaseTests();
    registerCsvImportTests();
    registerMyModuleTests();  // New
    ...
}
```

## Debugging Failed Tests

When a test fails, the exact line and the reason are printed:

```
TEST: Sample::InvalidTest... ✗ FAILED
  ✗ Assertion failed: sample.getId() == 42
    Expected: 42, Got: 0
```

## Best Practices

1. **One test = one piece of functionality**: Each test should test only one thing
2. **Meaningful names**: `TEST(EmptyIdShouldBeRejected)` instead of `TEST(Test1)`
3. **Cleanup**: Delete temporary files/DBs at the end
4. **Independence**: Tests should be able to run independently of one another
5. **Fast**: Tests should be quick to run (<1s per test)

## Continuous Integration

For CI/CD pipelines:

```yaml
# .github/workflows/test.yml
- name: Build and Test
  run: |
    ./test_and_build.sh
```

The exit code is 0 on success, 1 on failure.

## Already Delivered (as of v1.1.0)

- **Frontend unit tests** — 46 tests via Vitest + React Testing Library (components, auth context, protected routes).
- **RBAC & JWT auth tests** — auth-required and rejected-role cases in the backend.
- **End-to-end tests** — 8 Playwright tests (login, navigation, API read, mobile drawer), starting a real backend + frontend.
- **CI/CD pipeline** — GitHub Actions runs backend, frontend, and E2E tests on every push/PR.

### Running Frontend Tests

```bash
cd frontend
npm test              # Vitest unit tests
npm test -- --watch   # Watch mode
npm run test:e2e      # Playwright E2E (starts backend + frontend itself)
```

### Manual API Smoke Test

```bash
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin"}'
```

## Planned

- **Code coverage reporting** (gcov/lcov for the C++ backend, `npm run test:coverage` for the frontend).
- **Load / performance tests** (e.g. k6).
- **API contract tests** against `docs/openapi.yaml`.
