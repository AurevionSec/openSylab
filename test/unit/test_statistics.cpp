/**
 * @file test_statistics.cpp
 * @brief Unit-Tests für Statistik-Queries
 */

#include "db/Database.h"
#include "test_macros.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sstream>

using namespace opensylab::db;
using namespace opensylab::core;

namespace {
std::string uniqueDbPath() {
  std::ostringstream ss;
  ss << "test_stats_" << std::rand() << "_" << std::time(nullptr) << ".db";
  return ss.str();
}

int countFor(const std::vector<Database::StatusCount> &entries,
             const std::string &status) {
  for (const auto &entry : entries) {
    if (entry.status == status) {
      return entry.count;
    }
  }
  return 0;
}
} // namespace

bool test_statistics_StatsCounts() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample s1("S1", "P1");
  Sample s2("S2", "P2");
  s2.setStatus(Sample::Status::VALIDATED);
  Sample s3("S3", "P3");
  s3.setStatus(Sample::Status::ARCHIVED);
  ASSERT_TRUE(db.createSample(s1));
  ASSERT_TRUE(db.createSample(s2));
  ASSERT_TRUE(db.createSample(s3));

  Order o1("O1", "S1", "GLU");
  Order o2("O2", "S2", "HB");
  o2.setStatus(Order::Status::IN_PROGRESS);
  Order o3("O3", "S3", "CRP");
  o3.setStatus(Order::Status::CANCELLED);
  ASSERT_TRUE(db.createOrder(o1));
  ASSERT_TRUE(db.createOrder(o2));
  ASSERT_TRUE(db.createOrder(o3));

  auto order1 = db.getOrderByOrderId("O1");
  auto order2 = db.getOrderByOrderId("O2");
  ASSERT_NOT_NULL(order1);
  ASSERT_NOT_NULL(order2);

  TestResult r1("R1", order1->getId(), "GLU");
  r1.setValue("5.1");
  r1.setUnit("mg/dL");
  r1.setStatus(TestResult::Status::ENTERED);
  TestResult r2("R2", order2->getId(), "HB");
  r2.setValue("12.3");
  r2.setUnit("g/dL");
  r2.setStatus(TestResult::Status::VALIDATED);
  ASSERT_TRUE(db.createTestResult(r1));
  ASSERT_TRUE(db.createTestResult(r2));

  const auto sampleStats = db.getSampleStats();
  ASSERT_FALSE(db.hasError());
  ASSERT_EQ(sampleStats.total, 3);
  ASSERT_EQ(countFor(sampleStats.byStatus,
                     Sample::statusToString(Sample::Status::REGISTERED)),
            1);
  ASSERT_EQ(countFor(sampleStats.byStatus,
                     Sample::statusToString(Sample::Status::VALIDATED)),
            1);
  ASSERT_EQ(countFor(sampleStats.byStatus,
                     Sample::statusToString(Sample::Status::ARCHIVED)),
            1);

  const auto orderStats = db.getOrderStats();
  ASSERT_FALSE(db.hasError());
  ASSERT_EQ(orderStats.total, 3);
  ASSERT_EQ(countFor(orderStats.byStatus,
                     Order::statusToString(Order::Status::REQUESTED)),
            1);
  ASSERT_EQ(countFor(orderStats.byStatus,
                     Order::statusToString(Order::Status::IN_PROGRESS)),
            1);
  ASSERT_EQ(countFor(orderStats.byStatus,
                     Order::statusToString(Order::Status::CANCELLED)),
            1);

  const auto resultStats = db.getResultStats();
  ASSERT_FALSE(db.hasError());
  ASSERT_EQ(resultStats.total, 2);
  ASSERT_EQ(countFor(resultStats.byStatus,
                     TestResult::statusToString(TestResult::Status::ENTERED)),
            1);
  ASSERT_EQ(
      countFor(resultStats.byStatus,
               TestResult::statusToString(TestResult::Status::VALIDATED)),
      1);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

void registerStatisticsTests() {
  registerTest("Statistics::StatsCounts", test_statistics_StatsCounts);
}
