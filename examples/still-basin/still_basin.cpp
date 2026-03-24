#include "sesh.h"

#include <cmath>
#include <cstring>
#include <array>

static constexpr float SR = 44100.0f;
static constexpr int NUM_LINES = 16;
static constexpr int NUM_AP_PER_LINE = 3;
static constexpr int PRE_DELAY_MAX = (int)(0.15 * SR) + 1;

static const int NUM_INPUT_AP = 4;
static const int INPUT_AP_DELAYS[NUM_INPUT_AP] = {142, 107, 79, 53};
static const float INPUT_AP_G = 0.6f;

static const int LINE_DELAYS[NUM_LINES] = {
    1327, 1559, 1747, 1951, 2137, 2351, 2543, 2741,
    2953, 3163, 3371, 3547, 3761, 3967, 4177, 4397,
};

static const int AP_DELAYS[NUM_LINES][NUM_AP_PER_LINE] = {
    {149, 83, 41}, {151, 89, 43}, {139, 79, 37}, {157, 97, 47},
    {143, 83, 41}, {163, 101, 53}, {137, 79, 43}, {167, 103, 47},
    {149, 89, 41}, {151, 97, 53}, {139, 83, 37}, {157, 101, 43},
    {143, 89, 47}, {163, 97, 41}, {137, 103, 53}, {167, 83, 37},
};
static const float AP_G = 0.6f;

static const float LFO_RATES[NUM_LINES] = {
    0.31f, 0.43f, 0.57f, 0.37f, 0.71f, 0.53f, 0.83f, 0.47f,
    0.61f, 0.97f, 0.41f, 1.13f, 0.67f, 0.89f, 1.07f, 0.73f,
};
static const float LFO_DEPTH = 12.0f;

static const int GRAIN_SIZE = (int)(0.040 * SR);

struct Allpass {
    std::vector<float> buf;
    int pos = 0;

    Allpass() = default;
    Allpass(int delay) : buf(delay, 0.0f), pos(0) {}

    float process(float input, float g) {
        float delayed = buf[pos];
        float v = input - g * delayed;
        buf[pos] = v;
        if (++pos >= (int)buf.size()) pos = 0;
        return delayed + g * v;
    }
};

struct DelayLine {
    std::vector<float> buf;
    int pos = 0;
    int length = 0;

    DelayLine() = default;
    DelayLine(int len) : buf(len + 32, 0.0f), pos(0), length(len) {}

    void write(float sample) {
        buf[pos] = sample;
        if (++pos >= (int)buf.size()) pos = 0;
    }

    float read_cubic(float delay_samples) const {
        int bl = (int)buf.size();
        int d_int = (int)delay_samples;
        float frac = delay_samples - (float)d_int;

        int idx = (pos + bl - d_int - 1) % bl;
        float s0 = buf[(idx + bl - 1) % bl];
        float s1 = buf[idx];
        float s2 = buf[(idx + 1) % bl];
        float s3 = buf[(idx + 2) % bl];

        float c0 = s1;
        float c1 = 0.5f * (s2 - s0);
        float c2 = s0 - 2.5f * s1 + 2.0f * s2 - 0.5f * s3;
        float c3 = 0.5f * (s3 - s0) + 1.5f * (s1 - s2);
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }
};

struct PitchShifter {
    std::vector<float> buf;
    int write_pos = 0;
    float grain_phase = 0.0f;

    PitchShifter() : buf(GRAIN_SIZE * 4, 0.0f) {}

    float process(float input) {
        int bl = (int)buf.size();
        buf[write_pos] = input;
        write_pos = (write_pos + 1) % bl;

        float grain_size_f = (float)GRAIN_SIZE;
        float phase = grain_phase;
        float g2_phase = fmodf(phase + 0.5f, 1.0f);

        float w1 = 0.5f * (1.0f - cosf(2.0f * SESH_PI * phase));
        float w2 = 0.5f * (1.0f - cosf(2.0f * SESH_PI * g2_phase));

        int g1_pos = (int)(phase * grain_size_f);
        int g2_pos = (int)(g2_phase * grain_size_f);
        int read1 = (write_pos + bl - GRAIN_SIZE - (g1_pos * 2) % bl) % bl;
        int read2 = (write_pos + bl - GRAIN_SIZE - (g2_pos * 2) % bl) % bl;

        float out = buf[read1] * w1 + buf[read2] * w2;

        grain_phase += 1.0f / grain_size_f;
        if (grain_phase >= 1.0f) grain_phase -= 1.0f;
        return out;
    }
};

static void build_hadamard(float h[NUM_LINES][NUM_LINES]) {
    memset(h, 0, NUM_LINES * NUM_LINES * sizeof(float));
    h[0][0] = 1.0f;
    int size = 1;
    while (size < NUM_LINES) {
        for (int i = 0; i < size; i++)
            for (int j = 0; j < size; j++) {
                float v = h[i][j];
                h[i][j + size] = v;
                h[i + size][j] = v;
                h[i + size][j + size] = -v;
            }
        size *= 2;
    }
    float norm = 1.0f / sqrtf((float)NUM_LINES);
    for (int i = 0; i < NUM_LINES; i++)
        for (int j = 0; j < NUM_LINES; j++)
            h[i][j] *= norm;
}

class StillBasin : public sesh::Plugin {
public:
    SeshScratch scratch{};

    float pre_delay_buf[2][PRE_DELAY_MAX]{};
    int pre_delay_pos[2]{};

    Allpass input_aps[2][NUM_INPUT_AP];
    DelayLine fdn_lines[2][NUM_LINES];
    Allpass fdn_aps[2][NUM_LINES][NUM_AP_PER_LINE];
    float damp_state[2][NUM_LINES]{};
    float lfo_phases[NUM_LINES]{};
    float hadamard[NUM_LINES][NUM_LINES]{};
    PitchShifter pitch_shifter[2];
    float dc_x_prev[2]{};
    float dc_y_prev[2]{};
    SeshBiquadState hp_state[2]{};

    StillBasin() {
        sesh_scratch_init(&scratch);

        params = {
            {"Decay",   0.10f, 30.00f,    4.00f},
            {"Shimmer", 0.00f,  1.00f,    0.20f},
            {"Tone",    2000.0f, 16000.0f, 9000.0f},
            {"Width",   0.00f,  1.00f,    0.75f},
            {"Mix",     0.00f,  1.00f,    0.40f},
        };

        for (int ch = 0; ch < 2; ch++) {
            for (int i = 0; i < NUM_INPUT_AP; i++)
                input_aps[ch][i] = Allpass(INPUT_AP_DELAYS[i]);
            for (int i = 0; i < NUM_LINES; i++) {
                fdn_lines[ch][i] = DelayLine(LINE_DELAYS[i]);
                for (int j = 0; j < NUM_AP_PER_LINE; j++)
                    fdn_aps[ch][i][j] = Allpass(AP_DELAYS[i][j]);
            }
        }

        build_hadamard(hadamard);
    }

    ~StillBasin() {
        sesh_scratch_free(&scratch);
    }

    void process(sesh::ProcessContext& ctx) override {
        int frames = ctx.frames;
        if (frames == 0) return;

        float decay    = ctx.params[0][0];
        float shimmer  = ctx.params[1][0];
        float tone     = ctx.params[2][0];
        float width    = ctx.params[3][0];
        float mix_val  = ctx.params[4][0];

        float avg_delay_sec = 0.0f;
        for (int i = 0; i < NUM_LINES; i++) avg_delay_sec += (float)LINE_DELAYS[i];
        avg_delay_sec /= (float)NUM_LINES * SR;

        float feedback_gain = (decay > 0.1f)
            ? fminf(powf(10.0f, -3.0f * avg_delay_sec / decay), 0.998f)
            : 0.0f;

        float damp_coeff = expf(-2.0f * SESH_PI * tone / SR);
        float stereo_offset = width * 7.0f;
        int pre_delay_samples = (int)(0.02f * SR);
        int num_ch = ctx.num_channels < 2 ? ctx.num_channels : 2;

        float* dry_l = sesh_scratch_buf(&scratch, frames);
        float* dry_r = sesh_scratch_buf(&scratch, frames);
        float* wet_l = sesh_scratch_buf(&scratch, frames);
        float* wet_r = sesh_scratch_buf(&scratch, frames);

        sesh_vec_copy(dry_l, ctx.channels[0], frames);
        if (num_ch > 1)
            sesh_vec_copy(dry_r, ctx.channels[1], frames);
        else
            sesh_vec_copy(dry_r, ctx.channels[0], frames);

        for (int n = 0; n < frames; n++) {
            for (int i = 0; i < NUM_LINES; i++) {
                lfo_phases[i] += LFO_RATES[i] / SR;
                if (lfo_phases[i] >= 1.0f) lfo_phases[i] -= 1.0f;
            }

            for (int ch = 0; ch < num_ch; ch++) {
                float input_sample = (ch == 0) ? dry_l[n] : dry_r[n];

                // Pre-delay
                pre_delay_buf[ch][pre_delay_pos[ch]] = input_sample;
                int pd_read = (pre_delay_pos[ch] + PRE_DELAY_MAX - pre_delay_samples) % PRE_DELAY_MAX;
                float pre_delayed = pre_delay_buf[ch][pd_read];
                pre_delay_pos[ch] = (pre_delay_pos[ch] + 1) % PRE_DELAY_MAX;

                // Input diffusion
                float sig = pre_delayed;
                for (int i = 0; i < NUM_INPUT_AP; i++)
                    sig = input_aps[ch][i].process(sig, INPUT_AP_G);
                float fdn_input = sig;

                // Read FDN lines with modulation
                float line_outputs[NUM_LINES];
                for (int i = 0; i < NUM_LINES; i++) {
                    float lfo_val = sinf(2.0f * SESH_PI * lfo_phases[i]);
                    float delay_f = (float)LINE_DELAYS[i] + lfo_val * LFO_DEPTH;
                    if (ch == 1) delay_f += stereo_offset;
                    if (delay_f < 1.0f) delay_f = 1.0f;
                    line_outputs[i] = fdn_lines[ch][i].read_cubic(delay_f);
                }

                // Hadamard mixing
                float mixed[NUM_LINES];
                for (int i = 0; i < NUM_LINES; i++) {
                    float sum = 0.0f;
                    for (int j = 0; j < NUM_LINES; j++)
                        sum += hadamard[i][j] * line_outputs[j];
                    mixed[i] = sum;
                }

                // Shimmer
                float fdn_sum = 0.0f;
                for (int i = 0; i < NUM_LINES; i++) fdn_sum += line_outputs[i];
                fdn_sum /= sqrtf((float)NUM_LINES);
                float shimmer_sig = pitch_shifter[ch].process(fdn_sum);

                // Write back with feedback
                for (int i = 0; i < NUM_LINES; i++) {
                    float s = mixed[i] * feedback_gain;
                    s += shimmer_sig * shimmer * 0.3f;

                    damp_state[ch][i] = damp_state[ch][i] * damp_coeff
                        + s * (1.0f - damp_coeff);
                    s = damp_state[ch][i];

                    for (int j = 0; j < NUM_AP_PER_LINE; j++)
                        s = fdn_aps[ch][i][j].process(s, AP_G);

                    s = tanhf(s);
                    fdn_lines[ch][i].write(s + fdn_input / sqrtf((float)NUM_LINES));
                }

                // Sum wet
                float wet_sum = 0.0f;
                for (int i = 0; i < NUM_LINES; i++) wet_sum += line_outputs[i];
                wet_sum /= sqrtf((float)NUM_LINES);

                // DC blocker
                float dc_out = wet_sum - dc_x_prev[ch] + 0.9995f * dc_y_prev[ch];
                dc_x_prev[ch] = wet_sum;
                dc_y_prev[ch] = dc_out;

                if (ch == 0) wet_l[n] = dc_out;
                else         wet_r[n] = dc_out;
            }
        }

        // Highpass at 80Hz
        float* hp_cutoff = sesh_scratch_buf(&scratch, frames);
        float* hp_q      = sesh_scratch_buf(&scratch, frames);
        float* hp_gain   = sesh_scratch_buf(&scratch, frames);
        sesh_vec_fill(hp_cutoff, 80.0f, frames);
        sesh_vec_fill(hp_q, 0.707f, frames);
        sesh_vec_fill(hp_gain, 0.0f, frames);

        float* hp_out_l = sesh_scratch_buf(&scratch, frames);
        sesh_vec_biquad(&hp_state[0], hp_out_l, wet_l, hp_cutoff, hp_q, hp_gain,
                        SESH_FILTER_HIGHPASS, SR, frames);

        float* hp_out_r = sesh_scratch_buf(&scratch, frames);
        if (num_ch > 1)
            sesh_vec_biquad(&hp_state[1], hp_out_r, wet_r, hp_cutoff, hp_q, hp_gain,
                            SESH_FILTER_HIGHPASS, SR, frames);
        else
            sesh_vec_copy(hp_out_r, hp_out_l, frames);

        // Mix
        float dry_gain = cosf(mix_val * SESH_PI / 2.0f);
        float wet_gain = sinf(mix_val * SESH_PI / 2.0f);

        sesh_vec_mul_scalar(ctx.channels[0], 0.0f, frames);
        sesh_vec_mul_add(ctx.channels[0], dry_l, dry_gain, frames);
        sesh_vec_mul_add(ctx.channels[0], hp_out_l, wet_gain, frames);

        if (num_ch > 1) {
            sesh_vec_mul_scalar(ctx.channels[1], 0.0f, frames);
            sesh_vec_mul_add(ctx.channels[1], dry_r, dry_gain, frames);
            sesh_vec_mul_add(ctx.channels[1], hp_out_r, wet_gain, frames);
        }
    }
};

SESH_PLUGIN_CPP(StillBasin)
