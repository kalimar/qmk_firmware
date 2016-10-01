#pragma once

typedef struct {
    unsigned int duration;
    unsigned int current_frame_time;
} effect_param_t;

typedef void (*effect_update_func_t)(const effect_param_t* param);

typedef struct {
    unsigned int duration; // The duration of the effect in milliseconds
    effect_update_func_t update; // The update function
} effect_t;

void add_effect(effect_t* effect, unsigned int effect_size);
void update_effects(unsigned int dt);
