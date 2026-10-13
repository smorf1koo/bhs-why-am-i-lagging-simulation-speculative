#pragma once
// Player input for one simulation tick. The format is FIXED (it matches data/input_trace.csv).
#include <array>
#include <cstdint>

namespace spec {

inline constexpr int kMaxPlayers = 4;
inline constexpr int kTickHz = 60;
inline constexpr std::int64_t kTickUs = 16667;  // server tick duration, microseconds

enum Button : std::uint8_t {
    kUp = 1u << 0,
    kDown = 1u << 1,
    kLeft = 1u << 2,
    kRight = 1u << 3,
    kDash = 1u << 4,
};

struct Input {
    std::uint8_t buttons = 0;  // bitmask of Button
    bool operator==(const Input&) const = default;
};

// Inputs of all players for one tick. Index = player id (0 .. num_players-1).
struct InputFrame {
    std::array<Input, kMaxPlayers> players{};
    bool operator==(const InputFrame&) const = default;
};

}  // namespace spec
