// READY-MADE deterministic simulation of the toy game (you do not need to change this file).
//
// Rules: square arena 1000 x 1000, players are circles of radius 16. Each tick a player moves by
// 3 units in the direction of the pressed arrows (diagonals are not normalised), Dash = speed 9 for
// 8 ticks, then a 45-tick cooldown. Players are clamped to the arena; overlapping players are pushed
// apart by 2 units along the dominant axis. Everything is integer (fixed-point) arithmetic and the
// players are always processed in index order, so the result is deterministic.
#include "spec/simulation.hpp"

namespace spec {
namespace {

void mix(std::uint64_t& h, std::int64_t v) {
    for (int i = 0; i < 8; ++i) {
        h ^= (static_cast<std::uint64_t>(v) >> (8 * i)) & 0xffu;
        h *= 1099511628211ull;  // FNV-1a 64-bit prime
    }
}

std::uint64_t lcg(std::uint64_t& s) {
    s = s * 6364136223846793005ull + 1442695040888963407ull;
    return s >> 33;
}

std::int32_t clamp_coord(std::int32_t v) {
    if (v < kPlayerRadius) return kPlayerRadius;
    if (v > kArenaSize - kPlayerRadius) return kArenaSize - kPlayerRadius;
    return v;
}

std::int64_t abs64(std::int64_t v) { return v < 0 ? -v : v; }

}  // namespace

std::uint64_t GameState::hash() const {
    std::uint64_t h = 1469598103934665603ull;  // FNV-1a 64-bit offset basis
    mix(h, tick);
    mix(h, num_players);
    for (int i = 0; i < num_players; ++i) {
        const PlayerState& p = players[i];
        mix(h, p.x);
        mix(h, p.y);
        mix(h, p.vx);
        mix(h, p.vy);
        mix(h, p.dash_ticks);
        mix(h, p.cooldown);
    }
    return h;
}

Simulation::Simulation(const SimConfig& cfg) : cfg_(cfg) {
    state_.num_players = cfg.num_players;
    std::uint64_t s = cfg.seed;
    const std::uint64_t span = static_cast<std::uint64_t>(kArenaSize - 2 * kPlayerRadius);
    for (int i = 0; i < cfg.num_players; ++i) {
        state_.players[i].x = kPlayerRadius + static_cast<std::int32_t>(lcg(s) % span);
        state_.players[i].y = kPlayerRadius + static_cast<std::int32_t>(lcg(s) % span);
    }
}

void Simulation::step(const InputFrame& inputs) {
    // 1. movement and dash
    for (int i = 0; i < state_.num_players; ++i) {
        PlayerState& p = state_.players[i];
        const std::uint8_t b = inputs.players[i].buttons;
        const int dx = ((b & kRight) ? 1 : 0) - ((b & kLeft) ? 1 : 0);
        const int dy = ((b & kDown) ? 1 : 0) - ((b & kUp) ? 1 : 0);
        if ((b & kDash) && p.cooldown == 0 && (dx != 0 || dy != 0)) {
            p.dash_ticks = 8;
            p.cooldown = 45;
        }
        const std::int32_t speed = (p.dash_ticks > 0 ? 9 : 3) * kFixedOne;
        p.vx = dx * speed;
        p.vy = dy * speed;
        if (p.dash_ticks > 0) --p.dash_ticks;
        if (p.cooldown > 0) --p.cooldown;
        p.x = clamp_coord(p.x + p.vx);
        p.y = clamp_coord(p.y + p.vy);
    }
    // 2. player-player collisions, pairs in a fixed order (i < j)
    const std::int64_t min_dist = 2 * static_cast<std::int64_t>(kPlayerRadius);
    for (int i = 0; i < state_.num_players; ++i) {
        for (int j = i + 1; j < state_.num_players; ++j) {
            PlayerState& a = state_.players[i];
            PlayerState& c = state_.players[j];
            const std::int64_t ddx = static_cast<std::int64_t>(c.x) - a.x;
            const std::int64_t ddy = static_cast<std::int64_t>(c.y) - a.y;
            if (ddx * ddx + ddy * ddy >= min_dist * min_dist) continue;
            const std::int32_t push = 2 * kFixedOne;
            if (abs64(ddx) >= abs64(ddy)) {
                const int s = ddx >= 0 ? 1 : -1;
                a.x = clamp_coord(a.x - s * push);
                c.x = clamp_coord(c.x + s * push);
            } else {
                const int s = ddy >= 0 ? 1 : -1;
                a.y = clamp_coord(a.y - s * push);
                c.y = clamp_coord(c.y + s * push);
            }
        }
    }
    ++state_.tick;
}

}  // namespace spec
