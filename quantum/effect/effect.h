#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define EFFECT_MAX_USERDATA_SIZE 32
#define EFFECT_MAX_SLOTS 8

typedef struct {
    uint32_t duration;
    uint32_t current_frame_time;
    uint32_t current_frame_nr;
    void* user_data;
    bool entry;
    bool exit;
} effect_param_t;

typedef void (*effect_update_func_t)(const effect_param_t* param);

typedef struct {
    uint32_t duration; // The duration of the effect in milliseconds
    effect_update_func_t update; // The update function
} effect_frame_t;

#define EFFECT_NO_LOOP 1
#define EFFECT_LOOP_INFINITE 0xFFFF


void add_effect(unsigned slot, effect_frame_t* frames, unsigned int frames_size, void* userdata,
    unsigned user_data_size, uint16_t loops);
void remove_effect(unsigned slot);
void clear_all_effects(void);
void update_effects(uint16_t abstime);
