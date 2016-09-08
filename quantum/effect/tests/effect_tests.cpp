#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <tuple>

extern "C" {
    #include "../effect.h"
}

TEST(EffectTests, CreateEffectWithOneFrame) {
    auto update = []() {

    };

    effect_frame_t effect[] = {
        {
            .duration = 10,
            .update = update
        }
    };
}

TEST(EffectTests, CreateEffectWithTwoFrames) {
    auto update = []() {

    };

    auto update2 = []() {

    };

    effect_frame_t effect[] = {
        {
            .duration = 10,
            .update = update
        },
        {
            // No duration here means in instant keyframe
            // The GCC c++ version don't like leaving this out, but when compiling in pure c it should be
            // possible
            .duration = 0,
            .update = update2
        }
    };
}
