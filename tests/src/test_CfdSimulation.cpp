#include <ufc/CfdSimulation.h>
#include <ufc/ISolver.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <gtest/gtest.h>

#include <memory>
#include <string>

static const std::string BOX_USD =
    std::string(TEST_RESOURCES_DIR) + "/box.usda";

// Minimal solver stub: creates an empty output USD layer and returns its path.
class NullSolver : public ufc::ISolver {
public:
    std::string solve(const std::string& /*input_path*/,
                      const std::string& output_path) const override {
        auto stage = pxr::UsdStage::CreateNew(output_path);
        if (!stage) return {};
        stage->GetRootLayer()->Save();
        return output_path;
    }
};

static const std::string OUT =
    std::string(TEST_RESOURCES_DIR) + "/box_test_cfd";

TEST(CfdSimulationTest, RunCreatesIntermediateLayers) {
    ufc::CfdSimulation sim(
        ufd::DomainConfig{},
        ufd::EnvelopeConfig{},
        std::make_unique<NullSolver>()
    );

    sim.run(BOX_USD, OUT);

    EXPECT_TRUE(SdfLayer::FindOrOpen(OUT + ".domain.usda"));
    EXPECT_TRUE(SdfLayer::FindOrOpen(OUT + ".envelope.usda"));
    EXPECT_TRUE(SdfLayer::FindOrOpen(OUT + ".composed.usda"));
}

TEST(CfdSimulationTest, RunReturnsSolverOutputPath) {
    ufc::CfdSimulation sim(
        ufd::DomainConfig{},
        ufd::EnvelopeConfig{},
        std::make_unique<NullSolver>()
    );

    const std::string result = sim.run(BOX_USD, OUT);

    EXPECT_EQ(result, OUT);
}

TEST(CfdSimulationTest, RunFailsOnMissingInput) {
    ufc::CfdSimulation sim(
        ufd::DomainConfig{},
        ufd::EnvelopeConfig{},
        std::make_unique<NullSolver>()
    );

    const std::string result = sim.run("nonexistent.usda", OUT);

    EXPECT_TRUE(result.empty());
}
