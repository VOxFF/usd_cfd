"""
warp_solver.py  --  NVIDIA Warp CFD solver

Usage:
    python3 warp_solver.py <input.usd> <output.usd>

Expects the input USD stage to contain:
    /Envelope    -- watertight outer surface (boundary geometry)
    /FluidDomain -- simulation domain bounds

Writes simulation results (particle positions, velocities) to output.usd.
"""

import sys

# TODO: import warp as wp
# TODO: from pxr import Usd, UsdGeom


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input.usd> <output.usd>", file=sys.stderr)
        sys.exit(1)

    input_path  = sys.argv[1]
    output_path = sys.argv[2]

    # TODO: stage = Usd.Stage.Open(input_path)
    # TODO: extract /Envelope mesh as boundary
    # TODO: extract /FluidDomain bounds as simulation extent
    # TODO: initialize Warp simulation
    # TODO: run simulation steps
    # TODO: write results to output_path

    print(f"WarpSolver: input  = {input_path}")
    print(f"WarpSolver: output = {output_path}")
    print("WarpSolver: not implemented yet")
    sys.exit(1)


if __name__ == "__main__":
    main()
