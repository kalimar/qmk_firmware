#include "effect.h"

static effect_runtime_t* active_effects = NULL;

void add_effect(effect_runtime_t* runtime, effect_frame_t* frames, unsigned int frames_size, void* userdata) {
    runtime->frames = frames;
    runtime->time_left_in_frame = frames->duration;
    runtime->num_frames = frames_size / sizeof(effect_frame_t);
    runtime->current_frame = 0;
    runtime->user_data = userdata;
    active_effects = runtime;
}

void update_effects(unsigned int dt) {
    effect_runtime_t* effect = active_effects;
    while (dt > 0 && effect->current_frame < effect->num_frames) {
        effect_frame_t* frame = &effect->frames[effect->current_frame];
        unsigned update_time = dt <= effect->time_left_in_frame ? dt : effect->time_left_in_frame;
        dt -= update_time;
        effect->time_left_in_frame -= update_time;
        effect_param_t param;
        param.duration = frame->duration;
        param.current_frame_time = param.duration - effect->time_left_in_frame;
        param.current_frame_nr = effect->current_frame;
        param.user_data = effect->user_data;
        frame->update(&param);

        if (effect->time_left_in_frame == 0) {
            effect->current_frame++;
            if (effect->current_frame < effect->num_frames) {
                effect->time_left_in_frame = effect->frames[effect->current_frame].duration;
            }
        }
    }
}
