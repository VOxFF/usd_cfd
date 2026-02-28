#pragma once

#include <ufc/ISolver.h>
#include <ufc/SimConfig.h>

#include <ufd/DomainBuilder.h>
#include <ufd/DomainConfig.h>
#include <ufd/EnvelopeBuilder.h>
#include <ufd/StageComposer.h>

#include <memory>
#include <string>

namespace ufc {

// Full CFD pipeline in a single object.
//
// run() executes:
//   1. Load input USD  (StageReader)
//   2. Extract bounds  (SurfaceExtractor)
//   3. Build domain    (DomainBuilder)
//   4. Build envelope  (EnvelopeBuilder)
//   5. Compose layers  (StageComposer)
//   6. Run solver      (ISolver)
//
// Intermediate USD layers are written alongside output_path.
class CfdSimulation {
public:
    CfdSimulation(const ufd::DomainConfig&   domain_config,
                  const ufd::EnvelopeConfig& envelope_config,
                  std::unique_ptr<ISolver>   solver);

    // Run the full pipeline.  Returns the solver output path on success,
    // or an empty string on failure.
    std::string run(const std::string& input_path,
                    const std::string& output_path) const;

private:
    ufd::DomainBuilder   domain_builder_;
    ufd::EnvelopeBuilder envelope_builder_;
    std::unique_ptr<ISolver> solver_;
};

} // namespace ufc
