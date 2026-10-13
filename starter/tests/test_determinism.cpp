// Reference determinism checks. Do NOT weaken these tests; add your own next to them.
#include <cstdint>
#include <functional>
#include <thread>
#include <vector>

#include "mini_test.hpp"
#include "spec/simulation.hpp"
#include "spec/trace.hpp"

using namespace spec;

namespace {

const Trace& trace() {
    static const Trace t = load_trace(mini_test::trace_path());
    return t;
}

SimConfig config() {
    SimConfig c;
    c.num_players = trace().num_players;
    c.seed = 42;
    return c;
}

// Runs the simulation over the real inputs of the trace and returns hash after every tick.
std::vector<std::uint64_t> run_hashes(std::uint32_t ticks) {
    Simulation sim(config());
    std::vector<std::uint64_t> h;
    h.reserve(ticks + 1);
    h.push_back(sim.state_hash());
    for (std::uint32_t t = 0; t < ticks; ++t) {
        sim.step(trace().frame(t));
        h.push_back(sim.state_hash());
    }
    return h;
}

}  // namespace

TEST_CASE("determinism: two runs with identical inputs give identical hashes on every tick") {
    const auto a = run_hashes(trace().num_ticks);
    const auto b = run_hashes(trace().num_ticks);
    REQUIRE_EQ(a.size(), b.size());
    for (std::size_t i = 0; i < a.size(); ++i) REQUIRE_EQ(a[i], b[i]);
}

TEST_CASE("determinism: tick counter advances by exactly one per step") {
    Simulation sim(config());
    REQUIRE_EQ(sim.state().tick, 0u);
    for (std::uint32_t t = 0; t < 100; ++t) sim.step(trace().frame(t));
    REQUIRE_EQ(sim.state().tick, 100u);
}

TEST_CASE("determinism: hash actually depends on inputs (guards against a constant hash)") {
    Simulation a(config()), b(config());
    bool differs = false;
    for (std::uint32_t t = 0; t < 600 && !differs; ++t) {
        a.step(trace().frame(t));
        b.step(InputFrame{});  // nobody presses anything
        differs = a.state_hash() != b.state_hash();
    }
    REQUIRE(differs);
}

TEST_CASE("determinism: simulations in parallel threads match the sequential run (no hidden global state)") {
    const std::uint32_t n = 3000;
    const auto ref = run_hashes(n);
    std::vector<std::uint64_t> h1, h2;
    auto guarded = [](std::vector<std::uint64_t>& out, std::uint32_t count) {
        try {
            out = run_hashes(count);
        } catch (...) {
            out.clear();  // an exception must not escape a std::thread (std::terminate)
        }
    };
    std::thread t1(guarded, std::ref(h1), n);
    std::thread t2(guarded, std::ref(h2), n);
    t1.join();
    t2.join();
    REQUIRE(h1 == ref);
    REQUIRE(h2 == ref);
}

TEST_CASE("snapshot/restore: replay after restore is bit-identical") {
    Simulation sim(config());
    for (std::uint32_t t = 0; t < 500; ++t) sim.step(trace().frame(t));
    const Snapshot snap = sim.snapshot();
    const std::uint64_t at_snap = sim.state_hash();
    for (std::uint32_t t = 500; t < 700; ++t) sim.step(trace().frame(t));
    const std::uint64_t expected = sim.state_hash();

    for (int attempt = 0; attempt < 3; ++attempt) {  // the same snapshot can be restored many times
        sim.restore(snap);
        REQUIRE_EQ(sim.state().tick, 500u);
        REQUIRE_EQ(sim.state_hash(), at_snap);
        for (std::uint32_t t = 500; t < 700; ++t) sim.step(trace().frame(t));
        REQUIRE_EQ(sim.state_hash(), expected);
    }
}

TEST_CASE("snapshot/restore: branch with other inputs, restore, then real inputs give the oracle state") {
    const auto oracle = run_hashes(1000);
    Simulation sim(config());
    for (std::uint32_t t = 0; t < 400; ++t) sim.step(trace().frame(t));
    const Snapshot snap = sim.snapshot();
    for (std::uint32_t t = 400; t < 460; ++t) {  // wrong branch: everybody dashes to the right
        InputFrame f;
        for (auto& in : f.players) in.buttons = kRight | kDash;
        sim.step(f);
    }
    sim.restore(snap);
    for (std::uint32_t t = 400; t < 1000; ++t) sim.step(trace().frame(t));
    REQUIRE_EQ(sim.state_hash(), oracle[1000]);
}

TEST_CASE("snapshot/restore: snapshot is independent of later changes of the simulation") {
    Simulation sim(config());
    for (std::uint32_t t = 0; t < 50; ++t) sim.step(trace().frame(t));
    const Snapshot snap = sim.snapshot();
    const std::uint64_t h = sim.state_hash();
    for (std::uint32_t t = 50; t < 300; ++t) sim.step(trace().frame(t));
    Simulation other(config());
    other.restore(snap);  // a snapshot must be restorable into another instance with the same config
    REQUIRE_EQ(other.state_hash(), h);
}
