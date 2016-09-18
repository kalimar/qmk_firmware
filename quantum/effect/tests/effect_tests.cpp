#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <tuple>

extern "C" {
    #include "quantum/effect/effect.h"
}

class EffectTests : public testing::Test {
public:
    EffectTests() {
        mock = &mock_instance;
    }

    ~EffectTests() {
        mock = nullptr;
    }

    static void update1() {
        mock->update1();
    }

    static void update2() {
        mock->update2();
    }

    class update_mock_t {
    public:
        MOCK_METHOD0(update1, void ());
        MOCK_METHOD0(update2, void ());
    };

    static update_mock_t* mock;
private:
    update_mock_t mock_instance;
};

EffectTests::update_mock_t* EffectTests::mock = nullptr;

TEST_F(EffectTests, UpdatingFirstFrameOfEffectShouldCallUpdate) {
    effect_t effect[] = {
        {
            .duration = 10,
            .update = update1
        }
    };

    add_effect(effect, sizeof(effect));
    EXPECT_CALL(*mock, update1());
    update_effects(1);
}

TEST_F(EffectTests, CreateEffectWithTwoFrames) {
    effect_t effect[] = {
        {
            .duration = 10,
            .update = update1
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
