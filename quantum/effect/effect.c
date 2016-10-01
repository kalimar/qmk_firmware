#include "effect.h"
#include <stdint.h>

typedef struct {
    effect_t* frames;
    int32_t time_left_in_frame;
} active_effect_t;

#define MAX_ACTIVE_EFFECTS 10

active_effect_t active_effects[MAX_ACTIVE_EFFECTS];

void add_effect(effect_t* effect, unsigned int effect_size) {
    active_effect_t* new_effect = &active_effects[0];
    new_effect->frames = effect;
    new_effect->time_left_in_frame = effect->duration;
}

void update_effects(unsigned int dt) {
    if (dt == 0) {
        return;
    }
    active_effect_t* effect = &active_effects[0];
    if (effect->time_left_in_frame > 0) {
        effect->time_left_in_frame -= dt;
        effect_param_t param;
        param.duration = effect->frames[0].duration;
        unsigned real_left_in_frame = effect->time_left_in_frame > 0 ? effect->time_left_in_frame : 0;
        param.current_frame_time = param.duration - real_left_in_frame;
        effect->frames->update(&param);
    }
}
