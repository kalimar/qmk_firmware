#include "effect.h"

static effect_runtime_t* active_effects = NULL;

void add_effect(effect_runtime_t* runtime, effect_frame_t* frames, unsigned int frames_size) {
    runtime->frames = frames;
    runtime->time_left_in_frame = frames->duration;
    active_effects = runtime;
}

void update_effects(unsigned int dt) {
    if (dt == 0) {
        return;
    }
    effect_runtime_t* effect = active_effects;
    if (effect->time_left_in_frame > 0) {
        effect->time_left_in_frame -= dt;
        effect_param_t param;
        param.duration = effect->frames[0].duration;
        unsigned real_left_in_frame = effect->time_left_in_frame > 0 ? effect->time_left_in_frame : 0;
        param.current_frame_time = param.duration - real_left_in_frame;
        effect->frames->update(&param);
    }
}
