/**
 * @file test_utils.cpp
 * @brief Unit-Tests für Utility-Funktionen
 */

#include "test_macros.h"
#include "utils/CliInterface.h"

using opensylab::utils::CliInterface;

namespace {
bool test_utils_AutoRefreshInterval() {
  ASSERT_EQ(CliInterface::autoRefreshIntervalSeconds(), 5);
  return true;
}
} // namespace

void registerUtilsTests() {
  registerTest("Utils::AutoRefreshInterval", test_utils_AutoRefreshInterval);
}
