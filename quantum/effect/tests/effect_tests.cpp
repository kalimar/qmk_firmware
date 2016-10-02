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
        clear_all_effects();
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
    int userdata = 5;

    add_effect(&runtime, frames, sizeof(frames), &userdata, EFFECT_NO_LOOP);
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::duration, 10),
        Field(&effect_param_t::current_frame_time, 1),
        Field(&effect_param_t::current_frame_nr, 0),
        Field(&effect_param_t::user_data, &userdata)
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

    add_effect(&runtime, frames, sizeof(frames), NULL, EFFECT_NO_LOOP);
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

    add_effect(&runtime, frames, sizeof(frames), NULL, EFFECT_NO_LOOP);
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

    add_effect(&runtime, frames, sizeof(frames), NULL, EFFECT_NO_LOOP);
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

    add_effect(&runtime, frames, sizeof(frames), NULL, EFFECT_NO_LOOP);
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

    add_effect(&runtime, frames, sizeof(frames), NULL, EFFECT_NO_LOOP);
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
    int userdata = 3;
    effect_runtime_t runtime;
    add_effect(&runtime, frames, sizeof(frames), &userdata, EFFECT_NO_LOOP);
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::duration, 10),
        Field(&effect_param_t::current_frame_time, 10),
        Field(&effect_param_t::current_frame_nr, 0),
        Field(&effect_param_t::user_data, &userdata)
    )));
    update_effects(10);
    EXPECT_CALL(*mock, update2(AllOf(
        Field(&effect_param_t::duration, 5),
        Field(&effect_param_t::current_frame_time, 1),
        Field(&effect_param_t::current_frame_nr, 1),
        Field(&effect_param_t::user_data, &userdata)
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
    add_effect(&runtime, frames, sizeof(frames), NULL, EFFECT_NO_LOOP);
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
    add_effect(&runtime, frames, sizeof(frames), NULL, EFFECT_NO_LOOP);
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
    add_effect(&runtime, frames, sizeof(frames), NULL, EFFECT_NO_LOOP);
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 0)));
    EXPECT_CALL(*mock, update2(Field(&effect_param_t::current_frame_time, 2)));
    update_effects(2);
}

TEST_F(EffectTests, UpdatingInstantFrameAfterNormalWithExactTime_IsUpdatedDuringTheNextUpdate) {
    effect_frame_t frames[] = {
        {
            .duration = 10,
            .update = update1
        },
        {
            .duration = 0,
            .update = update2
        },
    };
    effect_runtime_t runtime;
    add_effect(&runtime, frames, sizeof(frames), NULL, EFFECT_NO_LOOP);
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 10)));
    update_effects(10);
    Mock::VerifyAndClearExpectations(mock);
    EXPECT_CALL(*mock, update2(Field(&effect_param_t::current_frame_time, 0)));
    update_effects(10);
}

TEST_F(EffectTests, OneFrameInfiniteLoopingEffect_ShouldLoopCorrectly) {
    effect_frame_t frames[] = {
        {
            .duration = 5,
            .update = update1
        },
    };
    effect_runtime_t runtime;
    add_effect(&runtime, frames, sizeof(frames), NULL, EFFECT_LOOP_INFINITE);
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 5)));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 1)));
    update_effects(6);
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 5))).Times(3);
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 1)));
    update_effects(15);
}

TEST_F(EffectTests, OneFrameFixedLoop_ShouldLoopCorrectly) {
    effect_frame_t frames[] = {
        {
            .duration = 5,
            .update = update1
        },
    };
    effect_runtime_t runtime;
    add_effect(&runtime, frames, sizeof(frames), NULL, 2);
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 1)));
    update_effects(1);
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 5))).Times(2);
    update_effects(10);
    update_effects(2);
}

TEST_F(EffectTests, TwoFrameFixedLoop_ShouldLoopCorrectly) {
    effect_frame_t frames[] = {
        {
            .duration = 5,
            .update = update1
        },
        {
            .duration = 0,
            .update = update2
        },
    };
    effect_runtime_t runtime;
    add_effect(&runtime, frames, sizeof(frames), NULL, 3);
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 5)));
    EXPECT_CALL(*mock, update2(Field(&effect_param_t::current_frame_time, 0)));
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 1)));
    update_effects(6);
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 5))).Times(2);
    EXPECT_CALL(*mock, update2(Field(&effect_param_t::current_frame_time, 0))).Times(2);
    update_effects(10);
    Mock::VerifyAndClear(mock);
    update_effects(10);
}

TEST_F(EffectTests, EffectEntryAndExit_ShouldBeSetCorrectly) {
    effect_frame_t frames[] = {
        {
            .duration = 5,
            .update = update1
        },
    };
    effect_runtime_t runtime;
    add_effect(&runtime, frames, sizeof(frames), NULL, EFFECT_NO_LOOP);
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::entry, true),
        Field(&effect_param_t::exit, false)
    )));
    update_effects(2);
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::entry, false),
        Field(&effect_param_t::exit, false)
    )));
    update_effects(2);
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::entry, false),
        Field(&effect_param_t::exit, true)
    )));
    update_effects(2);
}

TEST_F(EffectTests, EffectEntryAndExit_CanBeSetAtTheSameTime) {
    effect_frame_t frames[] = {
        {
            .duration = 5,
            .update = update1
        },
    };
    effect_runtime_t runtime;
    add_effect(&runtime, frames, sizeof(frames), NULL, EFFECT_NO_LOOP);
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::entry, true),
        Field(&effect_param_t::exit, true)
    )));
    update_effects(5);
}

TEST_F(EffectTests, UpdateTwoEffects_ShouldNotAffectEachOther) {
    effect_frame_t frames1[] = {
        {
            .duration = 5,
            .update = update1
        },
    };
    effect_frame_t frames2[] = {
        {
            .duration = 10,
            .update = update2
        },
    };
    int data1 = 1;
    int data2 = 2;
    effect_runtime_t runtime1;
    effect_runtime_t runtime2;
    add_effect(&runtime1, frames1, sizeof(frames1), &data1, EFFECT_NO_LOOP);
    add_effect(&runtime2, frames2, sizeof(frames2), &data2, EFFECT_NO_LOOP);
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::current_frame_time, 2),
        Field(&effect_param_t::user_data, &data1)
    )));
    EXPECT_CALL(*mock, update2(AllOf(
        Field(&effect_param_t::current_frame_time, 2),
        Field(&effect_param_t::user_data, &data2)
    )));
    update_effects(2);
    EXPECT_CALL(*mock, update1(Field(&effect_param_t::current_frame_time, 5)));
    EXPECT_CALL(*mock, update2(Field(&effect_param_t::current_frame_time, 6)));
    update_effects(4);
    EXPECT_CALL(*mock, update2(Field(&effect_param_t::current_frame_time, 10)));
    update_effects(6);
}


TEST_F(EffectTests, UpdateTwoEffectsWithSameFrames_ShouldGetDifferentUserData) {
    effect_frame_t frames[] = {
        {
            .duration = 5,
            .update = update1
        },
    };
    int data1 = 1;
    int data2 = 2;
    effect_runtime_t runtime1;
    effect_runtime_t runtime2;
    add_effect(&runtime1, frames, sizeof(frames), &data1, EFFECT_NO_LOOP);
    add_effect(&runtime2, frames, sizeof(frames), &data2, EFFECT_NO_LOOP);
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::current_frame_time, 2),
        Field(&effect_param_t::user_data, &data1)
    )));
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::current_frame_time, 2),
        Field(&effect_param_t::user_data, &data2)
    )));
    update_effects(2);
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::current_frame_time, 5),
        Field(&effect_param_t::user_data, &data1)
    )));
    EXPECT_CALL(*mock, update1(AllOf(
        Field(&effect_param_t::current_frame_time, 5),
        Field(&effect_param_t::user_data, &data2)
    )));
    update_effects(4);
}
