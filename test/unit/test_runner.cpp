/**
 * @file test_runner.cpp
 * @brief Einfacher Test-Runner für OpenSylab v0.2
 *
 * Dieser Test-Runner läuft alle Unit-Tests und gibt das Ergebnis aus.
 * Wir verwenden ein minimalistisches Test-Framework ohne externe
 * Abhängigkeiten.
 */

#include <functional>
#include <iostream>
#include <string>
#include <vector>

// Test-Framework - sehr einfach
struct Test {
  std::string name;
  std::function<bool()> func;
};

static std::vector<Test> tests;

void registerTest(const std::string &name, std::function<bool()> func) {
  tests.push_back({name, func});
}

#define TEST(name)                                                             \
  bool test_##name();                                                          \
  namespace {                                                                  \
  struct TestRegistrar_##name {                                                \
    TestRegistrar_##name() { registerTest(#name, test_##name); }               \
  } testRegistrar_##name;                                                      \
  }                                                                            \
  bool test_##name()

#define ASSERT_TRUE(expr)                                                      \
  if (!(expr)) {                                                               \
    std::cerr << "  ✗ Assertion failed: " << #expr << "\n";                    \
    return false;                                                              \
  }

#define ASSERT_FALSE(expr)                                                     \
  if (expr) {                                                                  \
    std::cerr << "  ✗ Assertion failed (expected false): " << #expr << "\n";   \
    return false;                                                              \
  }

#define ASSERT_EQ(a, b)                                                        \
  if ((a) != (b)) {                                                            \
    std::cerr << "  ✗ Assertion failed: " << #a << " == " << #b << "\n";       \
    std::cerr << "    Expected: " << (b) << ", Got: " << (a) << "\n";          \
    return false;                                                              \
  }

#define ASSERT_NE(a, b)                                                        \
  if ((a) == (b)) {                                                            \
    std::cerr << "  ✗ Assertion failed: " << #a << " != " << #b << "\n";       \
    return false;                                                              \
  }

// Externe Test-Funktionen (aus anderen Dateien)
extern void registerSampleTests();
extern void registerOrderTests();
extern void registerTestResultTests();
extern void registerDatabaseTests();
extern void registerApiTests();
extern void registerCsvImportTests();
extern void registerCsvResultImportTests();
extern void registerHl7Tests();
extern void registerFhirTests();
extern void registerStatisticsTests();
extern void registerUtilsTests();

int main() {
  std::cout << "\n";
  std::cout
      << "╔═══════════════════════════════════════════════════════════╗\n";
  std::cout
      << "║          OpenSylab v0.2 - Unit Test Suite                 ║\n";
  std::cout
      << "╚═══════════════════════════════════════════════════════════╝\n";
  std::cout << "\n";

  // Tests registrieren
  registerSampleTests();
  registerOrderTests();
  registerTestResultTests();
  registerDatabaseTests();
  registerApiTests();
  registerCsvImportTests();
  registerCsvResultImportTests();
  registerHl7Tests();
  registerFhirTests();
  registerStatisticsTests();
  registerUtilsTests();

  int passed = 0;
  int failed = 0;

  std::cout << "Running " << tests.size() << " tests...\n\n";

  for (const auto &test : tests) {
    std::cout << "TEST: " << test.name << "... ";
    std::cout.flush();

    bool result = test.func();

    if (result) {
      std::cout << "✓ PASSED\n";
      passed++;
    } else {
      std::cout << "✗ FAILED\n";
      failed++;
    }
  }

  std::cout << "\n";
  std::cout << "═══════════════════════════════════════════════════════════\n";
  std::cout << "Results:\n";
  std::cout << "  ✓ Passed: " << passed << "\n";
  std::cout << "  ✗ Failed: " << failed << "\n";
  std::cout << "  Total:   " << tests.size() << "\n";
  std::cout << "═══════════════════════════════════════════════════════════\n";
  std::cout << "\n";

  return (failed == 0) ? 0 : 1;
}
