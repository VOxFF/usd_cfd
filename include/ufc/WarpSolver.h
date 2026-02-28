#pragma once

#include <ufc/ISolver.h>
#include <ufc/SimConfig.h>

#include <string>

namespace ufc {

// CFD solver backed by NVIDIA Warp.  Runs as a subprocess:
//   python3 <script_path> <input.usd> <output.usd>
// The Python script is responsible for loading the USD stage, running the
// Warp simulation, and writing results back to output.usd.
class WarpSolver : public ISolver {
public:
    explicit WarpSolver(const std::string& script_path,
                        const SimConfig&   config = {});

    std::string solve(const std::string& input_path,
                      const std::string& output_path) const override;

private:
    std::string script_path_;
    SimConfig   config_;
};

} // namespace ufc
