#include <ufc/WarpSolver.h>

#include <cstdlib>
#include <iostream>

namespace ufc {

WarpSolver::WarpSolver(const std::string& script_path, const SimConfig& config)
    : script_path_(script_path), config_(config) {}

std::string WarpSolver::solve(const std::string& input_path,
                              const std::string& output_path) const
{
    const std::string cmd = "python3 " + script_path_
                          + " " + input_path
                          + " " + output_path;

    const int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "WarpSolver: script exited with code " << ret << std::endl;
        return {};
    }

    return output_path;
}

} // namespace ufc
