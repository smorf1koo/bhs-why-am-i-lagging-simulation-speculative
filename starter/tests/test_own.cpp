// YOUR tests go here (at least one, see the task). The runner is tests/mini_test.hpp:
//   TEST_CASE("my: ...") { REQUIRE(cond); REQUIRE_EQ(a, b); }
// Ideas: an input for tick t+1 arrives before the input for t; two players mispredicted in the same
// server tick; a misprediction deep in the past (rollback over many ticks); a tiny hand-made trace.
#include "mini_test.hpp"
#include "spec/speculative_server.hpp"
