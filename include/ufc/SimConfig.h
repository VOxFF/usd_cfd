#pragma once

namespace ufc {

struct SimConfig {
    double dt             = 0.01;   // time step in seconds
    int    num_steps      = 100;    // number of simulation steps
    double fluid_density  = 1.225;  // kg/m³ (air at sea level)
    double viscosity      = 1.8e-5; // Pa·s (air at 20°C)
};

} // namespace ufc
