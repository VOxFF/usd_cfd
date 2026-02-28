#pragma once

#include <string>

namespace ufc {

// Common interface for all CFD solvers.
// Input and output are USD file paths — solvers communicate via USD layers on
// disk. The input stage is expected to contain /FluidDomain and /Envelope prims
// (as produced by usd_fluid_domain). The solver writes simulation results to
// output_path and returns it on success, or an empty string on failure.
class ISolver {
public:
    virtual ~ISolver() = default;

    virtual std::string solve(const std::string& input_path,
                              const std::string& output_path) const = 0;
};

} // namespace ufc
