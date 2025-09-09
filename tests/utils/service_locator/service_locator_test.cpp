#include "service_locator.h"

#include <gtest/gtest.h>

TEST(ServiceLocatorTests, RegisterAndGetLogger)
{
    struct Logger
    {
        std::string log(const std::string& msg) const { return "[Log: " + msg + "]\n"; }
    };

    utils::ServiceLocator sl{};
    EXPECT_FALSE(sl.has<Logger>());

    ASSERT_NO_THROW(sl.registerService<Logger>());
    EXPECT_TRUE(sl.has<Logger>());

    auto logger = sl.get<Logger>();
    ASSERT_NE(logger, nullptr);

    auto result = logger->log("kek");
    EXPECT_EQ("[Log: kek]\n", result);
}
