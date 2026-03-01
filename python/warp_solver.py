"""
warp_solver.py  --  NVIDIA Warp SPH CFD solver

Usage:
    python3 warp_solver.py <input.usda> <output.usda>

Reads /FluidDomain bounds from the composed USD stage, runs an SPH
"dam break" simulation on the GPU, and writes an animated USD with
time-sampled particle positions to output.usda.

SPH kernels adapted from warp/examples/core/example_sph.py (Apache-2.0).
Reference: Müller et al. "Particle-based fluid simulation for interactive
applications." SCA 2003.
"""

import sys
import numpy as np
import warp as wp

from pxr import Usd, UsdGeom, Sdf, Vt, Gf


# ---- SPH helper functions (unchanged from example_sph.py) ------------------

@wp.func
def square(x: float):
    return x * x


@wp.func
def cube(x: float):
    return x * x * x


@wp.func
def density_kernel(xyz: wp.vec3, h: float):
    d2 = wp.dot(xyz, xyz)
    return wp.max(cube(square(h) - d2), 0.0)


@wp.func
def diff_pressure_kernel(
    xyz: wp.vec3,
    pressure: float,
    neighbor_pressure: float,
    neighbor_rho: float,
    h: float,
):
    dist = wp.sqrt(wp.dot(xyz, xyz))
    if dist < h:
        return (-xyz / dist) * ((neighbor_pressure + pressure) / (2.0 * neighbor_rho)) * square(h - dist)
    return wp.vec3()


@wp.func
def diff_viscous_kernel(
    xyz: wp.vec3,
    v: wp.vec3,
    neighbor_v: wp.vec3,
    neighbor_rho: float,
    h: float,
):
    dist = wp.sqrt(wp.dot(xyz, xyz))
    if dist < h:
        return ((neighbor_v - v) / neighbor_rho) * (h - dist)
    return wp.vec3()


# ---- SPH kernels ------------------------------------------------------------

@wp.kernel
def compute_density(
    grid:                  wp.uint64,
    particle_x:            wp.array(dtype=wp.vec3),
    particle_rho:          wp.array(dtype=float),
    density_normalization: float,
    smoothing_length:      float,
):
    tid = wp.tid()
    i   = wp.hash_grid_point_id(grid, tid)
    x   = particle_x[i]
    rho = float(0.0)

    neighbors = wp.hash_grid_query(grid, x, smoothing_length)
    for index in neighbors:
        rho += density_kernel(x - particle_x[index], smoothing_length)

    particle_rho[i] = density_normalization * rho


@wp.kernel
def get_acceleration(
    grid:                  wp.uint64,
    particle_x:            wp.array(dtype=wp.vec3),
    particle_v:            wp.array(dtype=wp.vec3),
    particle_rho:          wp.array(dtype=float),
    particle_a:            wp.array(dtype=wp.vec3),
    isotropic_exp:         float,
    base_density:          float,
    gravity:               float,
    pressure_normalization: float,
    viscous_normalization:  float,
    smoothing_length:       float,
):
    tid = wp.tid()
    i   = wp.hash_grid_point_id(grid, tid)
    x   = particle_x[i]
    v   = particle_v[i]
    rho = particle_rho[i]
    p   = isotropic_exp * (rho - base_density)

    press_f = wp.vec3()
    visc_f  = wp.vec3()

    neighbors = wp.hash_grid_query(grid, x, smoothing_length)
    for index in neighbors:
        if index != i:
            neighbor_rho = particle_rho[index]
            neighbor_p   = isotropic_exp * (neighbor_rho - base_density)
            rel_pos      = particle_x[index] - x
            press_f += diff_pressure_kernel(rel_pos, p, neighbor_p, neighbor_rho, smoothing_length)
            visc_f  += diff_viscous_kernel(rel_pos, v, particle_v[index], neighbor_rho, smoothing_length)

    force = pressure_normalization * press_f + viscous_normalization * visc_f
    particle_a[i] = force / rho + wp.vec3(0.0, gravity, 0.0)


@wp.kernel
def apply_bounds(
    particle_x:  wp.array(dtype=wp.vec3),
    particle_v:  wp.array(dtype=wp.vec3),
    damping:     float,
    bound_min:   wp.vec3,
    bound_max:   wp.vec3,
):
    tid = wp.tid()
    x = particle_x[tid]
    v = particle_v[tid]

    if x[0] < bound_min[0]:
        x = wp.vec3(bound_min[0], x[1], x[2])
        v = wp.vec3(v[0] * damping, v[1], v[2])
    if x[0] > bound_max[0]:
        x = wp.vec3(bound_max[0], x[1], x[2])
        v = wp.vec3(v[0] * damping, v[1], v[2])
    if x[1] < bound_min[1]:
        x = wp.vec3(x[0], bound_min[1], x[2])
        v = wp.vec3(v[0], v[1] * damping, v[2])
    if x[1] > bound_max[1]:
        x = wp.vec3(x[0], bound_max[1], x[2])
        v = wp.vec3(v[0], v[1] * damping, v[2])
    if x[2] < bound_min[2]:
        x = wp.vec3(x[0], x[1], bound_min[2])
        v = wp.vec3(v[0], v[1], v[2] * damping)
    if x[2] > bound_max[2]:
        x = wp.vec3(x[0], x[1], bound_max[2])
        v = wp.vec3(v[0], v[1], v[2] * damping)

    particle_x[tid] = x
    particle_v[tid] = v


@wp.kernel
def kick(particle_v: wp.array(dtype=wp.vec3), particle_a: wp.array(dtype=wp.vec3), dt: float):
    tid = wp.tid()
    particle_v[tid] = particle_v[tid] + particle_a[tid] * dt


@wp.kernel
def drift(particle_x: wp.array(dtype=wp.vec3), particle_v: wp.array(dtype=wp.vec3), dt: float):
    tid = wp.tid()
    particle_x[tid] = particle_x[tid] + particle_v[tid] * dt


@wp.kernel
def initialize_particles(
    particle_x:       wp.array(dtype=wp.vec3),
    smoothing_length: float,
    origin:           wp.vec3,
    nr_x:             int,
    nr_y:             int,
    nr_z:             int,
):
    tid = wp.tid()
    iz  = tid % nr_z
    iy  = (tid // nr_z) % nr_y
    ix  = (tid // (nr_z * nr_y)) % nr_x

    pos   = origin + smoothing_length * wp.vec3(wp.float(ix), wp.float(iy), wp.float(iz))
    state = wp.rand_init(123, tid)
    pos   = pos + 0.001 * smoothing_length * wp.vec3(wp.randn(state), wp.randn(state), wp.randn(state))

    particle_x[tid] = pos


# ---- USD helpers ------------------------------------------------------------

def create_output_stage(output_path, fps, num_frames):
    """Create a USD stage with a UsdGeomPoints prim for fluid particles."""
    stage = Usd.Stage.CreateNew(output_path)
    UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)
    stage.SetStartTimeCode(0)
    stage.SetEndTimeCode(num_frames)
    stage.SetFramesPerSecond(fps)

    # Points prim
    points_prim = UsdGeom.Points.Define(stage, "/FluidParticles")

    # Constant blue display color
    dc = points_prim.GetDisplayColorAttr()
    dc.Set(Vt.Vec3fArray([Gf.Vec3f(0.18, 0.46, 0.85)]))

    # Distant light for shading
    light = stage.DefinePrim("/DistantLight", "DistantLight")
    light.CreateAttribute("inputs:intensity", Sdf.ValueTypeNames.Float).Set(2000.0)
    light.CreateAttribute("xformOp:rotateXYZ", Sdf.ValueTypeNames.Float3).Set(Gf.Vec3f(-45, 45, 0))
    light.CreateAttribute("xformOpOrder", Sdf.ValueTypeNames.TokenArray).Set(
        Vt.TokenArray(["xformOp:rotateXYZ"])
    )

    return stage, points_prim


def write_frame(points_prim, positions_np, radius, frame):
    """Write one frame of particle positions to the Points prim."""
    pts = Vt.Vec3fArray([Gf.Vec3f(float(p[0]), float(p[1]), float(p[2])) for p in positions_np])
    widths = Vt.FloatArray([radius * 2.0] * len(positions_np))
    points_prim.GetPointsAttr().Set(pts, frame)
    points_prim.GetWidthsAttr().Set(widths, frame)


def read_domain_bounds(stage):
    """Return (np min, np max) of /FluidDomain mesh points."""
    prim = stage.GetPrimAtPath("/FluidDomain")
    if not prim.IsValid():
        return None, None
    pts = UsdGeom.Mesh(prim).GetPointsAttr().Get()
    if not pts:
        return None, None
    arr = np.array([[p[0], p[1], p[2]] for p in pts], dtype=np.float32)
    return arr.min(axis=0), arr.max(axis=0)


# ---- Main -------------------------------------------------------------------

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input.usda> <output.usda>", file=sys.stderr)
        sys.exit(1)

    input_path  = sys.argv[1]
    output_path = sys.argv[2]

    print(f"Opening: {input_path}")
    stage = Usd.Stage.Open(input_path)
    if not stage:
        print(f"Error: cannot open {input_path}", file=sys.stderr)
        sys.exit(1)

    # Read simulation domain
    bmin, bmax = read_domain_bounds(stage)
    if bmin is None:
        print("Error: /FluidDomain not found in stage", file=sys.stderr)
        sys.exit(1)

    size   = bmax - bmin
    width  = float(size[0])
    height = float(size[1])
    length = float(size[2])
    print(f"Domain: {bmin} -> {bmax}  size: ({width:.2f}, {height:.2f}, {length:.2f})")

    # Scale simulation parameters to domain size
    h      = min(width, height, length) * 0.05   # smoothing length = 5% of smallest axis
    h      = max(h, 0.01)                         # floor to avoid degenerate cases
    dt     = 0.01 * h
    fps    = 60
    steps_per_frame = max(1, int(32.0 / h))
    num_frames      = 240                         # 4 seconds at 60 fps

    # Particle block: one corner of the domain (x/4, full height, z/4)
    nr_x = max(1, int(width  / 4.0 / h))
    nr_y = max(1, int(height / h))
    nr_z = max(1, int(length / 4.0 / h))
    n    = nr_x * nr_y * nr_z

    # Simulation constants (same ratios as example_sph.py)
    particle_mass         = 0.01 * h ** 3
    base_density          = 1.0
    isotropic_exp         = 20.0
    dynamic_visc          = 0.025
    damping_coef          = -0.95
    gravity               = -0.1
    density_normalization = (315.0 * particle_mass) / (64.0 * np.pi * h ** 9)
    pressure_normalization= -(45.0 * particle_mass) / (np.pi * h ** 6)
    viscous_normalization = (45.0 * dynamic_visc * particle_mass) / (np.pi * h ** 6)

    print(f"SPH: h={h:.4f}  dt={dt:.5f}  particles={n}  steps/frame={steps_per_frame}")

    wp.init()
    device = "cuda:0"

    # Allocate arrays
    x = wp.empty(n, dtype=wp.vec3, device=device)
    v = wp.zeros(n, dtype=wp.vec3, device=device)
    a = wp.zeros(n, dtype=wp.vec3, device=device)
    rho = wp.zeros(n, dtype=float, device=device)

    origin_wp  = wp.vec3(float(bmin[0]), float(bmin[1]), float(bmin[2]))
    bound_min  = wp.vec3(float(bmin[0]), float(bmin[1]), float(bmin[2]))
    bound_max  = wp.vec3(float(bmax[0]), float(bmax[1]), float(bmax[2]))

    wp.launch(
        kernel=initialize_particles,
        dim=n,
        inputs=[x, h, origin_wp, nr_x, nr_y, nr_z],
        device=device,
    )

    grid_dim = max(1, int(min(width, height, length) / (4.0 * h)))
    grid = wp.HashGrid(grid_dim, grid_dim, grid_dim, device=device)

    # Create output USD stage with a UsdGeomPoints prim
    out_stage, points_prim = create_output_stage(output_path, fps, num_frames)

    print(f"Running {num_frames} frames × {steps_per_frame} steps on {device} ...")

    for frame in range(num_frames):
        for _ in range(steps_per_frame):
            grid.build(x, h)
            wp.launch(compute_density,    dim=n,
                inputs=[grid.id, x, rho, density_normalization, h], device=device)
            wp.launch(get_acceleration,   dim=n,
                inputs=[grid.id, x, v, rho, a, isotropic_exp, base_density,
                        gravity, pressure_normalization, viscous_normalization, h],
                device=device)
            wp.launch(apply_bounds,       dim=n,
                inputs=[x, v, damping_coef, bound_min, bound_max], device=device)
            wp.launch(kick,               dim=n, inputs=[v, a, dt], device=device)
            wp.launch(drift,              dim=n, inputs=[x, v, dt], device=device)

        write_frame(points_prim, x.numpy(), h, frame)

        if frame % 60 == 0:
            print(f"  frame {frame}/{num_frames}")

    out_stage.GetRootLayer().Save()
    print(f"Done. Written: {output_path}")


if __name__ == "__main__":
    main()
