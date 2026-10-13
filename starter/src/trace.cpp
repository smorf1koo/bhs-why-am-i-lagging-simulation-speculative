// Ready-made CSV loader for data/input_trace.csv. Not part of the task.
#include "spec/trace.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace spec {

namespace {
constexpr const char* kHeader = "tick,player,buttons,send_us,delay_us,arrival_us,arrival_tick";

std::int64_t to_i64(const std::string& s, std::size_t line) {
    try {
        std::size_t pos = 0;
        long long v = std::stoll(s, &pos);
        if (pos != s.size()) throw std::invalid_argument("trailing characters");
        return v;
    } catch (const std::exception&) {
        throw std::runtime_error("input_trace: bad integer '" + s + "' at line " + std::to_string(line));
    }
}
}  // namespace

Trace load_trace(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("input_trace: cannot open " + path);
    std::string line;
    if (!std::getline(in, line)) throw std::runtime_error("input_trace: empty file");
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != kHeader) throw std::runtime_error("input_trace: unexpected header: " + line);

    Trace tr;
    std::size_t lineno = 1;
    while (std::getline(in, line)) {
        ++lineno;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string f[7];
        for (int i = 0; i < 7; ++i) {
            if (!std::getline(ss, f[i], ','))
                throw std::runtime_error("input_trace: too few fields at line " + std::to_string(lineno));
        }
        TraceRecord r;
        r.tick = static_cast<std::uint32_t>(to_i64(f[0], lineno));
        r.player = static_cast<int>(to_i64(f[1], lineno));
        r.input.buttons = static_cast<std::uint8_t>(to_i64(f[2], lineno));
        r.send_us = to_i64(f[3], lineno);
        r.delay_us = to_i64(f[4], lineno);
        r.arrival_us = to_i64(f[5], lineno);
        r.arrival_tick = static_cast<std::uint32_t>(to_i64(f[6], lineno));
        if (r.player < 0 || r.player >= kMaxPlayers)
            throw std::runtime_error("input_trace: bad player id at line " + std::to_string(lineno));
        tr.num_players = std::max(tr.num_players, r.player + 1);
        tr.num_ticks = std::max(tr.num_ticks, r.tick + 1);
        tr.records.push_back(r);
    }
    if (tr.records.size() != static_cast<std::size_t>(tr.num_players) * tr.num_ticks)
        throw std::runtime_error("input_trace: expected exactly one record per (tick, player)");
    std::sort(tr.records.begin(), tr.records.end(), [](const TraceRecord& a, const TraceRecord& b) {
        return std::tie(a.tick, a.player) < std::tie(b.tick, b.player);
    });
    return tr;
}

InputFrame Trace::frame(std::uint32_t t) const {
    InputFrame fr;
    const std::size_t base = static_cast<std::size_t>(t) * static_cast<std::size_t>(num_players);
    for (int p = 0; p < num_players; ++p) fr.players[static_cast<std::size_t>(p)] = records.at(base + static_cast<std::size_t>(p)).input;
    return fr;
}

std::vector<TraceRecord> Trace::by_arrival() const {
    std::vector<TraceRecord> v = records;
    std::stable_sort(v.begin(), v.end(), [](const TraceRecord& a, const TraceRecord& b) {
        return std::tie(a.arrival_us, a.tick, a.player) < std::tie(b.arrival_us, b.tick, b.player);
    });
    return v;
}

}  // namespace spec
