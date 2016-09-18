#include "effect.h"

typedef struct {
    effect_t* effect;
} active_effect_t;

#define MAX_ACTIVE_EFFECTS 10

active_effect_t active_effects[MAX_ACTIVE_EFFECTS];

void add_effect(effect_t* effect, unsigned int effect_size) {
    active_effects[0].effect = effect;
}

void update_effects(unsigned int dt) {
    active_effects[0].effect->update();
}
