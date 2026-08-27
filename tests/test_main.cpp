#include "test_framework.hpp"

// Define the global test registry
namespace test {
std::vector<TestCase>& get_tests() {
    static std::vector<TestCase> tests;
    return tests;
}
}

// Include all test files into this single translation unit
// This ensures static initializers share the same registry
#include "test_http_parser.cpp"
#include "test_router.cpp"
#include "test_thread_pool.cpp"
#include "test_static_files.cpp"

int main() {
    std::cout << "=== C++ HTTP Server Test Suite ===\n\n";
    int result = test::run_all();
    return result;
}
