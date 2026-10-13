// Reference checks for commit / rollback. The confirmed state of the speculative server must be
// bit-identical to an oracle that knows all real inputs, at every server tick. The presented state
// must be bit-identical to an oracle that uses the real inputs that have arrived and the prediction
// rule from the task for the rest.
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <vector>

#include "mini_test.hpp"
#include "spec/speculative_server.hpp"
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

const std::vector<std::uint64_t>& oracle() {
    static const std::vector<std::uint64_t> h = [] {
        Simulation sim(config());
        std::vector<std::uint64_t> v{sim.state_hash()};
        for (std::uint32_t t = 0; t < trace().num_ticks; ++t) {
            sim.step(trace().frame(t));
            v.push_back(sim.state_hash());
        }
        return v;
    }();
    return h;
}

struct ReplayResult {
    std::vector<std::uint32_t> confirmed_tick_at;  // confirmed_state().tick after each server tick
    std::vector<std::uint32_t> presented_tick_at;
    ServerStats stats;
};

// Replays the trace on the server timeline and checks the confirmed state against the oracle.
ReplayResult replay(Mode mode) {
    SpeculativeServer srv(config(), mode);
    const auto arrivals = trace().by_arrival();
    const std::uint32_t n = trace().num_ticks;
    ReplayResult rr;
    std::size_t next = 0;
    for (std::uint32_t T = 0;; ++T) {
        while (next < arrivals.size() && arrivals[next].arrival_tick <= T) {
            const auto& r = arrivals[next++];
            srv.on_input(r.player, r.tick, r.input);
        }
        srv.on_server_tick(T);
        const GameState& c = srv.confirmed_state();
        REQUIRE(c.tick <= n);
        REQUIRE_EQ(c.hash(), oracle()[c.tick]);
        rr.confirmed_tick_at.push_back(c.tick);
        rr.presented_tick_at.push_back(srv.presented_state().tick);
        if (next == arrivals.size() && c.tick == n) break;
        REQUIRE(T < n + 1000);  // must converge shortly after the last input arrived
    }
    rr.stats = srv.stats();
    return rr;
}

}  // namespace

TEST_CASE("lockstep: confirmed state equals the oracle at every server tick") {
    const auto rr = replay(Mode::kLockstep);
    REQUIRE_EQ(rr.stats.ticks_confirmed, static_cast<std::uint64_t>(trace().num_ticks));
    REQUIRE_EQ(rr.stats.mispredictions, 0u);
}

TEST_CASE("speculative: confirmed state equals the oracle at every server tick") {
    const auto rr = replay(Mode::kSpeculative);
    const auto& s = rr.stats;
    REQUIRE_EQ(s.ticks_confirmed, static_cast<std::uint64_t>(trace().num_ticks));
    REQUIRE(s.predictions_made > 0);
    REQUIRE(s.mispredictions <= s.predictions_made);
    REQUIRE(s.rollbacks <= s.mispredictions);
    REQUIRE(s.mispredictions == 0 || s.rollbacks > 0);
}

TEST_CASE("speculative: presented state is never behind the lockstep state") {
    const auto lock = replay(Mode::kLockstep);
    const auto spec = replay(Mode::kSpeculative);
    const std::size_t m = std::min(lock.confirmed_tick_at.size(), spec.presented_tick_at.size());
    std::uint64_t ahead = 0;
    for (std::size_t i = 0; i < m; ++i) {
        REQUIRE(spec.presented_tick_at[i] >= lock.confirmed_tick_at[i]);
        ahead += spec.presented_tick_at[i] - lock.confirmed_tick_at[i];
    }
    REQUIRE(ahead > 0);  // speculation must actually gain something on this trace
}

// Presented state after on_server_tick(T): ticks 0..T simulated with the real input of (t, p) if it
// has arrived by T, otherwise with the prediction "real input of p for the greatest tick < t that has
// arrived by T, or no buttons if there is none". The oracle keeps its own lockstep simulation of the
// fully known prefix and re-simulates the rest from it on every server tick.
TEST_CASE("speculative: presented state equals the oracle (arrived inputs + prediction) at every server tick") {
    SpeculativeServer srv(config(), Mode::kSpeculative);
    const auto arrivals = trace().by_arrival();
    const std::uint32_t n = trace().num_ticks;
    const int np = trace().num_players;
    std::vector<InputFrame> real(n);
    std::vector<int> received(n, 0);       // number of players whose input for tick t has arrived
    std::vector<std::uint8_t> known(static_cast<std::size_t>(n) * kMaxPlayers, 0);
    Simulation prefix(config());           // ticks [0, prefix.tick) have all real inputs
    std::size_t next = 0;
    for (std::uint32_t T = 0;; ++T) {
        while (next < arrivals.size() && arrivals[next].arrival_tick <= T) {
            const auto& r = arrivals[next++];
            srv.on_input(r.player, r.tick, r.input);
            real[r.tick].players[r.player] = r.input;
            known[static_cast<std::size_t>(r.tick) * kMaxPlayers + r.player] = 1;
            ++received[r.tick];
        }
        srv.on_server_tick(T);
        while (prefix.state().tick < n && received[prefix.state().tick] == np)
            prefix.step(real[prefix.state().tick]);

        Simulation expected = prefix;
        InputFrame last;  // newest real input of each player for a tick before the current one
        if (expected.state().tick > 0) last = real[expected.state().tick - 1];
        while (expected.state().tick <= T) {
            const std::uint32_t t = expected.state().tick;
            InputFrame f = last;
            for (int p = 0; p < np; ++p) {
                if (t < n && known[static_cast<std::size_t>(t) * kMaxPlayers + p]) {
                    f.players[p] = real[t].players[p];
                    last.players[p] = real[t].players[p];
                }
            }
            expected.step(f);
        }
        const GameState& ps = srv.presented_state();
        REQUIRE_EQ(ps.tick, T + 1);
        if (ps.hash() != expected.state_hash()) {
            std::ostringstream os;
            os << "presented state differs from the oracle after on_server_tick(" << T << ")";
            throw mini_test::Failure(os.str());
        }
        if (next == arrivals.size() && srv.confirmed_state().tick == n) break;
        REQUIRE(T < n + 1000);
    }
    // The trace has input changes the prediction cannot guess, so mispredictions and rollbacks happen.
    REQUIRE(srv.stats().mispredictions > 0);
    REQUIRE(srv.stats().rollbacks > 0);
}
