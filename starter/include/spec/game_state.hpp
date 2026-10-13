#pragma once
// World state of the toy game (READY-MADE, you do not need to change it).
//   * the state is fully determined by (SimConfig::seed, sequence of InputFrames);
//   * all numbers are fixed-point integers (1 unit = 1/256 of an arena unit): no floating point,
//     so the result is bit-identical on every compiler / CPU;
//   * the struct is a plain value type (no pointers): copying it copies the whole world.
#include <array>
#include <cstdint>

#include "spec/input.hpp"

namespace spec {

inline constexpr std::int32_t kFixedOne = 256;                 // fixed-point scale
inline constexpr std::int32_t kArenaSize = 1000 * kFixedOne;   // square arena [0, kArenaSize)
inline constexpr std::int32_t kPlayerRadius = 16 * kFixedOne;

struct PlayerState {
    std::int32_t x = 0, y = 0;    // position, fixed-point
    std::int32_t vx = 0, vy = 0;  // velocity of the last tick, fixed-point units per tick
    std::int16_t dash_ticks = 0;  // > 0 while dashing
    std::int16_t cooldown = 0;    // > 0 while Dash is unavailable
};

struct GameState {
    std::uint32_t tick = 0;  // number of the NEXT tick to be simulated
    int num_players = 0;
    std::array<PlayerState, kMaxPlayers> players{};

    // Deterministic 64-bit FNV-1a hash over every logical field (not over raw struct bytes:
    // padding bytes are unspecified).
    std::uint64_t hash() const;
};

}  // namespace spec
