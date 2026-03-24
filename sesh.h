/*
 * Sesh Audio Plugin SDK
 * Single-header SDK for building Sesh audio plugins in C or C++.
 *
 * Usage (C):
 *   #define SESH_IMPLEMENTATION
 *   #include "sesh.h"
 *
 *   static SeshParam my_params[] = {
 *       { "Drive", 0.0f, 1.0f, 0.5f },
 *       { "Mix",   0.0f, 1.0f, 1.0f },
 *   };
 *
 *   typedef struct { float drive_state; } MyPlugin;
 *
 *   static void* my_create(float sample_rate) {
 *       MyPlugin* p = (MyPlugin*)sesh_calloc(sizeof(MyPlugin));
 *       return p;
 *   }
 *
 *   static void my_destroy(void* instance) {
 *       sesh_dealloc(instance, sizeof(MyPlugin));
 *   }
 *
 *   static void my_process(void* instance, SeshProcessContext* ctx) {
 *       MyPlugin* p = (MyPlugin*)instance;
 *       for (int ch = 0; ch < ctx->num_channels; ch++) {
 *           for (int i = 0; i < ctx->frames; i++) {
 *               ctx->channels[ch][i] *= ctx->params[0][i]; // Drive
 *           }
 *       }
 *   }
 *
 *   SESH_PLUGIN(
 *       .create  = my_create,
 *       .destroy = my_destroy,
 *       .process = my_process,
 *       .params  = my_params,
 *       .num_params = 2,
 *   )
 *
 * Usage (C++):
 *   #define SESH_IMPLEMENTATION
 *   #include "sesh.h"
 *
 *   class MyPlugin : public sesh::Plugin {
 *   public:
 *       MyPlugin() {
 *           params.push_back({"Drive", 0.0f, 1.0f, 0.5f});
 *           params.push_back({"Mix",   0.0f, 1.0f, 1.0f});
 *       }
 *
 *       void process(sesh::ProcessContext& ctx) override {
 *           for (int ch = 0; ch < ctx.num_channels; ch++) {
 *               for (int i = 0; i < ctx.frames; i++) {
 *                   ctx.channels[ch][i] *= ctx.params[0][i];
 *               }
 *           }
 *       }
 *   };
 *
 *   SESH_PLUGIN_CPP(MyPlugin)
 */

#ifndef SESH_H
#define SESH_H

#include <stdint.h>
#include <stddef.h>

/* -----------------------------------------------------------------------
 * ABI version
 * ----------------------------------------------------------------------- */

#define SESH_SDK_VERSION 4

/* -----------------------------------------------------------------------
 * Core types (C, visible to both C and C++)
 * ----------------------------------------------------------------------- */

typedef struct SeshParam {
    char name[64];  /* null-terminated UTF-8 */
    float min;
    float max;
    float default_value;
} SeshParam;

typedef struct SeshParamList {
    uint32_t count;
    const SeshParam* params;
} SeshParamList;

typedef struct SeshProcessContext {
    float** channels;           /* array of channel buffers [num_channels][frames] */
    int num_channels;
    int frames;
    const float* const* params; /* array of param buffers [num_params][frames] */
    int num_params;
} SeshProcessContext;

/* -----------------------------------------------------------------------
 * Draw types
 * ----------------------------------------------------------------------- */

typedef uint32_t SeshImageId;

typedef struct SeshDrawContext {
    float width;
    float height;
} SeshDrawContext;

/* -----------------------------------------------------------------------
 * Host imports (provided by the Sesh runtime, WASM only)
 * ----------------------------------------------------------------------- */

#ifdef __wasm__

__attribute__((import_module("env"), import_name("sesh_draw_rect_host")))
extern void sesh_draw_rect_host(float x, float y, float w, float h, uint32_t color);

__attribute__((import_module("env"), import_name("sesh_draw_knob_host")))
extern void sesh_draw_knob_host(uint32_t param_index, float x, float y, float size);

__attribute__((import_module("env"), import_name("sesh_load_image_host")))
extern uint32_t sesh_load_image_host(uint32_t ptr, uint32_t len);

__attribute__((import_module("env"), import_name("sesh_draw_image_host")))
extern void sesh_draw_image_host(uint32_t id, float x, float y);

__attribute__((import_module("env"), import_name("sesh_draw_bg_image_host")))
extern void sesh_draw_bg_image_host(uint32_t id);

__attribute__((import_module("env"), import_name("sesh_draw_knob_strip_host")))
extern void sesh_draw_knob_strip_host(
    uint32_t param_index, uint32_t id,
    float x, float y, float w, float h, uint32_t frame_count,
    float touch_x, float touch_y, float touch_w, float touch_h);

#endif /* __wasm__ */

/* -----------------------------------------------------------------------
 * Draw API (inline, no-ops on non-WASM)
 * ----------------------------------------------------------------------- */

static inline void sesh_draw_rect(float x, float y, float w, float h, uint32_t color) {
#ifdef __wasm__
    sesh_draw_rect_host(x, y, w, h, color);
#else
    (void)x; (void)y; (void)w; (void)h; (void)color;
#endif
}

static inline void sesh_draw_knob(uint32_t param_index, float x, float y, float size) {
#ifdef __wasm__
    sesh_draw_knob_host(param_index, x, y, size);
#else
    (void)param_index; (void)x; (void)y; (void)size;
#endif
}

static inline SeshImageId sesh_load_image(const void* data, uint32_t len) {
#ifdef __wasm__
    return sesh_load_image_host((uint32_t)(uintptr_t)data, len);
#else
    (void)data; (void)len;
    return 1;
#endif
}

static inline void sesh_draw_image(SeshImageId id, float x, float y) {
#ifdef __wasm__
    sesh_draw_image_host(id, x, y);
#else
    (void)id; (void)x; (void)y;
#endif
}

static inline void sesh_draw_bg_image(SeshImageId id) {
#ifdef __wasm__
    sesh_draw_bg_image_host(id);
#else
    (void)id;
#endif
}

static inline void sesh_draw_knob_strip(
    uint32_t param_index, SeshImageId id,
    float x, float y, float w, float h, uint32_t frame_count,
    float touch_x, float touch_y, float touch_w, float touch_h)
{
#ifdef __wasm__
    sesh_draw_knob_strip_host(param_index, id, x, y, w, h, frame_count,
                              touch_x, touch_y, touch_w, touch_h);
#else
    (void)param_index; (void)id; (void)x; (void)y; (void)w; (void)h;
    (void)frame_count; (void)touch_x; (void)touch_y; (void)touch_w; (void)touch_h;
#endif
}

/* -----------------------------------------------------------------------
 * Scratch buffer pool
 * ----------------------------------------------------------------------- */

#ifndef SESH_SCRATCH_BUFS
#define SESH_SCRATCH_BUFS 128
#endif
#ifndef SESH_SCRATCH_FRAMES
#define SESH_SCRATCH_FRAMES 2048
#endif

typedef struct SeshScratch {
    float* data;            /* SESH_SCRATCH_BUFS * SESH_SCRATCH_FRAMES floats */
    int cursor;
} SeshScratch;

static inline float* sesh_scratch_buf(SeshScratch* s, int frames) {
    (void)frames;
    int idx = s->cursor % SESH_SCRATCH_BUFS;
    s->cursor = idx + 1;
    return s->data + idx * SESH_SCRATCH_FRAMES;
}

/* -----------------------------------------------------------------------
 * C plugin definition + SESH_PLUGIN macro
 *
 * The macro generates the WASM exports that the Sesh host expects.
 * ----------------------------------------------------------------------- */

typedef struct SeshPluginDef {
    void* (*create)(float sample_rate);
    void  (*destroy)(void* instance);
    void  (*process)(void* instance, SeshProcessContext* ctx);
    void  (*draw)(void* instance, SeshDrawContext* ctx);    /* optional, NULL if no GUI */
    float draw_width;                                       /* GUI width, 0 if no GUI */
    const SeshParam* params;
    int num_params;
} SeshPluginDef;

/*
 * SESH_PLUGIN(def) - Register a C plugin.
 *
 * Pass a compound literal:
 *   SESH_PLUGIN(
 *       .create  = my_create,
 *       .destroy = my_destroy,
 *       .process = my_process,
 *       .params  = my_params,
 *       .num_params = 2,
 *   )
 */
#define SESH_PLUGIN(...)                                                        \
    static const SeshPluginDef sesh__plugin_def = { __VA_ARGS__ };              \
    SESH__EXPORTS(sesh__plugin_def)

/* -----------------------------------------------------------------------
 * Export generation (shared by C and C++ macros)
 * ----------------------------------------------------------------------- */

#ifdef __wasm__
#define SESH__EXPORT(name) __attribute__((export_name(#name)))
#else
#define SESH__EXPORT(name)
#endif

#ifdef __cplusplus
#define SESH__EXTERN_C extern "C"
#else
#define SESH__EXTERN_C
#endif

#define SESH__EXPORTS(def)                                                      \
                                                                                \
typedef struct {                                                                \
    void* user;                                                                 \
    SeshParam* params_storage;                                                  \
    SeshParamList param_list;                                                   \
    SeshScratch scratch;                                                        \
} SeshInstance;                                                                 \
                                                                                \
SESH__EXPORT(sesh_sdk_version) SESH__EXTERN_C                 \
uint32_t sesh_sdk_version(void) { return SESH_SDK_VERSION; }                    \
                                                                                \
SESH__EXPORT(sesh_alloc) SESH__EXTERN_C                       \
void* sesh_alloc(uint32_t size, uint32_t align) {                               \
    extern void* aligned_alloc(size_t, size_t);                                 \
    return aligned_alloc(align, size);                                          \
}                                                                               \
                                                                                \
SESH__EXPORT(sesh_free) SESH__EXTERN_C                        \
void sesh_free(void* ptr, uint32_t size, uint32_t align) {                      \
    extern void free(void*);                                                    \
    (void)size; (void)align;                                                    \
    free(ptr);                                                                  \
}                                                                               \
                                                                                \
SESH__EXPORT(sesh_create) SESH__EXTERN_C                      \
void* sesh_create(void) {                                                       \
    SeshInstance* inst = (SeshInstance*)malloc(sizeof(SeshInstance));             \
    inst->user = (def).create(44100.0f);                                        \
    int np = (def).num_params;                                                  \
    const SeshParam* src = (def).params;                                        \
    inst->params_storage = (SeshParam*)malloc(                                  \
        (np > 0 ? np : 1) * sizeof(SeshParam));                                \
    for (int i = 0; i < np; i++) inst->params_storage[i] = src[i];             \
    inst->param_list.count = (uint32_t)np;                                      \
    inst->param_list.params = inst->params_storage;                             \
    inst->scratch.data = (float*)malloc(                                         \
        SESH_SCRATCH_BUFS * SESH_SCRATCH_FRAMES * sizeof(float));              \
    inst->scratch.cursor = 0;                                                   \
    return inst;                                                                \
}                                                                               \
                                                                                \
SESH__EXPORT(sesh_destroy) SESH__EXTERN_C                     \
void sesh_destroy(void* raw) {                                                  \
    if (!raw) return;                                                           \
    SeshInstance* inst = (SeshInstance*)raw;                                     \
    (def).destroy(inst->user);                                                  \
    free(inst->params_storage);                                                 \
    free(inst->scratch.data);                                                   \
    free(inst);                                                                 \
}                                                                               \
                                                                                \
SESH__EXPORT(sesh_params) SESH__EXTERN_C                      \
const SeshParamList* sesh_params(void* raw) {                                   \
    SeshInstance* inst = (SeshInstance*)raw;                                     \
    return &inst->param_list;                                                   \
}                                                                               \
                                                                                \
SESH__EXPORT(sesh_process) SESH__EXTERN_C                     \
void sesh_process(void* raw, float* ptr, uint32_t frames,                       \
                  uint32_t channels, const float* const* params_ptr,            \
                  uint32_t param_count)                                         \
{                                                                               \
    if (frames == 0 || channels == 0) return;                                   \
    SeshInstance* inst = (SeshInstance*)raw;                                     \
    float* ch_ptrs[32];                                                         \
    for (uint32_t c = 0; c < channels && c < 32; c++)                           \
        ch_ptrs[c] = ptr + c * frames;                                          \
    SeshProcessContext ctx;                                                      \
    ctx.channels = ch_ptrs;                                                     \
    ctx.num_channels = (int)channels;                                           \
    ctx.frames = (int)frames;                                                   \
    ctx.params = params_ptr;                                                    \
    ctx.num_params = (int)param_count;                                          \
    (def).process(inst->user, &ctx);                                            \
}                                                                               \
                                                                                \
SESH__EXPORT(sesh_draw) SESH__EXTERN_C                        \
int32_t sesh_draw(void* raw) {                                                  \
    SeshInstance* inst = (SeshInstance*)raw;                                     \
    if (!(def).draw || (def).draw_width == 0.0f) return 0;                      \
    SeshDrawContext ctx;                                                         \
    ctx.width = (def).draw_width;                                               \
    ctx.height = 244.0f;                                                        \
    (def).draw(inst->user, &ctx);                                               \
    return 1;                                                                   \
}                                                                               \
                                                                                \
SESH__EXPORT(sesh_draw_width) SESH__EXTERN_C                  \
float sesh_draw_width(void* raw) {                                              \
    (void)raw;                                                                  \
    return (def).draw_width;                                                    \
}

/* -----------------------------------------------------------------------
 * Memory helpers
 * ----------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>

static inline void* sesh_calloc(size_t size) {
    void* p = malloc(size);
    if (p) memset(p, 0, size);
    return p;
}

static inline void sesh_dealloc(void* p, size_t size) {
    (void)size;
    free(p);
}

/* -----------------------------------------------------------------------
 * C++ wrapper
 * ----------------------------------------------------------------------- */

#ifdef __cplusplus

#include <vector>
#include <cstring>

namespace sesh {

struct ProcessContext {
    float** channels;
    int num_channels;
    int frames;
    const float* const* params;
    int num_params;
};

struct DrawContext {
    float width;
    float height;

    void rect(float x, float y, float w, float h, uint32_t color) const {
        sesh_draw_rect(x, y, w, h, color);
    }
    void knob(uint32_t param_index, float x, float y, float size) const {
        sesh_draw_knob(param_index, x, y, size);
    }
    SeshImageId load_image(const void* data, uint32_t len) const {
        return sesh_load_image(data, len);
    }
    void image(SeshImageId id, float x, float y) const {
        sesh_draw_image(id, x, y);
    }
    void bg_image(SeshImageId id) const {
        sesh_draw_bg_image(id);
    }
    void knob_strip(uint32_t param_index, SeshImageId id,
                    float x, float y, float w, float h, uint32_t frame_count,
                    float touch_x, float touch_y, float touch_w, float touch_h) const {
        sesh_draw_knob_strip(param_index, id, x, y, w, h, frame_count,
                             touch_x, touch_y, touch_w, touch_h);
    }
};

struct Param : SeshParam {
    Param(const char* n, float min_, float max_, float default_) {
        std::memset(name, 0, sizeof(name));
        size_t len = std::strlen(n);
        if (len > 63) len = 63;
        std::memcpy(name, n, len);
        min = min_;
        max = max_;
        default_value = default_;
    }
};

class Plugin {
public:
    std::vector<Param> params;
    float draw_width = 0.0f;

    virtual ~Plugin() = default;
    virtual void process(ProcessContext& ctx) = 0;
    virtual void draw(DrawContext& ctx) { (void)ctx; }
};

} /* namespace sesh */

/*
 * SESH_PLUGIN_CPP(ClassName) - Register a C++ plugin.
 *
 * The class must inherit from sesh::Plugin and implement process().
 */
#define SESH_PLUGIN_CPP(T)                                                      \
    static void* sesh__cpp_create(float);                                       \
    static void sesh__cpp_destroy(void*);                                       \
    static void sesh__cpp_process(void*, SeshProcessContext*);                  \
    static void sesh__cpp_draw(void*, SeshDrawContext*);                        \
    static SeshPluginDef sesh__plugin_def = {                                   \
        sesh__cpp_create,                                                       \
        sesh__cpp_destroy,                                                      \
        sesh__cpp_process,                                                      \
        sesh__cpp_draw,                                                         \
        0.0f,                                                                   \
        nullptr,                                                                \
        0,                                                                      \
    };                                                                          \
    SESH__EXPORTS(sesh__plugin_def)                                             \
    static void* sesh__cpp_create(float sr) {                                   \
        (void)sr;                                                               \
        T* p = new T();                                                         \
        sesh__plugin_def.params = (const SeshParam*)p->params.data();           \
        sesh__plugin_def.num_params = (int)p->params.size();                    \
        sesh__plugin_def.draw_width = p->draw_width;                            \
        return p;                                                               \
    }                                                                           \
    static void sesh__cpp_destroy(void* inst) {                                 \
        delete static_cast<T*>(inst);                                           \
    }                                                                           \
    static void sesh__cpp_process(void* inst, SeshProcessContext* ctx) {        \
        sesh::ProcessContext cpp_ctx;                                            \
        cpp_ctx.channels = ctx->channels;                                       \
        cpp_ctx.num_channels = ctx->num_channels;                               \
        cpp_ctx.frames = ctx->frames;                                           \
        cpp_ctx.params = ctx->params;                                           \
        cpp_ctx.num_params = ctx->num_params;                                   \
        static_cast<T*>(inst)->process(cpp_ctx);                                \
    }                                                                           \
    static void sesh__cpp_draw(void* inst, SeshDrawContext* ctx) {              \
        sesh::DrawContext cpp_ctx;                                              \
        cpp_ctx.width = ctx->width;                                             \
        cpp_ctx.height = ctx->height;                                           \
        static_cast<T*>(inst)->draw(cpp_ctx);                                   \
    }

#endif /* __cplusplus */

/* -----------------------------------------------------------------------
 * Vec DSP types
 * ----------------------------------------------------------------------- */

typedef enum SeshWaveform {
    SESH_WAVEFORM_SINE     = 0,
    SESH_WAVEFORM_TRIANGLE = 1,
    SESH_WAVEFORM_SAW      = 2,
    SESH_WAVEFORM_SQUARE   = 3,
} SeshWaveform;

typedef enum SeshFilterType {
    SESH_FILTER_LOWPASS   = 0,
    SESH_FILTER_HIGHPASS  = 1,
    SESH_FILTER_BANDPASS  = 2,
    SESH_FILTER_NOTCH     = 3,
    SESH_FILTER_PEAK      = 4,
    SESH_FILTER_LOWSHELF  = 5,
    SESH_FILTER_HIGHSHELF = 6,
    SESH_FILTER_ALLPASS   = 7,
} SeshFilterType;

typedef enum SeshEnvelopeMode {
    SESH_ENVELOPE_PEAK = 0,
    SESH_ENVELOPE_RMS  = 1,
} SeshEnvelopeMode;

typedef struct SeshBiquadState {
    float x1, x2, y1, y2;
} SeshBiquadState;

typedef struct SeshEnvelopeState {
    float current;
} SeshEnvelopeState;

typedef struct SeshOnePoleState {
    float y1;
} SeshOnePoleState;

/* -----------------------------------------------------------------------
 * Vec host imports (WASM only, provided by Sesh runtime)
 * ----------------------------------------------------------------------- */

#ifdef __wasm__

__attribute__((import_module("env"), import_name("sesh_vec_version")))
extern uint32_t sesh_vec_version_host(void);

__attribute__((import_module("env"), import_name("sesh_vec_copy_host")))
extern void sesh_vec_copy_host(float* dst, const float* src, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_fill_host")))
extern void sesh_vec_fill_host(float* dst, float value, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_add_host")))
extern void sesh_vec_add_host(float* dst, const float* a, const float* b, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_add_scalar_host")))
extern void sesh_vec_add_scalar_host(float* dst, float value, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_mul_host")))
extern void sesh_vec_mul_host(float* dst, const float* a, const float* b, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_mul_scalar_host")))
extern void sesh_vec_mul_scalar_host(float* dst, float value, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_mul_add_host")))
extern void sesh_vec_mul_add_host(float* dst, const float* src, float gain, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_clamp_host")))
extern void sesh_vec_clamp_host(float* dst, const float* src, float min, float max, uint32_t len);

__attribute__((import_module("env"), import_name("sesh_vec_ring_write_host")))
extern void sesh_vec_ring_write_host(float* buf, uint32_t buf_len, uint32_t* pos, const float* src, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_ring_read_host")))
extern void sesh_vec_ring_read_host(const float* buf, uint32_t buf_len, uint32_t pos, float* dst, uint32_t offset, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_delay_read_host")))
extern void sesh_vec_delay_read_host(const float* buf, uint32_t buf_len, uint32_t pos, float* dst, const float* time, uint32_t len);

__attribute__((import_module("env"), import_name("sesh_vec_schroeder_allpass_host")))
extern void sesh_vec_schroeder_allpass_host(float* buf, uint32_t buf_len, uint32_t* pos, float* dst, const float* src, uint32_t delay, float g, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_one_pole_host")))
extern void sesh_vec_one_pole_host(float* state, float* dst, const float* src, float coefficient, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_comb_host")))
extern void sesh_vec_comb_host(float* buf, uint32_t buf_len, uint32_t* pos, float* damp, float* dst, const float* src, const float* time, float feedback, float damping, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_comb_parallel_host")))
extern void sesh_vec_comb_parallel_host(const float* const* bufs, const uint32_t* buf_lens, uint32_t* positions, float* damp, const float* const* dst, const float* src, const float* const* time, float feedback, float damping, uint32_t n, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_comb_coupled_host")))
extern void sesh_vec_comb_coupled_host(const float* const* bufs, const uint32_t* buf_lens, uint32_t* positions, float* damp, const float* const* dst, const float* const* src, const float* const* time, const float* matrix, float damping, uint32_t n, uint32_t len);

__attribute__((import_module("env"), import_name("sesh_vec_osc_host")))
extern void sesh_vec_osc_host(float* phase, float* dst, float freq, uint32_t waveform, float sample_rate, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_biquad_host")))
extern void sesh_vec_biquad_host(float* state, float* dst, const float* src, const float* cutoff, const float* q, const float* gain, uint32_t filter_type, float sample_rate, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_envelope_host")))
extern void sesh_vec_envelope_host(float* state, float* dst, const float* src, const float* attack, const float* release, uint32_t mode, float sample_rate, uint32_t len);

__attribute__((import_module("env"), import_name("sesh_vec_tanh_host")))
extern void sesh_vec_tanh_host(float* dst, const float* src, const float* drive, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_hard_clip_host")))
extern void sesh_vec_hard_clip_host(float* dst, const float* src, const float* threshold, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_abs_host")))
extern void sesh_vec_abs_host(float* dst, const float* src, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_neg_host")))
extern void sesh_vec_neg_host(float* dst, const float* src, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_sqrt_host")))
extern void sesh_vec_sqrt_host(float* dst, const float* src, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_recip_host")))
extern void sesh_vec_recip_host(float* dst, const float* src, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_div_host")))
extern void sesh_vec_div_host(float* dst, const float* a, const float* b, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_pow_host")))
extern void sesh_vec_pow_host(float* dst, const float* src, const float* exp, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_log_host")))
extern void sesh_vec_log_host(float* dst, const float* src, uint32_t len);
__attribute__((import_module("env"), import_name("sesh_vec_exp_host")))
extern void sesh_vec_exp_host(float* dst, const float* src, uint32_t len);

static uint32_t sesh__vec_version_cached = 0;
static inline int sesh__use_host_vec(void) {
    if (sesh__vec_version_cached == 0) {
        uint32_t v = sesh_vec_version_host();
        sesh__vec_version_cached = (v == 0) ? 0xFFFFFFFFu : v;
    }
    return sesh__vec_version_cached != 0xFFFFFFFFu;
}

#else
static inline int sesh__use_host_vec(void) { return 0; }
#endif /* __wasm__ */

/* -----------------------------------------------------------------------
 * Vec DSP ops: basic math
 * ----------------------------------------------------------------------- */

#include <math.h>

static inline void sesh_vec_copy(float* dst, const float* src, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_copy_host(dst, src, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = src[i];
}

static inline void sesh_vec_fill(float* dst, float value, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_fill_host(dst, value, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = value;
}

static inline void sesh_vec_add(float* dst, const float* a, const float* b, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_add_host(dst, a, b, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = a[i] + b[i];
}

static inline void sesh_vec_add_scalar(float* dst, float value, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_add_scalar_host(dst, value, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] += value;
}

static inline void sesh_vec_mul(float* dst, const float* a, const float* b, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_mul_host(dst, a, b, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = a[i] * b[i];
}

static inline void sesh_vec_mul_scalar(float* dst, float value, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_mul_scalar_host(dst, value, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] *= value;
}

static inline void sesh_vec_mul_add(float* dst, const float* src, float gain, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_mul_add_host(dst, src, gain, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] += src[i] * gain;
}

static inline void sesh_vec_clamp(float* dst, const float* src, float min, float max, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_clamp_host(dst, src, min, max, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) {
        float v = src[i];
        dst[i] = v < min ? min : (v > max ? max : v);
    }
}

/* -----------------------------------------------------------------------
 * Vec DSP ops: circular buffer
 * ----------------------------------------------------------------------- */

static inline void sesh_vec_ring_write(float* buf, int buf_len, int* pos, const float* src, int frames) {
#ifdef __wasm__
    if (sesh__use_host_vec()) {
        uint32_t p = (uint32_t)*pos;
        sesh_vec_ring_write_host(buf, (uint32_t)buf_len, &p, src, (uint32_t)frames);
        *pos = (int)p;
        return;
    }
#endif
    for (int i = 0; i < frames; i++) {
        buf[(*pos + i) % buf_len] = src[i];
    }
    *pos = (*pos + frames) % buf_len;
}

static inline void sesh_vec_ring_read(const float* buf, int buf_len, int pos, float* dst, int offset, int frames) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_ring_read_host(buf, (uint32_t)buf_len, (uint32_t)pos, dst, (uint32_t)offset, (uint32_t)frames); return; }
#endif
    int start = (pos + buf_len - offset) % buf_len;
    for (int i = 0; i < frames; i++) {
        dst[i] = buf[(start + i) % buf_len];
    }
}

/* -----------------------------------------------------------------------
 * Vec DSP ops: modulated delay read (linear interpolation)
 * ----------------------------------------------------------------------- */

static inline void sesh_vec_delay_read(const float* buf, int buf_len, int pos,
                                        float* dst, const float* time, int frames) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_delay_read_host(buf, (uint32_t)buf_len, (uint32_t)pos, dst, time, (uint32_t)frames); return; }
#endif
    for (int i = 0; i < frames; i++) {
        int write_pos_at_i = (pos + buf_len - frames + i) % buf_len;
        int delay_int = (int)time[i];
        float delay_frac = time[i] - (float)delay_int;
        int idx1 = (write_pos_at_i + buf_len - delay_int) % buf_len;
        int idx2 = (idx1 + buf_len - 1) % buf_len;
        dst[i] = buf[idx1] + delay_frac * (buf[idx2] - buf[idx1]);
    }
}

/* -----------------------------------------------------------------------
 * Vec DSP ops: Schroeder allpass
 * ----------------------------------------------------------------------- */

static inline void sesh_vec_schroeder_allpass(float* buf, int buf_len, int* pos,
                                               float* dst, const float* src,
                                               int delay, float g, int frames) {
#ifdef __wasm__
    if (sesh__use_host_vec()) {
        uint32_t p = (uint32_t)*pos;
        sesh_vec_schroeder_allpass_host(buf, (uint32_t)buf_len, &p, dst, src, (uint32_t)delay, g, (uint32_t)frames);
        *pos = (int)p;
        return;
    }
#endif
    int wp = *pos;
    for (int i = 0; i < frames; i++) {
        int read_idx = (wp + buf_len - delay) % buf_len;
        float buf_out = buf[read_idx];
        float v = src[i] + g * buf_out;
        dst[i] = buf_out - g * v;
        buf[wp] = v;
        wp = (wp + 1) % buf_len;
    }
    *pos = wp;
}

/* -----------------------------------------------------------------------
 * Vec DSP ops: one-pole filter (6 dB/oct lowpass)
 * ----------------------------------------------------------------------- */

static inline void sesh_vec_one_pole(SeshOnePoleState* state, float* dst,
                                      const float* src, float coefficient, int frames) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_one_pole_host(&state->y1, dst, src, coefficient, (uint32_t)frames); return; }
#endif
    float y = state->y1;
    for (int i = 0; i < frames; i++) {
        y = src[i] + coefficient * (y - src[i]);
        dst[i] = y;
    }
    state->y1 = y;
}

/* -----------------------------------------------------------------------
 * Vec DSP ops: comb filter (single delay line with feedback + damping)
 * ----------------------------------------------------------------------- */

static inline void sesh_vec_comb(float* buf, int buf_len, int* pos,
                                  SeshOnePoleState* damp, float* dst, const float* src,
                                  const float* time, float feedback, float damping, int frames) {
#ifdef __wasm__
    if (sesh__use_host_vec()) {
        uint32_t p = (uint32_t)*pos;
        sesh_vec_comb_host(buf, (uint32_t)buf_len, &p, &damp->y1, dst, src, time, feedback, damping, (uint32_t)frames);
        *pos = (int)p;
        return;
    }
#endif
    int wp = *pos;
    float y = damp->y1;
    for (int i = 0; i < frames; i++) {
        int delay_int = (int)time[i];
        float delay_frac = time[i] - (float)delay_int;
        int idx1 = (wp + buf_len - delay_int) % buf_len;
        int idx2 = (idx1 + buf_len - 1) % buf_len;
        float tap = buf[idx1] + delay_frac * (buf[idx2] - buf[idx1]);
        y = tap + damping * (y - tap);
        dst[i] = y;
        buf[wp] = src[i] + feedback * y;
        wp = (wp + 1) % buf_len;
    }
    *pos = wp;
    damp->y1 = y;
}

/* -----------------------------------------------------------------------
 * Vec DSP ops: parallel and coupled comb filters
 * ----------------------------------------------------------------------- */

#ifndef SESH_FDN_MAX_LINES
#define SESH_FDN_MAX_LINES 16
#endif

static inline void sesh_vec_comb_parallel(
    float** bufs, const int* buf_lens, int* positions,
    SeshOnePoleState* damp, float** dst, const float* src,
    const float* const* time, float feedback, float damping,
    int n, int frames)
{
#ifdef __wasm__
    if (sesh__use_host_vec()) {
        uint32_t bl32[SESH_FDN_MAX_LINES], pos32[SESH_FDN_MAX_LINES];
        float damp_vals[SESH_FDN_MAX_LINES];
        for (int i = 0; i < n; i++) {
            bl32[i] = (uint32_t)buf_lens[i];
            pos32[i] = (uint32_t)positions[i];
            damp_vals[i] = damp[i].y1;
        }
        sesh_vec_comb_parallel_host(
            (const float* const*)bufs, bl32, pos32, damp_vals,
            (const float* const*)dst, src, time,
            feedback, damping, (uint32_t)n, (uint32_t)frames);
        for (int i = 0; i < n; i++) {
            positions[i] = (int)pos32[i];
            damp[i].y1 = damp_vals[i];
        }
        return;
    }
#endif
    for (int line = 0; line < n; line++) {
        sesh_vec_comb(bufs[line], buf_lens[line], &positions[line],
                      &damp[line], dst[line], src, time[line],
                      feedback, damping, frames);
    }
}

/* -----------------------------------------------------------------------
 * Vec DSP ops: coupled comb filter (FDN with N*N mixing matrix)
 * ----------------------------------------------------------------------- */

static inline void sesh_vec_comb_coupled(
    float** bufs, const int* buf_lens, int* positions,
    SeshOnePoleState* damp, float** dst, const float* const* src,
    const float* const* time, const float* matrix, float damping,
    int n, int frames)
{
#ifdef __wasm__
    if (sesh__use_host_vec()) {
        uint32_t bl32[SESH_FDN_MAX_LINES], pos32[SESH_FDN_MAX_LINES];
        float damp_vals[SESH_FDN_MAX_LINES];
        for (int i = 0; i < n; i++) {
            bl32[i] = (uint32_t)buf_lens[i];
            pos32[i] = (uint32_t)positions[i];
            damp_vals[i] = damp[i].y1;
        }
        sesh_vec_comb_coupled_host(
            (const float* const*)bufs, bl32, pos32, damp_vals,
            (const float* const*)dst, src, time, matrix,
            damping, (uint32_t)n, (uint32_t)frames);
        for (int i = 0; i < n; i++) {
            positions[i] = (int)pos32[i];
            damp[i].y1 = damp_vals[i];
        }
        return;
    }
#endif
    for (int i = 0; i < frames; i++) {
        float taps[SESH_FDN_MAX_LINES];
        float mixed[SESH_FDN_MAX_LINES];

        for (int line = 0; line < n; line++) {
            int bl = buf_lens[line];
            int wp = positions[line];
            float t = time[line][i];
            int delay_int = (int)t;
            float delay_frac = t - (float)delay_int;
            int idx1 = (wp + bl - delay_int) % bl;
            int idx2 = (idx1 + bl - 1) % bl;
            taps[line] = bufs[line][idx1] + delay_frac * (bufs[line][idx2] - bufs[line][idx1]);
        }

        for (int line = 0; line < n; line++) {
            damp[line].y1 = taps[line] + damping * (damp[line].y1 - taps[line]);
            taps[line] = damp[line].y1;
        }

        for (int row = 0; row < n; row++) {
            float sum = 0.0f;
            for (int col = 0; col < n; col++) {
                sum += matrix[row * n + col] * taps[col];
            }
            mixed[row] = sum;
        }

        for (int line = 0; line < n; line++) {
            dst[line][i] = taps[line];
            bufs[line][positions[line]] = src[line][i] + mixed[line];
            positions[line] = (positions[line] + 1) % buf_lens[line];
        }
    }
}

/* -----------------------------------------------------------------------
 * Vec DSP ops: oscillator
 * ----------------------------------------------------------------------- */

#ifndef SESH_PI
#define SESH_PI 3.14159265358979323846f
#endif
#ifndef SESH_TAU
#define SESH_TAU (2.0f * SESH_PI)
#endif

static inline void sesh_vec_osc(float* phase, float* dst, float freq,
                                 SeshWaveform waveform, float sample_rate, int frames) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_osc_host(phase, dst, freq, (uint32_t)waveform, sample_rate, (uint32_t)frames); return; }
#endif
    float phase_inc = freq / sample_rate;
    float p = *phase;
    for (int i = 0; i < frames; i++) {
        switch (waveform) {
        case SESH_WAVEFORM_SINE:
            dst[i] = sinf(p * SESH_TAU);
            break;
        case SESH_WAVEFORM_TRIANGLE:
            dst[i] = 4.0f * fabsf(p - floorf(p + 0.5f)) - 1.0f;
            break;
        case SESH_WAVEFORM_SAW:
            dst[i] = 2.0f * (p - floorf(p + 0.5f));
            break;
        case SESH_WAVEFORM_SQUARE:
            dst[i] = fmodf(p, 1.0f) < 0.5f ? 1.0f : -1.0f;
            break;
        }
        p += phase_inc;
        if (p >= 1.0f) p -= 1.0f;
    }
    *phase = p;
}

/* -----------------------------------------------------------------------
 * Vec DSP ops: biquad filter (per-sample modulated coefficients)
 * ----------------------------------------------------------------------- */

static inline void sesh_vec_biquad(SeshBiquadState* state, float* dst, const float* src,
                                    const float* cutoff, const float* q, const float* gain_db,
                                    SeshFilterType filter_type, float sample_rate, int frames) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_biquad_host((float*)state, dst, src, cutoff, q, gain_db, (uint32_t)filter_type, sample_rate, (uint32_t)frames); return; }
#endif
    for (int i = 0; i < frames; i++) {
        float w0 = SESH_TAU * cutoff[i] / sample_rate;
        float cos_w0 = cosf(w0);
        float sin_w0 = sinf(w0);
        float alpha = sin_w0 / (2.0f * q[i]);
        float a_lin = powf(10.0f, gain_db[i] / 40.0f);

        float b0, b1, b2, a0, a1, a2;
        switch (filter_type) {
        case SESH_FILTER_LOWPASS:
            b1 = 1.0f - cos_w0; b0 = b1 / 2.0f; b2 = b0;
            a0 = 1.0f + alpha; a1 = -2.0f * cos_w0; a2 = 1.0f - alpha;
            break;
        case SESH_FILTER_HIGHPASS:
            b1 = -(1.0f + cos_w0); b0 = (1.0f + cos_w0) / 2.0f; b2 = b0;
            a0 = 1.0f + alpha; a1 = -2.0f * cos_w0; a2 = 1.0f - alpha;
            break;
        case SESH_FILTER_BANDPASS:
            b0 = alpha; b1 = 0.0f; b2 = -alpha;
            a0 = 1.0f + alpha; a1 = -2.0f * cos_w0; a2 = 1.0f - alpha;
            break;
        case SESH_FILTER_NOTCH:
            b0 = 1.0f; b1 = -2.0f * cos_w0; b2 = 1.0f;
            a0 = 1.0f + alpha; a1 = -2.0f * cos_w0; a2 = 1.0f - alpha;
            break;
        case SESH_FILTER_PEAK:
            b0 = 1.0f + alpha * a_lin; b1 = -2.0f * cos_w0; b2 = 1.0f - alpha * a_lin;
            a0 = 1.0f + alpha / a_lin; a1 = -2.0f * cos_w0; a2 = 1.0f - alpha / a_lin;
            break;
        case SESH_FILTER_LOWSHELF: {
            float tsa = 2.0f * sqrtf(a_lin) * alpha;
            b0 = a_lin * ((a_lin+1.0f) - (a_lin-1.0f)*cos_w0 + tsa);
            b1 = 2.0f * a_lin * ((a_lin-1.0f) - (a_lin+1.0f)*cos_w0);
            b2 = a_lin * ((a_lin+1.0f) - (a_lin-1.0f)*cos_w0 - tsa);
            a0 = (a_lin+1.0f) + (a_lin-1.0f)*cos_w0 + tsa;
            a1 = -2.0f * ((a_lin-1.0f) + (a_lin+1.0f)*cos_w0);
            a2 = (a_lin+1.0f) + (a_lin-1.0f)*cos_w0 - tsa;
            break;
        }
        case SESH_FILTER_HIGHSHELF: {
            float tsa = 2.0f * sqrtf(a_lin) * alpha;
            b0 = a_lin * ((a_lin+1.0f) + (a_lin-1.0f)*cos_w0 + tsa);
            b1 = -2.0f * a_lin * ((a_lin-1.0f) + (a_lin+1.0f)*cos_w0);
            b2 = a_lin * ((a_lin+1.0f) + (a_lin-1.0f)*cos_w0 - tsa);
            a0 = (a_lin+1.0f) - (a_lin-1.0f)*cos_w0 + tsa;
            a1 = 2.0f * ((a_lin-1.0f) - (a_lin+1.0f)*cos_w0);
            a2 = (a_lin+1.0f) - (a_lin-1.0f)*cos_w0 - tsa;
            break;
        }
        case SESH_FILTER_ALLPASS:
            b0 = 1.0f - alpha; b1 = -2.0f * cos_w0; b2 = 1.0f + alpha;
            a0 = 1.0f + alpha; a1 = -2.0f * cos_w0; a2 = 1.0f - alpha;
            break;
        default:
            b0 = 1.0f; b1 = 0.0f; b2 = 0.0f;
            a0 = 1.0f; a1 = 0.0f; a2 = 0.0f;
            break;
        }

        b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;

        float x0 = src[i];
        float y0 = b0*x0 + b1*state->x1 + b2*state->x2 - a1*state->y1 - a2*state->y2;
        state->x2 = state->x1; state->x1 = x0;
        state->y2 = state->y1; state->y1 = y0;
        dst[i] = y0;
    }
}

/* -----------------------------------------------------------------------
 * Vec DSP ops: envelope follower
 * ----------------------------------------------------------------------- */

static inline void sesh_vec_envelope(SeshEnvelopeState* state, float* dst, const float* src,
                                      const float* attack, const float* release,
                                      SeshEnvelopeMode mode, float sample_rate, int frames) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_envelope_host((float*)state, dst, src, attack, release, (uint32_t)mode, sample_rate, (uint32_t)frames); return; }
#endif
    for (int i = 0; i < frames; i++) {
        float input_level = (mode == SESH_ENVELOPE_RMS) ? src[i] * src[i] : fabsf(src[i]);
        float att_coeff = expf(-1.0f / (attack[i] * sample_rate));
        float rel_coeff = expf(-1.0f / (release[i] * sample_rate));
        float coeff = (input_level > state->current) ? att_coeff : rel_coeff;
        state->current = coeff * state->current + (1.0f - coeff) * input_level;
        dst[i] = (mode == SESH_ENVELOPE_RMS) ? sqrtf(state->current) : state->current;
    }
}

/* -----------------------------------------------------------------------
 * Vec DSP ops: waveshaping
 * ----------------------------------------------------------------------- */

static inline void sesh_vec_tanh(float* dst, const float* src, const float* drive, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_tanh_host(dst, src, drive, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = tanhf(src[i] * drive[i]);
}

static inline void sesh_vec_hard_clip(float* dst, const float* src, const float* threshold, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_hard_clip_host(dst, src, threshold, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) {
        float t = threshold[i];
        float v = src[i];
        dst[i] = v < -t ? -t : (v > t ? t : v);
    }
}

/* -----------------------------------------------------------------------
 * Vec DSP ops: unary math
 * ----------------------------------------------------------------------- */

static inline void sesh_vec_abs(float* dst, const float* src, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_abs_host(dst, src, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = fabsf(src[i]);
}

static inline void sesh_vec_neg(float* dst, const float* src, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_neg_host(dst, src, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = -src[i];
}

static inline void sesh_vec_sqrt(float* dst, const float* src, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_sqrt_host(dst, src, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = sqrtf(src[i]);
}

static inline void sesh_vec_recip(float* dst, const float* src, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_recip_host(dst, src, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = 1.0f / src[i];
}

static inline void sesh_vec_div(float* dst, const float* a, const float* b, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_div_host(dst, a, b, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = a[i] / b[i];
}

static inline void sesh_vec_pow(float* dst, const float* src, const float* e, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_pow_host(dst, src, e, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = powf(src[i], e[i]);
}

static inline void sesh_vec_log(float* dst, const float* src, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_log_host(dst, src, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = logf(src[i]);
}

static inline void sesh_vec_exp(float* dst, const float* src, int len) {
#ifdef __wasm__
    if (sesh__use_host_vec()) { sesh_vec_exp_host(dst, src, (uint32_t)len); return; }
#endif
    for (int i = 0; i < len; i++) dst[i] = expf(src[i]);
}

/* -----------------------------------------------------------------------
 * Scratch pool helpers
 * ----------------------------------------------------------------------- */

static inline void sesh_scratch_init(SeshScratch* s) {
    s->data = (float*)sesh_calloc(SESH_SCRATCH_BUFS * SESH_SCRATCH_FRAMES * sizeof(float));
    s->cursor = 0;
}

static inline void sesh_scratch_free(SeshScratch* s) {
    if (s->data) {
        free(s->data);
        s->data = 0;
    }
}

/* -----------------------------------------------------------------------
 * WASI stubs (WASM only)
 *
 * musl's abort path pulls in fd_close/fd_seek/fd_write and the threads
 * runtime pulls in sched_yield. These are never called in a plugin that
 * doesn't do I/O. Providing stubs eliminates all WASI imports.
 * ----------------------------------------------------------------------- */

#ifdef __wasm__
#ifdef __cplusplus
extern "C" {
#endif
int __imported_wasi_snapshot_preview1_fd_close(int fd) { (void)fd; return 8; }
int __imported_wasi_snapshot_preview1_sched_yield(void) { return 0; }
int __imported_wasi_snapshot_preview1_fd_seek(int fd, long long offset, int whence, long long* newoffset) {
    (void)fd; (void)offset; (void)whence; (void)newoffset; return 8;
}
int __imported_wasi_snapshot_preview1_fd_write(int fd, const void* iovs, int iovs_len, int* nwritten) {
    (void)fd; (void)iovs; (void)iovs_len; (void)nwritten; return 8;
}
#ifdef __cplusplus
}
#endif
#endif /* __wasm__ */

#endif /* SESH_H */
