#pragma once

#include <ufc/SimConfig.h>

#include <pxr/usd/usd/stage.h>

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace ufc {

// Runs a CFD simulation on a composed USD stage produced by usd_fluid_domain.
// The stage is expected to contain /FluidDomain and /Envelope prims.
// Simulation is evaluated using NVIDIA Warp on the GPU.
class WarpSolver {
public:
    explicit WarpSolver(const SimConfig& config = {});

    // Run the simulation and write results to a new USD layer.
    // Returns the output layer path on success, or an empty string on failure.
    std::string solve(UsdStageRefPtr stage, const std::string& output_path) const;

private:
    SimConfig config_;
};

} // namespace ufc
