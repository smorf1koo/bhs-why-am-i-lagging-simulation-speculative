#include "spec/speculative_server.hpp"

#include <stdexcept>

namespace spec {

SpeculativeServer::SpeculativeServer(const SimConfig& cfg, Mode mode) {
    (void)cfg;
    (void)mode;
    // TODO
}

void SpeculativeServer::on_input(int player, std::uint32_t tick, Input input) {
    (void)player;
    (void)tick;
    (void)input;
    // TODO: store the real input; if the tick was already simulated with a prediction for this
    // player, compare and either keep (commit) or schedule a rollback to the confirmed state.
    throw std::logic_error("TODO: SpeculativeServer::on_input is not implemented");
}

void SpeculativeServer::on_server_tick(std::uint32_t server_tick) {
    (void)server_tick;
    // TODO: advance confirmed state as far as real inputs allow; in speculative mode also advance
    // the presented state up to server_tick using predicted inputs: for tick t, the player's real
    // input for the greatest tick < t that has arrived, or no buttons if there is none.
    throw std::logic_error("TODO: SpeculativeServer::on_server_tick is not implemented");
}

const GameState& SpeculativeServer::confirmed_state() const {
    throw std::logic_error("TODO: SpeculativeServer::confirmed_state is not implemented");
}

const GameState& SpeculativeServer::presented_state() const {
    throw std::logic_error("TODO: SpeculativeServer::presented_state is not implemented");
}

const ServerStats& SpeculativeServer::stats() const {
    throw std::logic_error("TODO: SpeculativeServer::stats is not implemented");
}

}  // namespace spec
