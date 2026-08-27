#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>

namespace test {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

// Defined in test_main.cpp - single definition
std::vector<TestCase>& get_tests();

static int register_test(const std::string& name, std::function<void()> func) {
    get_tests().push_back({name, std::move(func)});
    return 0;
}

inline int run_all() {
    int passed = 0;
    int failed = 0;
    int total = static_cast<int>(get_tests().size());

    for (auto& tc : get_tests()) {
        try {
            tc.func();
            passed++;
            std::cout << "  PASS: " << tc.name << "\n";
        } catch (const std::exception& e) {
            failed++;
            std::cout << "  FAIL: " << tc.name << " - " << e.what() << "\n";
        }
    }

    std::cout << "\n" << passed << "/" << total << " tests passed";
    if (failed > 0) {
        std::cout << " (" << failed << " FAILED)";
    }
    std::cout << "\n";
    return failed;
}

} // namespace test

#define TEST(name) \
    static void test_##name(); \
    static int reg_##name = ::test::register_test(#name, test_##name); \
    static void test_##name()

#define ASSERT_TRUE(cond) \
    do { if (!(cond)) throw std::runtime_error("Assertion failed: " #cond); } while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) \
    do { if ((a) != (b)) throw std::runtime_error("Assertion failed: " #a " == " #b); } while(0)

#define ASSERT_NE(a, b) \
    do { if ((a) == (b)) throw std::runtime_error("Assertion failed: " #a " != " #b); } while(0)

#define ASSERT_CONTAINS(str, sub) \
    do { if ((str).find(sub) == std::string::npos) \
        throw std::runtime_error("Assertion failed: \"" + (str) + "\" contains \"" + (sub) + "\""); } while(0)
