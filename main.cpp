#include <ufc/SimConfig.h>
#include <ufc/WarpSolver.h>

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: usd_cfd <input.usd> <output.usd>" << std::endl;
        return 1;
    }

    const std::string input_path  = argv[1];
    const std::string output_path = argv[2];

    auto stage = pxr::UsdStage::Open(input_path);
    if (!stage) {
        std::cerr << "Error: cannot open stage " << input_path << std::endl;
        return 1;
    }

    ufc::SimConfig config;
    ufc::WarpSolver solver(config);

    const std::string result = solver.solve(stage, output_path);
    if (result.empty()) {
        std::cerr << "Error: solver failed." << std::endl;
        return 1;
    }

    std::cout << "Written: " << result << std::endl;
    return 0;
}
