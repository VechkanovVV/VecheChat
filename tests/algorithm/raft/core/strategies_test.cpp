#include "strategies.h"

#include <gtest/gtest.h>

TEST(RaftStrategiesTest, CheckFunctionPassing)
{
    int test_value = 0;
    ElectionTimerStrategy el{[&test_value] { test_value++; }, 150, 300, 47};
    el.onTimeout();
    EXPECT_EQ(test_value, 1);

    HeartbeatTimerStrategy hb{[&test_value] { test_value++; }, 50};
    hb.onTimeout();
    EXPECT_EQ(test_value, 2);
}

TEST(RaftStrategiesTest, CheckExceptions)
{
    EXPECT_THROW(ElectionTimerStrategy([] {}, 300, 150, 47), std::invalid_argument);
    EXPECT_THROW(HeartbeatTimerStrategy([] {}, 0), std::invalid_argument);
}