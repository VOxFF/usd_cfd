# USD CFD

USD CFD — Load a USD scene, generate a watertight envelope surface and a far-field simulation domain, then evaluate CFD using NVIDIA Warp. Built on OpenUSD, OpenVDB, and Warp for a fully GPU-accelerated preprocessing and simulation pipeline.

## Prerequisites

### C++ build

| Requirement | Version | Notes |
|-------------|---------|-------|
| CMake | 3.20+ | |
| OpenUSD | 21.x+ | Set `USD_ROOT` to install path |
| OpenVDB | 8.x | `libopenvdb-dev` on Ubuntu/Debian |
| GCC | 12+ | C++17 required |

### Python solver (NVIDIA Warp)

| Requirement | Notes |
|-------------|-------|
| Python | 3.8+ |
| warp-lang | `pip install warp-lang` |
| pxr (USD Python bindings) | Included in USD install — add to `PYTHONPATH` |
| NVIDIA GPU | CUDA-capable; CPU fallback available but slow |

**Verify your environment:**
```sh
python3 -c "import warp; print(warp.__version__)"
python3 -c "from pxr import Usd; print('USD ok')"
nvidia-smi
```

**Set USD Python bindings on path** (adjust to your install location):
```sh
export PYTHONPATH=/path/to/usd_install/lib/python:$PYTHONPATH
```

## Build

```sh
git clone --recurse-submodules <repo>
cmake -B build -DUSD_ROOT=/path/to/usd_install
cmake --build build
ctest --test-dir build
```

## Usage

```sh
usd_cfd <input.usd> <output.usda>
```

The pipeline produces:

| File | Contents |
|------|----------|
| `<output>.domain.usda` | Fluid domain mesh (`/FluidDomain`) |
| `<output>.envelope.usda` | Watertight envelope mesh (`/Envelope`) |
| `<output>.composed.usda` | Composed USD layer (input + domain + envelope) |
| `<output>.usda` | Solver results |
