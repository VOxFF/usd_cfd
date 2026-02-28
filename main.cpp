#include <ufc/WarpSolver.h>
#include <ufc/SimConfig.h>

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: usd_cfd <input.usd> <output.usd>" << std::endl;
        return 1;
    }

    const std::string input_path  = argv[1];
    const std::string output_path = argv[2];

    // Locate the Warp solver script relative to the executable
    const std::string script_path = std::string(argv[0]) + "/../python/warp_solver.py";

    ufc::WarpSolver solver(script_path, ufc::SimConfig{});

    const std::string result = solver.solve(input_path, output_path);
    if (result.empty()) {
        std::cerr << "Error: solver failed." << std::endl;
        return 1;
    }

    std::cout << "Written: " << result << std::endl;
    return 0;
}
