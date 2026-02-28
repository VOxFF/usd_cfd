#include <ufc/CfdSimulation.h>

#include <ufd/StageReader.h>
#include <ufd/SurfaceExtractor.h>

#include <iostream>

namespace ufc {

CfdSimulation::CfdSimulation(const ufd::DomainConfig&   domain_config,
                             const ufd::EnvelopeConfig& envelope_config,
                             std::unique_ptr<ISolver>   solver)
    : domain_builder_(domain_config)
    , envelope_builder_(envelope_config)
    , solver_(std::move(solver)) {}

std::string CfdSimulation::run(const std::string& input_path,
                               const std::string& output_path) const
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

    // 3. Build domain
    const std::string domain_path   = output_path + ".domain.usda";
    auto domain_stage = pxr::UsdStage::CreateNew(domain_path);
    if (!domain_stage) {
        std::cerr << "CfdSimulation: cannot create domain stage." << std::endl;
        return {};
    }
    domain_builder_.build(domain_stage, bounds);

    // 4. Build envelope
    const std::string envelope_path = output_path + ".envelope.usda";
    auto envelope_stage = pxr::UsdStage::CreateNew(envelope_path);
    if (!envelope_stage) {
        std::cerr << "CfdSimulation: cannot create envelope stage." << std::endl;
        return {};
    }
    envelope_builder_.build(envelope_stage, meshes);

    // 5. Compose all layers
    const std::string composed_path = output_path + ".composed.usda";
    ufd::StageComposer composer(composed_path);
    composer.add_component(ufd::ComponentType::InputGeometry, reader.get_stage());
    composer.add_component(ufd::ComponentType::FluidDomain,   domain_stage);
    composer.add_component(ufd::ComponentType::Envelope,      envelope_stage);
    if (!composer.write()) {
        std::cerr << "CfdSimulation: cannot write composed stage." << std::endl;
        return {};
    }

    // 6. Run solver
    return solver_->solve(composed_path, output_path);
}

} // namespace ufc
