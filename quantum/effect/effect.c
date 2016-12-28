#include "effect.h"

static effect_runtime_t* active_effects = NULL;
static uint16_t last_time = 0;
static uint32_t current_time = 0;

void add_effect(effect_runtime_t* runtime, effect_frame_t* frames, unsigned int frames_size, void* userdata,
    uint16_t loops) {
    runtime->frames = frames;
    runtime->start_time = current_time;
    runtime->num_frames = frames_size / sizeof(effect_frame_t);
    runtime->user_data = userdata;
    runtime->loop = loops;
    if (!active_effects) {
        active_effects = runtime;
        runtime->next = NULL;
    } else {
        effect_runtime_t* prev = active_effects;
        while (prev->next && prev != runtime) {
            prev = prev->next;
        }
        if (prev != runtime) {
            prev->next = runtime;
            runtime->next = NULL;
        }
    }
}

void remove_effect(effect_runtime_t* runtime) {
    if (runtime == active_effects) {
        active_effects = active_effects->next;
    } else {
        effect_runtime_t* effect = active_effects;
        if (effect->next == runtime) {
            effect->next = effect->next->next;
            return;
        }
    }
}

void clear_all_effects(void) {
    active_effects = NULL;
}

static void update_effect(unsigned int dt, effect_runtime_t* effect) {
    uint32_t effect_length = 0;
    //TODO: This could be pre-calculated
    for (int i=0; i<effect->num_frames; i++) {
        effect_length += effect->frames[i].duration;
    }
    uint32_t time_since_effect_start = current_time - effect->start_time;
    uint32_t current_effect_time = time_since_effect_start % effect_length;
    uint32_t num_loops = time_since_effect_start  / effect_length;
    // If the previous iteration ended exactly at the end of the effect, we are still on the previous
    // iteration, so fixup the time and loop count
    if (current_effect_time == 0 && num_loops > 0) {
        num_loops--;
        current_effect_time = effect_length;
    }
    unsigned current_frame = 0;
    if (num_loops < effect->loop || effect->loop == EFFECT_LOOP_INFINITE) {
        while (current_effect_time >= effect->frames[current_frame].duration) {
            current_effect_time -= effect->frames[current_frame].duration;
            // Don't move over zero duration frames
            if (current_effect_time == 0 && effect->frames[current_frame].duration == 0) {
                break;
            }
            current_frame++;
        }
    }
    else {
        return;
    }
    unsigned time_left_in_frame = effect->frames[current_frame].duration - current_effect_time;

    while (dt > 0 && current_frame < effect->num_frames) {
        effect_frame_t* frame = &effect->frames[current_frame];
        unsigned update_time = dt <= time_left_in_frame ? dt : time_left_in_frame;
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
        param.user_data = effect->user_data;
        frame->update(&param);

        if (time_left_in_frame == 0) {
            current_frame++;
            if(current_frame == effect->num_frames) {
                if (effect->loop != EFFECT_LOOP_INFINITE) {
                    num_loops++;
                    if (num_loops == effect->loop)
                        return;
                }
                current_frame = 0;
            }
            if (current_frame < effect->num_frames) {
                time_left_in_frame = effect->frames[current_frame].duration;
            }
        }
    }
}

void update_effects(uint16_t abstime) {
    uint16_t dt = abstime - last_time;
    // We don't want huge Dts that could look everything up
    if (dt > 1000) {
        dt = 1000;
    }
    last_time = abstime;
    effect_runtime_t* effect = active_effects;
    while(effect) {
        update_effect(dt, effect);
        effect = effect->next;
    }
    current_time += dt;
}

