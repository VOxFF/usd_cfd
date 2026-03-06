#include <ufc/WarpSolver.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/gf/vec3f.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

static const std::string SCRIPT   = WARP_SOLVER_SCRIPT;
static const std::string BASE     = std::string(TEST_RESOURCES_DIR) + "/warp_solver_test_";
static const std::string IN_USD   = BASE + "input.usda";
static const std::string OUT_USD  = BASE + "particles.usda";

// Create a minimal USD with a /FluidDomain box mesh (100³ units).
static void make_domain_stage(const std::string& path) {
    std::filesystem::remove(path);
    auto stage = pxr::UsdStage::CreateNew(path);
    auto mesh  = pxr::UsdGeomMesh::Define(stage, pxr::SdfPath("/FluidDomain"));

    mesh.GetPointsAttr().Set(pxr::VtVec3fArray{
        {0,0,0},{100,0,0},{100,100,0},{0,100,0},
        {0,0,100},{100,0,100},{100,100,100},{0,100,100},
    });
    mesh.GetFaceVertexCountsAttr().Set(pxr::VtIntArray{4,4,4,4,4,4});
    mesh.GetFaceVertexIndicesAttr().Set(pxr::VtIntArray{
        0,1,2,3, 4,7,6,5, 0,4,7,3, 1,5,6,2, 0,1,5,4, 3,2,6,7
    });
    stage->GetRootLayer()->Save();
}

class WarpSolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        make_domain_stage(IN_USD);
        std::filesystem::remove(OUT_USD);
    }
};

TEST_F(WarpSolverTest, SolveReturnsOutputPath) {
    ufc::WarpSolver solver(SCRIPT);
    EXPECT_EQ(solver.solve(IN_USD, OUT_USD), OUT_USD);
}

TEST_F(WarpSolverTest, SolveProducesAnimatedFluidParticles) {
    ufc::WarpSolver solver(SCRIPT);
    solver.solve(IN_USD, OUT_USD);

    auto stage = pxr::UsdStage::Open(OUT_USD);
    ASSERT_TRUE(stage);

    auto prim = stage->GetPrimAtPath(pxr::SdfPath("/FluidParticles"));
    ASSERT_TRUE(prim.IsValid());
    EXPECT_EQ(prim.GetTypeName(), pxr::TfToken("Points"));

    auto pts_attr = pxr::UsdGeomPoints(prim).GetPointsAttr();
    EXPECT_GT(pts_attr.GetNumTimeSamples(), 1u);

    // Positions must change over time (fluid flows)
    pxr::VtVec3fArray pts_start, pts_end;
    ASSERT_TRUE(pts_attr.Get(&pts_start, stage->GetStartTimeCode()));
    ASSERT_TRUE(pts_attr.Get(&pts_end,   stage->GetEndTimeCode() - 1));
    ASSERT_FALSE(pts_start.empty());
    EXPECT_EQ(pts_start.size(), pts_end.size());
    EXPECT_NE(pts_start[0], pts_end[0]);
}

TEST_F(WarpSolverTest, SolveFailsOnMissingInput) {
    ufc::WarpSolver solver(SCRIPT);
    EXPECT_TRUE(solver.solve("nonexistent.usda", OUT_USD).empty());
}

TEST_F(WarpSolverTest, SolveFailsOnMissingScript) {
    ufc::WarpSolver solver("/no/such/script.py");
    EXPECT_TRUE(solver.solve(IN_USD, OUT_USD).empty());
}
