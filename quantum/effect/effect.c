#include "effect.h"
#include <memory.h>

static uint16_t last_time = 0;
static uint32_t current_time = 0;

#if EFFECT_MAX_SLOTS > 32
// The active_effects bitmask would need to be extended beyond 32 bits to support more
#error "Only 32 effect slots supported at the moment"
#endif

typedef struct effect_runtime{
    uint32_t start_time;
    uint32_t num_frames;
    uint32_t length;
    effect_frame_t* frames;
    uint16_t loop;
    uint8_t userdata[EFFECT_MAX_USERDATA_SIZE];
} effect_runtime_t;


uint32_t active_effects = 0;
static effect_runtime_t runtimes[EFFECT_MAX_SLOTS];

static uint32_t calculate_effect_length(effect_runtime_t* effect) {
    uint32_t effect_length = 0;
    for (int i=0; i<effect->num_frames; i++) {
        effect_length += effect->frames[i].duration;
    }
    return effect_length;
}

void add_effect(unsigned slot, effect_frame_t* frames, unsigned int frames_size, void* userdata,
    unsigned userdata_size, uint16_t loops) {

    if (userdata_size > EFFECT_MAX_USERDATA_SIZE) {
        return;
    }
    if (slot >= EFFECT_MAX_SLOTS) {
        return;
    }
    effect_runtime_t* runtime = &runtimes[slot];
    runtime->frames = frames;
    runtime->start_time = current_time;
    runtime->num_frames = frames_size / sizeof(effect_frame_t);
    memcpy(runtime->userdata,  userdata, userdata_size);
    runtime->loop = loops;
    runtime->length = calculate_effect_length(runtime);
    active_effects |= 1u << slot;
}

void remove_effect(unsigned slot) {
    if (slot >= EFFECT_MAX_SLOTS) {
        return;
    }
    active_effects &= ~(1u << slot);
}

void clear_all_effects(void) {
    active_effects = 0;
}

static void seek(effect_runtime_t* effect, uint32_t time, unsigned* frame, uint32_t* time_left_in_frame) {
    unsigned current_frame = 0;
    while (time >= effect->frames[current_frame].duration) {
        time -= effect->frames[current_frame].duration;
        // Don't move over zero duration frames
        if (time == 0 && effect->frames[current_frame].duration == 0) {
            break;
        }
        current_frame++;
    }
    *frame = current_frame;
    *time_left_in_frame = effect->frames[current_frame].duration - time;
}

static bool update_effect(uint32_t prev_time, unsigned int dt, effect_runtime_t* effect) {
    const uint32_t time_since_effect_start = prev_time - effect->start_time;
    const uint32_t current_effect_time = time_since_effect_start % effect->length;
    uint32_t num_loops = time_since_effect_start  / effect->length;

    unsigned current_frame;
    uint32_t time_left_in_frame;
    seek(effect, current_effect_time, &current_frame, &time_left_in_frame);

    while (dt > 0 || time_left_in_frame == 0) {
        effect_frame_t* frame = &effect->frames[current_frame];
        const unsigned update_time = dt <= time_left_in_frame ? dt : time_left_in_frame;
        dt -= update_time;
        effect_param_t param;
        param.entry = false;
        param.exit = false;
        if (time_left_in_frame == frame->duration) {
            param.entry = true;
        }
        time_left_in_frame -= update_time;
        if(time_left_in_frame == 0) {
            param.exit = true;
        }
        param.duration = frame->duration;
        param.current_frame_time = param.duration - time_left_in_frame;
        param.current_frame_nr = current_frame;
        param.user_data = effect->userdata;
        frame->update(&param);

        if (param.exit) {
            current_frame++;
            if(current_frame == effect->num_frames) {
                if (effect->loop != EFFECT_LOOP_INFINITE) {
                    num_loops++;
                    if (num_loops == effect->loop)
                        return true;
                }
                current_frame = 0;
            }
            time_left_in_frame = effect->frames[current_frame].duration;
        }
    }
    return false;
}

void update_effects(uint16_t abstime) {
    uint16_t dt = abstime - last_time;
    // We don't want huge deltas that could lock everything up
    if (dt > 1000) {
        dt = 1000;
    }
    last_time = abstime;
    for (unsigned i = 0; i < EFFECT_MAX_SLOTS; i++) {
        if (active_effects & (1u << i)) {
            effect_runtime_t* effect = &runtimes[i];
            bool effect_finished = update_effect(current_time, dt, effect);
            if (effect_finished) {
                remove_effect(i);
            }
        }
    }
    current_time += dt;
}

