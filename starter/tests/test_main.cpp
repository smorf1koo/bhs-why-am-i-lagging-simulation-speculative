#include <chrono>
#include <cstring>
#include <exception>
#include <iostream>

#include "mini_test.hpp"

#ifndef SPEC_DEFAULT_TRACE
#define SPEC_DEFAULT_TRACE "../data/input_trace.csv"
#endif

std::string& mini_test::trace_path() {
    static std::string p = SPEC_DEFAULT_TRACE;
    return p;
}

int main(int argc, char** argv) {
    std::string filter;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--trace") && i + 1 < argc) {
            mini_test::trace_path() = argv[++i];
        } else {
            filter = argv[i];
        }
    }
    int passed = 0, failed = 0;
    for (const auto& c : mini_test::registry()) {
        if (!filter.empty() && c.name.find(filter) == std::string::npos) continue;
        const auto t0 = std::chrono::steady_clock::now();
        try {
            c.fn();
            const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
            std::cout << "[ PASS ] " << c.name << "  (" << ms << " ms)\n";
            ++passed;
        } catch (const mini_test::Failure& e) {
            std::cout << "[ FAIL ] " << c.name << "\n         " << e.what() << "\n";
            ++failed;
        } catch (const std::exception& e) {
            std::cout << "[ FAIL ] " << c.name << "\n         exception: " << e.what() << "\n";
            ++failed;
        }
    }
    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
