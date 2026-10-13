#pragma once
// YOUR TASK (part 2): server-side speculation logic.
// The PUBLIC methods below are used by tests/test_speculative.cpp: keep their signatures. You may add
// anything you need (members, helper methods, constructor parameters WITH default values, ...).
#include <array>
#include <cstdint>

#include "spec/simulation.hpp"

namespace spec {

enum class Mode {
    kLockstep,     // step tick t only when the real inputs of ALL players for t have arrived (reference)
    kSpeculative,  // step with predicted inputs for missing players, verify later, roll back if needed
};

struct ServerStats {
    std::uint64_t ticks_confirmed = 0;    // ticks whose state is final (all real inputs applied)
    std::uint64_t predictions_made = 0;   // (tick, player) inputs that were predicted (re-predictions count)
    std::uint64_t mispredictions = 0;     // predicted inputs that turned out different from the real ones
    std::uint64_t rollbacks = 0;          // number of restore() + re-simulate episodes
    std::uint64_t resimulated_ticks = 0;  // total ticks re-simulated because of rollbacks
    std::array<std::uint64_t, kMaxPlayers> predictions_by_player{};
    std::array<std::uint64_t, kMaxPlayers> mispredictions_by_player{};
    // You may add more (max rollback depth, ...).
};

class SpeculativeServer {
public:
    SpeculativeServer(const SimConfig& cfg, Mode mode);

    // The real input of `player` for simulation tick `tick` has arrived (any order, each exactly once).
    void on_input(int player, std::uint32_t tick, Input input);

    // Server tick boundary. Lockstep: advance only as far as real inputs allow.
    // Speculative: also advance the presented state up to and including tick `server_tick`
    // (i.e. presented_state().tick == server_tick + 1), predicting the missing inputs.
    void on_server_tick(std::uint32_t server_tick);

    // Latest state in which every applied input is a real one.
    const GameState& confirmed_state() const;
    // State the server would show to players right now (may contain predicted inputs).
    // In lockstep mode it is the confirmed state.
    const GameState& presented_state() const;

    const ServerStats& stats() const;

private:
    // TODO: your data (simulation instances, received inputs, inputs used for each simulated tick,
    // snapshots, ...)
};

}  // namespace spec
