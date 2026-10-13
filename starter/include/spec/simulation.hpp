#pragma once
// Deterministic fixed-step simulation: step(state, inputs) -> next state.
// step() and the constructor are READY-MADE (src/simulation.cpp). YOUR TASK here: snapshot() / restore()
// in src/snapshot.cpp.
#include <cstdint>

#include "spec/game_state.hpp"
#include "spec/input.hpp"

namespace spec {

struct SimConfig {
    int num_players = 4;
    std::uint64_t seed = 1;  // initial placement; there is no other source of randomness or time
};

// Snapshot of the simulation: everything needed to continue EXACTLY from this point later.
// You may change the representation (a full copy is the simplest and is fine for this task).
struct Snapshot {
    GameState state;
};

class Simulation {
public:
    explicit Simulation(const SimConfig& cfg);

    // Advances the world by exactly one tick using the inputs of ALL players for state().tick.
    void step(const InputFrame& inputs);

    const GameState& state() const { return state_; }
    std::uint64_t state_hash() const { return state_.hash(); }

    // TODO (src/snapshot.cpp): take a snapshot / go back to it. Any snapshot must be restorable
    // many times and into another Simulation instance with the same SimConfig.
    Snapshot snapshot() const;
    void restore(const Snapshot& snap);

private:
    SimConfig cfg_;
    GameState state_;
};

}  // namespace spec
