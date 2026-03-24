#include "sesh.h"
#include <cmath>

static const unsigned char backdrop_data[] = {
    #embed "assets/backdrop.png"
};
static const unsigned char strip_data[] = {
    #embed "assets/strip_0.png"
};

static constexpr float SR = 44100.0f;
static constexpr float BASE_DELAYS[4] = {0.008f, 0.012f, 0.018f, 0.025f};
static constexpr float LFO_DETUNE[4] = {0.88f, 1.07f, 0.92f, 1.13f};
static constexpr float PAN_L[4] = {0.7f, 0.3f, 0.65f, 0.35f};
static constexpr float PAN_R[4] = {0.3f, 0.7f, 0.35f, 0.65f};
static constexpr float FEEDBACK = 0.08f;
static constexpr int MAX_DELAY_SAMPLES = (int)((0.025 + 0.012 + 0.005) * SR) + 4;

class LiquidSunday : public sesh::Plugin {
public:
    SeshScratch scratch{};
    float delay_bufs[2][4][MAX_DELAY_SAMPLES]{};
    int delay_pos[2][4]{};
    float lfo_phases[4] = {0.0f, 0.25f, 0.5f, 0.75f};
    SeshBiquadState hp_filters[2]{};
    SeshBiquadState lp_filters[2]{};
    SeshImageId bg_id = 0;
    SeshImageId strip_id = 0;

    LiquidSunday() {
        sesh_scratch_init(&scratch);
        draw_width = 366.0f;
        params = {
            {"Rate",   0.05f, 2.00f, 0.20f},
            {"Depth",  0.00f, 1.00f, 0.60f},
            {"Voices", 1.00f, 4.00f, 3.00f},
            {"Tone",   800.0f, 18000.0f, 6000.0f},
            {"Mix",    0.00f, 1.00f, 0.50f},
        };
    }

    ~LiquidSunday() {
        sesh_scratch_free(&scratch);
    }

    void process(sesh::ProcessContext& ctx) override {
        int frames = ctx.frames;
        int num_ch = ctx.num_channels < 2 ? ctx.num_channels : 2;

        float rate       = ctx.params[0][0];
        float depth      = ctx.params[1][0];
        float voices_param = ctx.params[2][0];
        float tone_val   = ctx.params[3][0];
        float mix        = ctx.params[4][0];

        float depth_sec = 0.003f + depth * 0.009f;

        float* hp_out    = sesh_scratch_buf(&scratch, frames);
        float* wet_l     = sesh_scratch_buf(&scratch, frames);
        float* wet_r     = sesh_scratch_buf(&scratch, frames);
        float* tone_buf  = sesh_scratch_buf(&scratch, frames);
        float* q_buf     = sesh_scratch_buf(&scratch, frames);
        float* gain_buf  = sesh_scratch_buf(&scratch, frames);
        float* hp_freq   = sesh_scratch_buf(&scratch, frames);

        sesh_vec_fill(tone_buf, tone_val, frames);
        sesh_vec_fill(q_buf, 0.707f, frames);
        sesh_vec_fill(gain_buf, 0.0f, frames);
        sesh_vec_fill(hp_freq, 80.0f, frames);
        sesh_vec_fill(wet_l, 0.0f, frames);
        sesh_vec_fill(wet_r, 0.0f, frames);

        for (int ch = 0; ch < num_ch; ch++) {
            sesh_vec_biquad(&hp_filters[ch], hp_out, ctx.channels[ch],
                            hp_freq, q_buf, gain_buf,
                            SESH_FILTER_HIGHPASS, SR, frames);

            for (int v = 0; v < 4; v++) {
                float voice_threshold = (float)(v + 1);
                float voice_gain;
                if (voices_param >= voice_threshold) voice_gain = 1.0f;
                else if (voices_param > voice_threshold - 1.0f) voice_gain = voices_param - (voice_threshold - 1.0f);
                else continue;

                float* lfo_buf = sesh_scratch_buf(&scratch, frames);
                if (ch == 0) {
                    sesh_vec_osc(&lfo_phases[v], lfo_buf, rate * LFO_DETUNE[v],
                                 SESH_WAVEFORM_SINE, SR, frames);
                } else {
                    float temp_phase = lfo_phases[v] - (rate * LFO_DETUNE[v] * (float)frames / SR);
                    while (temp_phase < 0.0f) temp_phase += 1.0f;
                    sesh_vec_osc(&temp_phase, lfo_buf, rate * LFO_DETUNE[v],
                                 SESH_WAVEFORM_SINE, SR, frames);
                }

                float* time_buf = sesh_scratch_buf(&scratch, frames);
                float base_samples = BASE_DELAYS[v] * SR;
                sesh_vec_copy(time_buf, lfo_buf, frames);
                sesh_vec_mul_scalar(time_buf, depth_sec * SR, frames);
                sesh_vec_add_scalar(time_buf, base_samples, frames);

                float* voice_out = sesh_scratch_buf(&scratch, frames);
                sesh_vec_delay_read(delay_bufs[ch][v], MAX_DELAY_SAMPLES,
                                    delay_pos[ch][v], voice_out, time_buf, frames);

                float* write_buf = sesh_scratch_buf(&scratch, frames);
                sesh_vec_copy(write_buf, voice_out, frames);
                float* drive_buf = sesh_scratch_buf(&scratch, frames);
                sesh_vec_fill(drive_buf, 1.0f, frames);
                sesh_vec_tanh(write_buf, write_buf, drive_buf, frames);
                sesh_vec_mul_scalar(write_buf, FEEDBACK, frames);
                sesh_vec_add(write_buf, write_buf, hp_out, frames);

                sesh_vec_ring_write(delay_bufs[ch][v], MAX_DELAY_SAMPLES,
                                    &delay_pos[ch][v], write_buf, frames);

                float pan_l = PAN_L[v] * voice_gain;
                float pan_r = PAN_R[v] * voice_gain;
                if (ch == 0) {
                    sesh_vec_mul_add(wet_l, voice_out, pan_l, frames);
                    sesh_vec_mul_add(wet_r, voice_out, pan_r, frames);
                } else {
                    sesh_vec_mul_add(wet_l, voice_out, pan_r, frames);
                    sesh_vec_mul_add(wet_r, voice_out, pan_l, frames);
                }
            }
        }

        float norm = 1.0f / (float)num_ch;
        sesh_vec_mul_scalar(wet_l, norm, frames);
        sesh_vec_mul_scalar(wet_r, norm, frames);

        float* wet_l_filt = sesh_scratch_buf(&scratch, frames);
        sesh_vec_biquad(&lp_filters[0], wet_l_filt, wet_l,
                        tone_buf, q_buf, gain_buf,
                        SESH_FILTER_LOWPASS, SR, frames);

        if (num_ch > 0) {
            sesh_vec_mul_scalar(ctx.channels[0], 1.0f - mix, frames);
            sesh_vec_mul_add(ctx.channels[0], wet_l_filt, mix, frames);
        }

        float* wet_r_filt = sesh_scratch_buf(&scratch, frames);
        sesh_vec_biquad(&lp_filters[1], wet_r_filt, wet_r,
                        tone_buf, q_buf, gain_buf,
                        SESH_FILTER_LOWPASS, SR, frames);

        if (num_ch > 1) {
            sesh_vec_mul_scalar(ctx.channels[1], 1.0f - mix, frames);
            sesh_vec_mul_add(ctx.channels[1], wet_r_filt, mix, frames);
        }
    }

    void draw(sesh::DrawContext& ctx) override {
        if (!bg_id) bg_id = sesh_load_image(backdrop_data, sizeof(backdrop_data));
        if (!strip_id) strip_id = sesh_load_image(strip_data, sizeof(strip_data));

        sesh_draw_bg_image(bg_id);

        // Depth (param 1)
        sesh_draw_knob_strip(1, strip_id, 27.0f, 58.0f, 99.5f, 99.5f, 82, 47.0f, 79.0f, 60.0f, 59.5f);
        // Tone (param 3)
        sesh_draw_knob_strip(3, strip_id, 81.0f, 121.5f, 101.5f, 101.5f, 82, 101.5f, 142.5f, 61.0f, 60.5f);
        // Rate (param 0)
        sesh_draw_knob_strip(0, strip_id, 113.0f, 16.0f, 137.5f, 137.5f, 82, 140.5f, 44.5f, 82.5f, 82.0f);
        // Mix (param 4)
        sesh_draw_knob_strip(4, strip_id, 181.0f, 122.0f, 101.0f, 101.0f, 82, 201.5f, 143.0f, 60.5f, 60.5f);
        // Voices (param 2)
        sesh_draw_knob_strip(2, strip_id, 237.0f, 58.0f, 99.5f, 99.5f, 82, 257.0f, 79.0f, 60.0f, 59.5f);
    }
};

SESH_PLUGIN_CPP(LiquidSunday)
