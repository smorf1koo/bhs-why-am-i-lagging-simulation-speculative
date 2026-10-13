#pragma once
// Minimal self-contained test runner (no gtest/Catch2 needed). Usage:
//   TEST_CASE("name") { REQUIRE(a == b); REQUIRE_EQ(x, y); }
// Run: ./build/spec_tests [--trace PATH] [substring-filter]
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mini_test {

struct Failure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Case {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> r;
    return r;
}

inline int& soft_failures() {
    static int n = 0;
    return n;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> fn) { registry().push_back({name, std::move(fn)}); }
};

// Path to data/input_trace.csv (overridable with --trace).
std::string& trace_path();

}  // namespace mini_test

#define MT_CAT2(a, b) a##b
#define MT_CAT(a, b) MT_CAT2(a, b)
#define TEST_CASE(name)                                                                 \
    static void MT_CAT(mt_case_, __LINE__)();                                           \
    static ::mini_test::Registrar MT_CAT(mt_reg_, __LINE__)(name, &MT_CAT(mt_case_, __LINE__)); \
    static void MT_CAT(mt_case_, __LINE__)()

#define MT_FAIL_MSG(expr_text, extra)                                                    \
    ([&] {                                                                               \
        std::ostringstream mt_os;                                                        \
        mt_os << __FILE__ << ":" << __LINE__ << ": " << expr_text << extra;              \
        return mt_os.str();                                                              \
    }())

#define REQUIRE(cond)                                                                    \
    do {                                                                                 \
        if (!(cond)) throw ::mini_test::Failure(MT_FAIL_MSG("REQUIRE(" #cond ") failed", "")); \
    } while (0)

#define REQUIRE_EQ(a, b)                                                                 \
    do {                                                                                 \
        const auto& mt_a = (a);                                                          \
        const auto& mt_b = (b);                                                          \
        if (!(mt_a == mt_b))                                                             \
            throw ::mini_test::Failure(MT_FAIL_MSG("REQUIRE_EQ(" #a ", " #b ") failed: ", \
                                                   mt_a << " != " << mt_b));             \
    } while (0)
