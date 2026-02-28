#include <ufc/WarpSolver.h>

namespace ufc {

WarpSolver::WarpSolver(const SimConfig& config)
    : config_(config) {}

std::string WarpSolver::solve(UsdStageRefPtr /*stage*/,
                              const std::string& /*output_path*/) const
{
    // TODO: implement CFD evaluation using NVIDIA Warp
    return {};
}

} // namespace ufc
