#include <ufc/CfdSimulation.h>

#include <ufd/StageReader.h>
#include <ufd/SurfaceExtractor.h>
#include <ufd/StageComposer.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/sdf/layer.h>

#include <filesystem>
#include <iostream>

namespace ufc {

CfdSimulation::CfdSimulation(const ufd::DomainConfig&   domain_config,
                             const ufd::EnvelopeConfig& envelope_config,
                             std::unique_ptr<ISolver>   solver)
    : domain_builder_(domain_config)
    , envelope_builder_(envelope_config)
    , solver_(std::move(solver)) {}

std::string CfdSimulation::run(const std::string& input_path,
                               const std::string& output_dir) const
{
    // 1. Load input geometry
    ufd::StageReader reader;
    if (!reader.open(input_path)) {
        std::cerr << "CfdSimulation: cannot open stage " << input_path << std::endl;
        return {};
    }

    auto meshes = reader.collect_meshes();
    if (meshes.empty()) {
        std::cerr << "CfdSimulation: no meshes found in stage." << std::endl;
    }

    // 2. Extract bounds
    ufd::SurfaceExtractor extractor;
    auto bounds = extractor.compute_bounding_box(extractor.extract(meshes));

    // Derive all output paths from the input filename stem placed in output_dir;
    // if output_dir is empty, use the input file's directory.
    namespace fs = std::filesystem;
    const fs::path out_dir = output_dir.empty() ? fs::path(input_path).parent_path()
                                                 : fs::path(output_dir);
    const std::string base = (out_dir / fs::path(input_path).stem()).string();

    const std::string domain_path    = base + ".domain.usda";
    const std::string envelope_path  = base + ".envelope.usda";
    const std::string composed_path  = base + ".composed.usda";
    const std::string particles_path = base + ".particles.usda";

    // 3. Build domain
    auto domain_stage = pxr::UsdStage::CreateNew(domain_path);
    if (!domain_stage) {
        std::cerr << "CfdSimulation: cannot create domain stage." << std::endl;
        return {};
    }
    domain_builder_.build(domain_stage, bounds);

    // 4. Build envelope
    auto envelope_stage = pxr::UsdStage::CreateNew(envelope_path);
    if (!envelope_stage) {
        std::cerr << "CfdSimulation: cannot create envelope stage." << std::endl;
        return {};
    }
    const std::string sdf_path = base + ".envelope.bin";
    envelope_builder_.build(envelope_stage, meshes, sdf_path);

    // 5. Compose input + domain + envelope — passed to solver as input
    {
        ufd::StageComposer composer(composed_path);
        composer.add_component(ufd::ComponentType::InputGeometry, reader.get_stage());
        composer.add_component(ufd::ComponentType::FluidDomain,   domain_stage);
        composer.add_component(ufd::ComponentType::Envelope,      envelope_stage);
        if (!composer.write()) {
            std::cerr << "CfdSimulation: cannot write composed stage." << std::endl;
            return {};
        }
    }

    // 6. Run solver — writes particle animation to particles_path
    if (solver_->solve(composed_path, particles_path).empty()) {
        std::cerr << "CfdSimulation: solver failed." << std::endl;
        return {};
    }

    // 7. Add particles as the strongest sublayer in composed.usda and
    //    copy time metadata so usdview knows the animation range.
    {
        auto composed_layer  = SdfLayer::FindOrOpen(composed_path);
        auto particles_layer = SdfLayer::FindOrOpen(particles_path);
        if (composed_layer && particles_layer) {
            composed_layer->GetSubLayerPaths().insert(
                composed_layer->GetSubLayerPaths().begin(), particles_path);
            composed_layer->SetStartTimeCode(particles_layer->GetStartTimeCode());
            composed_layer->SetEndTimeCode(particles_layer->GetEndTimeCode());
            composed_layer->SetFramesPerSecond(particles_layer->GetFramesPerSecond());
            composed_layer->Save();
        }
    }

    return composed_path;
}

} // namespace ufc
