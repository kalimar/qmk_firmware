#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <tuple>

using namespace testing;

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

    static void update1(const effect_param_t* param) {
        mock->update1(param);
    }

    static void update2(const effect_param_t* param) {
        mock->update2(param);
    }

    class update_mock_t {
    public:
        MOCK_METHOD1(update1, void (const effect_param_t* param));
        MOCK_METHOD1(update2, void (const effect_param_t* param));
    };

    static update_mock_t* mock;
private:
    update_mock_t mock_instance;
};

EffectTests::update_mock_t* EffectTests::mock = nullptr;

TEST_F(EffectTests, UpdateFirstFrame_ShouldCallUpdateWithCorrectParameters) {
    effect_frame_t frames[] = {
        {
            .duration = 10,
            .update = update1
        }
    };
    effect_runtime_t runtime;

    add_effect(&runtime, frames, sizeof(frames));
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::duration, 10),
        Field(&effect_param_t::current_frame_time, 1),
        Field(&effect_param_t::current_frame_nr, 0)
    )));
    update_effects(1);
}

TEST_F(EffectTests, UpdateEffectThatHasEnded_ShouldCallNothing) {
    effect_frame_t frames[] = {
        {
            .duration = 5,
            .update = update1
        }
    };
    effect_runtime_t runtime;

    add_effect(&runtime, frames, sizeof(frames));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 5))).Times(1);
    update_effects(5);
    update_effects(1);
}

TEST_F(EffectTests, UpdateEffectWithDtLessThanDuration_ShouldAllowTheSameEffectToBeUpdatedAgain) {
    effect_frame_t frames[] = {
        {
            .duration = 5,
            .update = update1
        }
    };
    effect_runtime_t runtime;

    add_effect(&runtime, frames, sizeof(frames));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 4)));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 5)));
    update_effects(4);
    update_effects(1);
}

TEST_F(EffectTests, UpdateEffectWithDtLongerThanDuration_ShouldCallUpdateAndEndTheEffect) {
    effect_frame_t frames[] = {
        {
            .duration = 5,
            .update = update1
        }
    };
    effect_runtime_t runtime;

    add_effect(&runtime, frames, sizeof(frames));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 5)));
    update_effects(6);
    update_effects(1);
}

TEST_F(EffectTests, UpdateSameEffectMultipleTimes_ShouldAllowUpdateToBeCalledForEachOfThem) {
    effect_frame_t frames[] = {
        {
            .duration = 10,
            .update = update1
        }
    };
    effect_runtime_t runtime;

    add_effect(&runtime, frames, sizeof(frames));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 1)));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 4)));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 9)));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 10)));
    update_effects(1);
    update_effects(3);
    update_effects(5);
    update_effects(2);
    // The effect should have ended here
    update_effects(1);
}

TEST_F(EffectTests, UpdateEffectWithZeroDt_ShouldDoNothing) {
    effect_frame_t frames[] = {
        {
            .duration = 5,
            .update = update1
        }
    };
    effect_runtime_t runtime;

    add_effect(&runtime, frames, sizeof(frames));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 1)));
    update_effects(0);
    update_effects(1);
}


TEST_F(EffectTests, UpdateSecondFrameFromStart_ShouldUpdateItWithTheCorrectParameters) {
    effect_frame_t frames[] = {
        {
            .duration = 10,
            .update = update1
        },
        {
            .duration = 5,
            .update = update2
        }
    };
    effect_runtime_t runtime;
    add_effect(&runtime, frames, sizeof(frames));
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::duration, 10),
        Field(&effect_param_t::current_frame_time, 10),
        Field(&effect_param_t::current_frame_nr, 0)
    )));
    update_effects(10);
    EXPECT_CALL(*mock, update2(AllOf(
        Field(&effect_param_t::duration, 5),
        Field(&effect_param_t::current_frame_time, 1),
        Field(&effect_param_t::current_frame_nr, 1)
    )));
    update_effects(1);
}

TEST_F(EffectTests, UpdateFirstFrameWithExcessTime_ShouldUpdateTheSecondFrameWithTheRemainder) {
    effect_frame_t frames[] = {
        {
            .duration = 5,
            .update = update1
        },
        {
            .duration = 10,
            .update = update2
        }
    };
    effect_runtime_t runtime;
    add_effect(&runtime, frames, sizeof(frames));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 5)));
    EXPECT_CALL(*mock, update2(Field(&effect_param_t::current_frame_time, 6)));
    update_effects(11);
}

TEST_F(EffectTests, UpdatingWithBigEnoughDt_ShouldUpdateAllTheFramesBetween) {
    effect_frame_t frames[] = {
        {
            .duration = 5,
            .update = update1
        },
        {
            .duration = 10,
            .update = update2
        },
        {
            .duration = 10,
            .update = update2
        }
    };
    effect_runtime_t runtime;
    add_effect(&runtime, frames, sizeof(frames));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 5)));
    EXPECT_CALL(*mock, update2(Field(&effect_param_t::current_frame_time, 10))).Times(2);
    update_effects(26);
    update_effects(1);
}

TEST_F(EffectTests, UpdatingInstantFrame_ShouldMoveToTheNextFrameWithoutUsingTime) {
    effect_frame_t frames[] = {
        {
            // In c the duration can be left out complete, but c++ requires it
            .duration = 0,
            .update = update1
        },
        {
            .duration = 10,
            .update = update2
        },
    };
    effect_runtime_t runtime;
    add_effect(&runtime, frames, sizeof(frames));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 0)));
    EXPECT_CALL(*mock, update2(Field(&effect_param_t::current_frame_time, 2)));
    update_effects(2);
}
