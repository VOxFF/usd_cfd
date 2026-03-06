#include <ufc/CfdSimulation.h>
#include <ufc/ISolver.h>
#include <ufc/WarpSolver.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/points.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>

static const std::string BOX_USD  = std::string(TEST_RESOURCES_DIR) + "/box.usda";
static const std::string OUT_BASE = std::string(TEST_RESOURCES_DIR) + "/box";

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

TEST(CfdSimulationTest, RunCreatesIntermediateLayers) {
    ufc::CfdSimulation sim(
        ufd::DomainConfig{},
        ufd::EnvelopeConfig{},
        std::make_unique<NullSolver>()
    );

    sim.run(BOX_USD);

    EXPECT_TRUE(SdfLayer::FindOrOpen(OUT_BASE + ".domain.usda"));
    EXPECT_TRUE(SdfLayer::FindOrOpen(OUT_BASE + ".envelope.usda"));
}

TEST(CfdSimulationTest, RunReturnsSolverOutputPath) {
    ufc::CfdSimulation sim(
        ufd::DomainConfig{},
        ufd::EnvelopeConfig{},
        std::make_unique<NullSolver>()
    );

    const std::string result = sim.run(BOX_USD);

    EXPECT_EQ(result, OUT_BASE + ".composed.usda");
}

TEST(CfdSimulationTest, RunFailsOnMissingInput) {
    ufc::CfdSimulation sim(
        ufd::DomainConfig{},
        ufd::EnvelopeConfig{},
        std::make_unique<NullSolver>()
    );

    const std::string result = sim.run("nonexistent.usda");

    EXPECT_TRUE(result.empty());
}

TEST(CfdSimulationTest, RunUsesCustomOutputDir) {
    const std::string custom_dir = std::string(TEST_RESOURCES_DIR) + "/custom_out";
    std::filesystem::create_directory(custom_dir);

    ufc::CfdSimulation sim(
        ufd::DomainConfig{},
        ufd::EnvelopeConfig{},
        std::make_unique<NullSolver>()
    );

    const std::string result = sim.run(BOX_USD, custom_dir);

    EXPECT_EQ(result, custom_dir + "/box.composed.usda");
    EXPECT_TRUE(SdfLayer::FindOrOpen(custom_dir + "/box.domain.usda"));
    EXPECT_TRUE(SdfLayer::FindOrOpen(custom_dir + "/box.envelope.usda"));

    std::filesystem::remove_all(custom_dir);
}

TEST(CfdSimulationTest, RunWithWarpProducesAnimatedParticles) {
    ufc::CfdSimulation sim(
        ufd::DomainConfig{},
        ufd::EnvelopeConfig{},
        std::make_unique<ufc::WarpSolver>(WARP_SOLVER_SCRIPT)
    );

    const std::string result = sim.run(BOX_USD);
    ASSERT_FALSE(result.empty());

    auto stage = pxr::UsdStage::Open(OUT_BASE + ".particles.usda");
    ASSERT_TRUE(stage);

    auto prim = stage->GetPrimAtPath(pxr::SdfPath("/FluidParticles"));
    ASSERT_TRUE(prim.IsValid());

    auto pts_attr = pxr::UsdGeomPoints(prim).GetPointsAttr();
    EXPECT_GT(pts_attr.GetNumTimeSamples(), 1u);

    pxr::VtVec3fArray pts_start, pts_end;
    ASSERT_TRUE(pts_attr.Get(&pts_start, stage->GetStartTimeCode()));
    ASSERT_TRUE(pts_attr.Get(&pts_end,   stage->GetEndTimeCode() - 1));
    ASSERT_FALSE(pts_start.empty());
    EXPECT_NE(pts_start[0], pts_end[0]);
}
