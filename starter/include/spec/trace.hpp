#pragma once
// Loader for data/input_trace.csv (format: data/FORMAT.md). Provided ready-made: it is not part of the task.
#include <cstdint>
#include <string>
#include <vector>

#include "spec/input.hpp"

namespace spec {

struct TraceRecord {
    std::uint32_t tick = 0;        // simulation tick this input belongs to
    int player = 0;                // 0..3
    Input input{};
    std::int64_t send_us = 0;      // client send time (server timeline, microseconds)
    std::int64_t delay_us = 0;     // one-way network delay of this packet
    std::int64_t arrival_us = 0;   // send_us + delay_us
    std::uint32_t arrival_tick = 0;  // first server tick T with T * kTickUs >= arrival_us
};

struct Trace {
    int num_players = 0;
    std::uint32_t num_ticks = 0;
    std::vector<TraceRecord> records;  // in file order: by tick, then by player

    // Ground-truth inputs of all players for tick t (what an oracle server would use).
    InputFrame frame(std::uint32_t t) const;
    // Records sorted by (arrival_us, tick, player): the order in which the server receives them.
    std::vector<TraceRecord> by_arrival() const;
};

// Throws std::runtime_error on I/O or format errors.
Trace load_trace(const std::string& path);

}  // namespace spec
