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

bool test_utils_AutoRefreshOptIn() {
  ASSERT_FALSE(CliInterface::isAutoRefreshEnabled(""));
  ASSERT_FALSE(CliInterface::isAutoRefreshEnabled("n"));
  ASSERT_FALSE(CliInterface::isAutoRefreshEnabled("nein"));
  ASSERT_TRUE(CliInterface::isAutoRefreshEnabled("j"));
  ASSERT_TRUE(CliInterface::isAutoRefreshEnabled("ja"));
  ASSERT_TRUE(CliInterface::isAutoRefreshEnabled("yes"));
  return true;
}
} // namespace

void registerUtilsTests() {
  registerTest("Utils::AutoRefreshInterval", test_utils_AutoRefreshInterval);
  registerTest("Utils::AutoRefreshOptIn", test_utils_AutoRefreshOptIn);
}
