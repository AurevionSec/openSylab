/**
 * @file test_macros.h
 * @brief Zentrale Test-Makros für OpenSylab Unit-Tests
 */

#ifndef OPENSYLAB_TEST_MACROS_H
#define OPENSYLAB_TEST_MACROS_H

#include <iostream>
#include <functional>
#include <string>

// Externe Registrierungsfunktion (definiert in test_runner.cpp)
extern void registerTest(const std::string& name, std::function<bool()> func);

// Assertion-Makros
#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        std::cerr << "  ✗ Assertion failed: " << #expr << "\n"; \
        return false; \
    }

#define ASSERT_FALSE(expr) \
    if (expr) { \
        std::cerr << "  ✗ Assertion failed (expected false): " << #expr << "\n"; \
        return false; \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cerr << "  ✗ Assertion failed: " << #a << " == " << #b << "\n"; \
        return false; \
    }

#define ASSERT_NE(a, b) \
    if ((a) == (b)) { \
        std::cerr << "  ✗ Assertion failed: " << #a << " != " << #b << "\n"; \
        return false; \
    }

#define ASSERT_NULL(ptr) \
    if ((ptr) != nullptr) { \
        std::cerr << "  ✗ Assertion failed: " << #ptr << " should be null\n"; \
        return false; \
    }

#define ASSERT_NOT_NULL(ptr) \
    if ((ptr) == nullptr) { \
        std::cerr << "  ✗ Assertion failed: " << #ptr << " should not be null\n"; \
        return false; \
    }

#endif // OPENSYLAB_TEST_MACROS_H
