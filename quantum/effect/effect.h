#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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

typedef struct effect_runtime{
    struct effect_runtime* next;
    effect_frame_t* frames;
    uint32_t time_left_in_frame;
    uint32_t current_frame;
    uint32_t num_frames;
    void* user_data;
    uint16_t loop;
} effect_runtime_t;

#define EFFECT_NO_LOOP 0
#define EFFECT_LOOP_INFINITE 0xFFFF


void add_effect(effect_runtime_t* runtime, effect_frame_t* frames, unsigned int frames_size, void* userdata,
    uint16_t loops);
void remove_effect(effect_runtime_t* runtime);
void clear_all_effects(void);
void update_effects(unsigned int dt);
