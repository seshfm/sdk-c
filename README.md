# Sesh C/C++ SDK

Single-header SDK for building [Sesh](https://sesh.fm) audio plugins in C or C++.

## Quick start

```bash
git clone https://github.com/seshfm/sdk-c.git
cd sdk-c/examples/still-basin
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../../../cmake/sesh.cmake ..
cmake --build .
```

This produces `still_basin.wasm`. The toolchain automatically downloads [wasi-sdk](https://github.com/WebAssembly/wasi-sdk) on first run (Linux, macOS, Windows).

## Writing a plugin

### C++

```cpp
#include "sesh.h"

class MyDelay : public sesh::Plugin {
    float buf[44100] = {};
    int pos = 0;

public:
    MyDelay() {
        params = {
            {"Time",     0.0f, 1.0f, 0.3f},
            {"Feedback", 0.0f, 0.95f, 0.5f},
            {"Mix",      0.0f, 1.0f, 0.5f},
        };
    }

    void process(sesh::ProcessContext& ctx) override {
        for (int ch = 0; ch < ctx.num_channels; ch++) {
            for (int i = 0; i < ctx.frames; i++) {
                float delay_samples = ctx.params[0][i] * 44100.0f;
                int read_pos = (pos + 44100 - (int)delay_samples) % 44100;
                float delayed = buf[read_pos];
                float out = ctx.channels[ch][i] + delayed * ctx.params[2][i];
                buf[pos] = ctx.channels[ch][i] + delayed * ctx.params[1][i];
                ctx.channels[ch][i] = out;
                pos = (pos + 1) % 44100;
            }
        }
    }
};

SESH_PLUGIN_CPP(MyDelay)
```

### C

```c
#include "sesh.h"

typedef struct { float gain; } MyGain;

static SeshParam my_params[] = {
    { "Gain", 0.0f, 1.0f, 0.5f },
};

static void* my_create(float sr)  { (void)sr; return sesh_calloc(sizeof(MyGain)); }
static void  my_destroy(void* p)  { sesh_dealloc(p, sizeof(MyGain)); }

static void my_process(void* inst, SeshProcessContext* ctx) {
    for (int ch = 0; ch < ctx->num_channels; ch++)
        for (int i = 0; i < ctx->frames; i++)
            ctx->channels[ch][i] *= ctx->params[0][i];
}

SESH_PLUGIN(
    .create     = my_create,
    .destroy    = my_destroy,
    .process    = my_process,
    .params     = my_params,
    .num_params = 1,
)
```

## What's in the header

`sesh.h` is a single file containing everything a plugin needs:

- **Plugin interface**: `SESH_PLUGIN()` (C) and `SESH_PLUGIN_CPP()` (C++) macros that generate all required WASM exports
- **Draw API**: `sesh_draw_rect`, `sesh_draw_image`, `sesh_draw_knob_strip`, etc.
- **Scratch buffers**: `SeshScratch` pool for intermediate DSP buffers
- **Vec DSP ops**: batch audio processing functions with host-accelerated WASM imports when available, inline fallbacks otherwise:
  - Basic math: copy, fill, add, mul, mul_add, clamp
  - Circular buffers: ring_write, ring_read
  - Delay: modulated delay read with linear interpolation
  - Filters: one-pole (6dB/oct), biquad (8 types, per-sample modulated)
  - Comb filters: single, parallel (Schroeder), coupled (FDN with N*N matrix)
  - Allpass: Schroeder allpass diffuser
  - Oscillator: sine, triangle, saw, square
  - Envelope: peak/RMS follower with attack/release
  - Waveshaping: tanh saturation, hard clip
  - Math: abs, neg, sqrt, recip, div, pow, log, exp

## Build requirements

- [CMake](https://cmake.org/) 3.16+
- An internet connection (first build only, to download wasi-sdk)

Or set `WASI_SDK_PATH` to skip the download if you already have [wasi-sdk](https://github.com/WebAssembly/wasi-sdk) installed.

## CMakeLists.txt for your plugin

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_plugin CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(my_plugin my_plugin.cpp)
target_include_directories(my_plugin PRIVATE /path/to/sdk-c)
sesh_plugin(my_plugin)
```

Build:
```bash
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/sdk-c/cmake/sesh.cmake ..
cmake --build .
```
