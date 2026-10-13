// YOUR TASK (part 1): snapshot / restore of the simulation.
#include <stdexcept>

#include "spec/simulation.hpp"

namespace spec {

Snapshot Simulation::snapshot() const {
    // TODO: return everything needed to continue exactly from the current state.
    throw std::logic_error("TODO: Simulation::snapshot is not implemented");
}

void Simulation::restore(const Snapshot& snap) {
    (void)snap;
    // TODO: make the simulation continue exactly from `snap` (the snapshot itself must stay usable).
    throw std::logic_error("TODO: Simulation::restore is not implemented");
}

}  // namespace spec
