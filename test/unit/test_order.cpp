/**
 * @file test_order.cpp
 * @brief Unit-Tests für die Order-Klasse
 */

#include "core/Order.h"
#include "test_macros.h"

using namespace opensylab::core;

// Test-Funktionen
bool test_order_DefaultConstructor() {
  Order order;
  ASSERT_EQ(order.getId(), 0);
  ASSERT_TRUE(order.getOrderId().empty());
  ASSERT_TRUE(order.getSampleId().empty());
  ASSERT_TRUE(order.getTestType().empty());
  ASSERT_EQ(order.getStatus(), Order::Status::REQUESTED);
  ASSERT_EQ(order.getPriority(), Order::Priority::NORMAL);
  ASSERT_EQ(order.getCompletedDate(), 0);
  return true;
}

bool test_order_ParameterizedConstructor() {
  Order order("O001", "S001", "Blutbild");
  ASSERT_EQ(order.getOrderId(), "O001");
  ASSERT_EQ(order.getSampleId(), "S001");
  ASSERT_EQ(order.getTestType(), "Blutbild");
  ASSERT_EQ(order.getStatus(), Order::Status::REQUESTED);
  ASSERT_EQ(order.getPriority(), Order::Priority::NORMAL);
  return true;
}

bool test_order_SettersAndGetters() {
  Order order;
  order.setId(42);
  order.setOrderId("O002");
  order.setSampleId("S002");
  order.setTestType("Glucose");
  order.setStatus(Order::Status::IN_PROGRESS);
  order.setPriority(Order::Priority::URGENT);
  order.setRequestedBy("Dr. Test");
  order.setNotes("Testnotiz");

  ASSERT_EQ(order.getId(), 42);
  ASSERT_EQ(order.getOrderId(), "O002");
  ASSERT_EQ(order.getSampleId(), "S002");
  ASSERT_EQ(order.getTestType(), "Glucose");
  ASSERT_EQ(order.getStatus(), Order::Status::IN_PROGRESS);
  ASSERT_EQ(order.getPriority(), Order::Priority::URGENT);
  ASSERT_EQ(order.getRequestedBy(), "Dr. Test");
  ASSERT_EQ(order.getNotes(), "Testnotiz");
  return true;
}

bool test_order_StatusToString() {
  ASSERT_EQ(Order::statusToString(Order::Status::REQUESTED), "Angefordert");
  ASSERT_EQ(Order::statusToString(Order::Status::IN_PROGRESS),
            "In Bearbeitung");
  ASSERT_EQ(Order::statusToString(Order::Status::COMPLETED), "Abgeschlossen");
  ASSERT_EQ(Order::statusToString(Order::Status::VALIDATED), "Validiert");
  ASSERT_EQ(Order::statusToString(Order::Status::CANCELLED), "Storniert");
  return true;
}

bool test_order_StringToStatus() {
  ASSERT_EQ(Order::stringToStatus("Angefordert"), Order::Status::REQUESTED);
  ASSERT_EQ(Order::stringToStatus("REQUESTED"), Order::Status::REQUESTED);
  ASSERT_EQ(Order::stringToStatus("In Bearbeitung"),
            Order::Status::IN_PROGRESS);
  ASSERT_EQ(Order::stringToStatus("Abgeschlossen"), Order::Status::COMPLETED);
  ASSERT_EQ(Order::stringToStatus("Validiert"), Order::Status::VALIDATED);
  ASSERT_EQ(Order::stringToStatus("Storniert"), Order::Status::CANCELLED);
  return true;
}

bool test_order_StatusRoundtrip() {
  for (auto status : {Order::Status::REQUESTED, Order::Status::IN_PROGRESS,
                      Order::Status::COMPLETED, Order::Status::VALIDATED,
                      Order::Status::CANCELLED}) {
    std::string statusStr = Order::statusToString(status);
    Order::Status roundtrip = Order::stringToStatus(statusStr);
    ASSERT_EQ(status, roundtrip);
  }
  return true;
}

bool test_order_PriorityToString() {
  ASSERT_EQ(Order::priorityToString(Order::Priority::NORMAL), "Normal");
  ASSERT_EQ(Order::priorityToString(Order::Priority::URGENT), "Dringend");
  ASSERT_EQ(Order::priorityToString(Order::Priority::EMERGENCY), "Notfall");
  return true;
}

bool test_order_StringToPriority() {
  ASSERT_EQ(Order::stringToPriority("Normal"), Order::Priority::NORMAL);
  ASSERT_EQ(Order::stringToPriority("NORMAL"), Order::Priority::NORMAL);
  ASSERT_EQ(Order::stringToPriority("Dringend"), Order::Priority::URGENT);
  ASSERT_EQ(Order::stringToPriority("URGENT"), Order::Priority::URGENT);
  ASSERT_EQ(Order::stringToPriority("Notfall"), Order::Priority::EMERGENCY);
  ASSERT_EQ(Order::stringToPriority("EMERGENCY"), Order::Priority::EMERGENCY);
  return true;
}

bool test_order_PriorityRoundtrip() {
  for (auto priority : {Order::Priority::NORMAL, Order::Priority::URGENT,
                        Order::Priority::EMERGENCY}) {
    std::string priorityStr = Order::priorityToString(priority);
    Order::Priority roundtrip = Order::stringToPriority(priorityStr);
    ASSERT_EQ(priority, roundtrip);
  }
  return true;
}

bool test_order_CompletedDate() {
  Order order("O003", "S003", "Lipide");
  ASSERT_EQ(order.getCompletedDate(), 0);

  std::time_t now = std::time(nullptr);
  order.setCompletedDate(now);
  ASSERT_EQ(order.getCompletedDate(), now);
  return true;
}

void registerOrderTests() {
  registerTest("Order::DefaultConstructor", test_order_DefaultConstructor);
  registerTest("Order::ParameterizedConstructor",
               test_order_ParameterizedConstructor);
  registerTest("Order::SettersAndGetters", test_order_SettersAndGetters);
  registerTest("Order::StatusToString", test_order_StatusToString);
  registerTest("Order::StringToStatus", test_order_StringToStatus);
  registerTest("Order::StatusRoundtrip", test_order_StatusRoundtrip);
  registerTest("Order::PriorityToString", test_order_PriorityToString);
  registerTest("Order::StringToPriority", test_order_StringToPriority);
  registerTest("Order::PriorityRoundtrip", test_order_PriorityRoundtrip);
  registerTest("Order::CompletedDate", test_order_CompletedDate);
}
