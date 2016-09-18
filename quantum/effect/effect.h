#pragma once

typedef void (*effect_update_func_t)();

typedef struct {
    unsigned int duration; // The duration of the effect in milliseconds
    effect_update_func_t update; // The update function
} effect_t;

void add_effect(effect_t* effect, unsigned int effect_size);
void update_effects(unsigned int dt);
