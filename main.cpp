#include <ufc/CfdSimulation.h>
#include <ufc/WarpSolver.h>

#include <iostream>
#include <memory>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: usd_cfd <input.usd> <output.usd>" << std::endl;
        return 1;
    }

    const std::string input_path  = argv[1];
    const std::string output_path = argv[2];

    const std::string script_path = std::string(argv[0]) + "/../python/warp_solver.py";

    ufc::CfdSimulation sim(
        ufd::DomainConfig{},
        ufd::EnvelopeConfig{},
        std::make_unique<ufc::WarpSolver>(script_path)
    );

    const std::string result = sim.run(input_path, output_path);
    if (result.empty()) {
        std::cerr << "Error: simulation failed." << std::endl;
        return 1;
    }

    std::cout << "Written: " << result << std::endl;
    return 0;
}
