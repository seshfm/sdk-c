#include "sesh.h"

class SimpleGain : public sesh::Plugin {
public:
    SimpleGain() {
        params = {
            {"Gain", 0.0f, 1.0f, 0.5f},
        };
    }

    void process(sesh::ProcessContext& ctx) override {
        for (int ch = 0; ch < ctx.num_channels; ch++)
            sesh_vec_mul(ctx.channels[ch], ctx.channels[ch], ctx.params[0], ctx.frames);
    }
};

SESH_PLUGIN_CPP(SimpleGain)
