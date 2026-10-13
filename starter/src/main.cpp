// spec_run: experiment driver. Replays data/input_trace.csv on the server timeline.
//
//   ./build/spec_run --trace ../data/input_trace.csv
//
// The replay loop is ready. YOUR TASK (part 3): the metrics (see TODO below).
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "spec/speculative_server.hpp"
#include "spec/trace.hpp"

namespace {

struct Replay {
    std::vector<std::uint32_t> confirmed_tick;  // confirmed_state().tick after on_server_tick(T), index = T
    std::vector<std::uint32_t> presented_tick;  // presented_state().tick after on_server_tick(T)
    std::vector<std::uint64_t> presented_hash;  // presented_state().hash() after on_server_tick(T)
    spec::ServerStats stats;
    std::uint64_t final_hash = 0;
};

// At server tick T: deliver every input with arrival_tick <= T (in arrival order), then on_server_tick(T).
// Continues after the last tick of the trace until everything is confirmed.
Replay replay(const spec::Trace& trace, spec::Mode mode) {
    spec::SimConfig cfg;
    cfg.num_players = trace.num_players;
    cfg.seed = 42;
    spec::SpeculativeServer server(cfg, mode);
    const auto arrivals = trace.by_arrival();
    Replay r;
    std::size_t next = 0;
    for (std::uint32_t T = 0;; ++T) {
        while (next < arrivals.size() && arrivals[next].arrival_tick <= T) {
            const auto& rec = arrivals[next++];
            server.on_input(rec.player, rec.tick, rec.input);
        }
        server.on_server_tick(T);
        r.confirmed_tick.push_back(server.confirmed_state().tick);
        r.presented_tick.push_back(server.presented_state().tick);
        r.presented_hash.push_back(server.presented_state().hash());
        if (next == arrivals.size() && server.confirmed_state().tick == trace.num_ticks) break;
        if (T > trace.num_ticks + 1000) throw std::runtime_error("server does not converge");
    }
    r.stats = server.stats();
    r.final_hash = server.confirmed_state().hash();
    return r;
}

}  // namespace

int main(int argc, char** argv) {
    std::string trace_path = "../data/input_trace.csv";
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--trace") && i + 1 < argc) {
            trace_path = argv[++i];
        } else {
            std::cerr << "usage: spec_run [--trace PATH]\n";
            return 2;
        }
    }
    try {
        const spec::Trace trace = spec::load_trace(trace_path);
        std::cout << "trace: " << trace.num_players << " players, " << trace.num_ticks << " ticks\n";
        const Replay lock = replay(trace, spec::Mode::kLockstep);
        const Replay spec_ = replay(trace, spec::Mode::kSpeculative);
        std::cout << "final confirmed hash: lockstep " << lock.final_hash << ", speculative " << spec_.final_hash
                  << "\n";
        // TODO (metrics, print them for report.md):
        //   * misprediction rate = mispredictions / predictions_made, overall and per player;
        //   * share of ticks where the presented state matched the final one, in %: for every server tick
        //       T < num_ticks, spec_.presented_hash[T] is compared with the hash of the confirmed state of
        //       tick T + 1 (all real inputs of ticks 0..T applied; Simulation stepped with trace.frame(t)
        //       gives exactly these states); share = matches / num_ticks. Server ticks T >= num_ticks are
        //       not counted: their states are never confirmed;
        //   * number of rollbacks and re-simulated ticks (and their share of num_ticks).
        std::cout << "TODO: metrics are not implemented\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
