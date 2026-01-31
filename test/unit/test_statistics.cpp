/**
 * @file test_statistics.cpp
 * @brief Unit-Tests für Statistik-Queries
 */

#include "db/Database.h"
#include "test_macros.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
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

bool hasStatus(const std::vector<Database::StatusCount> &entries,
               const std::string &status) {
  for (const auto &entry : entries) {
    if (entry.status == status) {
      return true;
    }
  }
  return false;
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
  ASSERT_TRUE(hasStatus(sampleStats.byStatus,
                        Sample::statusToString(Sample::Status::IN_ANALYSIS)));
  ASSERT_TRUE(hasStatus(sampleStats.byStatus,
                        Sample::statusToString(Sample::Status::ANALYZED)));
  ASSERT_EQ(countFor(sampleStats.byStatus,
                     Sample::statusToString(Sample::Status::IN_ANALYSIS)),
            0);
  ASSERT_EQ(countFor(sampleStats.byStatus,
                     Sample::statusToString(Sample::Status::ANALYZED)),
            0);

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
  ASSERT_TRUE(hasStatus(orderStats.byStatus,
                        Order::statusToString(Order::Status::COMPLETED)));
  ASSERT_TRUE(hasStatus(orderStats.byStatus,
                        Order::statusToString(Order::Status::VALIDATED)));
  ASSERT_EQ(countFor(orderStats.byStatus,
                     Order::statusToString(Order::Status::COMPLETED)),
            0);
  ASSERT_EQ(countFor(orderStats.byStatus,
                     Order::statusToString(Order::Status::VALIDATED)),
            0);

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
  ASSERT_TRUE(hasStatus(resultStats.byStatus,
                        TestResult::statusToString(TestResult::Status::PENDING)));
  ASSERT_TRUE(hasStatus(resultStats.byStatus,
                        TestResult::statusToString(TestResult::Status::REJECTED)));
  ASSERT_TRUE(hasStatus(resultStats.byStatus,
                        TestResult::statusToString(TestResult::Status::REPEATED)));
  ASSERT_EQ(countFor(resultStats.byStatus,
                     TestResult::statusToString(TestResult::Status::PENDING)),
            0);
  ASSERT_EQ(countFor(resultStats.byStatus,
                     TestResult::statusToString(TestResult::Status::REJECTED)),
            0);
  ASSERT_EQ(countFor(resultStats.byStatus,
                     TestResult::statusToString(TestResult::Status::REPEATED)),
            0);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_statistics_FilteredCounts() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample s1("S10", "P10");
  s1.setRegistrationDate(1000);
  Sample s2("S20", "P20");
  s2.setStatus(Sample::Status::VALIDATED);
  s2.setRegistrationDate(2000);
  ASSERT_TRUE(db.createSample(s1));
  ASSERT_TRUE(db.createSample(s2));

  Order o1("O10", "S10", "GLU");
  o1.setRequestedDate(1000);
  Order o2("O20", "S20", "HB");
  o2.setStatus(Order::Status::IN_PROGRESS);
  o2.setRequestedDate(2000);
  ASSERT_TRUE(db.createOrder(o1));
  ASSERT_TRUE(db.createOrder(o2));

  auto order1 = db.getOrderByOrderId("O10");
  auto order2 = db.getOrderByOrderId("O20");
  ASSERT_NOT_NULL(order1);
  ASSERT_NOT_NULL(order2);

  TestResult r1("R10", order1->getId(), "GLU");
  r1.setValue("5.2");
  r1.setUnit("mg/dL");
  r1.setStatus(TestResult::Status::ENTERED);
  r1.setMeasuredDate(1000);
  TestResult r2("R20", order2->getId(), "HB");
  r2.setValue("12.0");
  r2.setUnit("g/dL");
  r2.setStatus(TestResult::Status::VALIDATED);
  r2.setMeasuredDate(2000);
  ASSERT_TRUE(db.createTestResult(r1));
  ASSERT_TRUE(db.createTestResult(r2));

  Database::StatsFilter dateFilter;
  dateFilter.fromDate = 1500;
  dateFilter.toDate = 2500;
  const auto sampleDateStats = db.getSampleStats(dateFilter);
  ASSERT_EQ(sampleDateStats.total, 1);
  const auto orderDateStats = db.getOrderStats(dateFilter);
  ASSERT_EQ(orderDateStats.total, 1);
  const auto resultDateStats = db.getResultStats(dateFilter);
  ASSERT_EQ(resultDateStats.total, 1);

  Database::StatsFilter statusFilter;
  statusFilter.status = Sample::statusToString(Sample::Status::VALIDATED);
  const auto sampleStatusStats = db.getSampleStats(statusFilter);
  ASSERT_EQ(sampleStatusStats.total, 1);
  ASSERT_EQ(countFor(sampleStatusStats.byStatus,
                     Sample::statusToString(Sample::Status::VALIDATED)),
            1);
  ASSERT_TRUE(hasStatus(sampleStatusStats.byStatus,
                        Sample::statusToString(Sample::Status::REGISTERED)));
  ASSERT_EQ(countFor(sampleStatusStats.byStatus,
                     Sample::statusToString(Sample::Status::REGISTERED)),
            0);

  Database::StatsFilter orderStatusFilter;
  orderStatusFilter.status =
      Order::statusToString(Order::Status::IN_PROGRESS);
  const auto orderStatusStats = db.getOrderStats(orderStatusFilter);
  ASSERT_EQ(orderStatusStats.total, 1);
  ASSERT_EQ(countFor(orderStatusStats.byStatus,
                     Order::statusToString(Order::Status::IN_PROGRESS)),
            1);
  ASSERT_TRUE(hasStatus(orderStatusStats.byStatus,
                        Order::statusToString(Order::Status::REQUESTED)));
  ASSERT_EQ(countFor(orderStatusStats.byStatus,
                     Order::statusToString(Order::Status::REQUESTED)),
            0);

  Database::StatsFilter resultStatusFilter;
  resultStatusFilter.status =
      TestResult::statusToString(TestResult::Status::VALIDATED);
  const auto resultStatusStats = db.getResultStats(resultStatusFilter);
  ASSERT_EQ(resultStatusStats.total, 1);
  ASSERT_EQ(countFor(resultStatusStats.byStatus,
                     TestResult::statusToString(TestResult::Status::VALIDATED)),
            1);
  ASSERT_TRUE(hasStatus(resultStatusStats.byStatus,
                        TestResult::statusToString(TestResult::Status::ENTERED)));
  ASSERT_EQ(countFor(resultStatusStats.byStatus,
                     TestResult::statusToString(TestResult::Status::ENTERED)),
            0);

  db.close();
  std::remove(dbPath.c_str());
  return true;
}

bool test_statistics_ExportReport() {
  std::string dbPath = uniqueDbPath();
  Database db(dbPath);
  ASSERT_TRUE(db.open());
  ASSERT_TRUE(db.initializeSchema());

  Sample s1("SX", "PX");
  ASSERT_TRUE(db.createSample(s1));
  Order o1("OX", "SX", "GLU");
  ASSERT_TRUE(db.createOrder(o1));
  auto order = db.getOrderByOrderId("OX");
  ASSERT_NOT_NULL(order);
  TestResult r1("RX", order->getId(), "GLU");
  r1.setValue("5.0");
  r1.setUnit("mg/dL");
  ASSERT_TRUE(db.createTestResult(r1));

  const std::string reportPath = "test_stats_export.csv";
  Database::StatsFilter sampleFilter;
  Database::StatsFilter orderFilter;
  Database::StatsFilter resultFilter;
  ASSERT_TRUE(db.exportStatsReportToCsv(reportPath, sampleFilter, orderFilter,
                                        resultFilter, "tester"));

  std::ifstream input(reportPath);
  ASSERT_TRUE(input.is_open());
  std::string content((std::istreambuf_iterator<char>(input)),
                      std::istreambuf_iterator<char>());
  input.close();
  ASSERT_NE(content.find("entity,status,count"), std::string::npos);
  ASSERT_NE(content.find("samples,TOTAL"), std::string::npos);
  ASSERT_NE(content.find("orders,TOTAL"), std::string::npos);
  ASSERT_NE(content.find("results,TOTAL"), std::string::npos);

  auto entries =
      db.getAuditLogByEntity(opensylab::core::AuditEntry::EntityType::SYSTEM,
                              "stats_report");
  ASSERT_FALSE(entries.empty());

  std::remove(reportPath.c_str());
  db.close();
  std::remove(dbPath.c_str());
  return true;
}

void registerStatisticsTests() {
  registerTest("Statistics::StatsCounts", test_statistics_StatsCounts);
  registerTest("Statistics::FilteredCounts", test_statistics_FilteredCounts);
  registerTest("Statistics::ExportReport", test_statistics_ExportReport);
}
