#include "logger.h"

#include <string>
#include <gtest/gtest.h>

using namespace lon;

TEST(LogTest, LogLeveltoString) {
    EXPECT_STREQ(Logger::levelToString(Logger::Level::DEBUG),
        "DEBUG");
    EXPECT_STREQ(Logger::levelToString(Logger::Level::WARN),
        "WARN");
    EXPECT_STREQ(Logger::levelToString(Logger::Level::INFO),
        "INFO");
    EXPECT_STREQ(Logger::levelToString(Logger::Level::ERROR),
        "ERROR");
    EXPECT_STREQ(Logger::levelToString(Logger::Level::FATAL),
        "FATAL");


    EXPECT_EQ(Logger::levelFromString("debug"), Logger::Level::DEBUG);
    EXPECT_EQ(Logger::levelFromString("Warn"), Logger::Level::WARN);
    EXPECT_EQ(Logger::levelFromString("InFo"), Logger::Level::INFO);
    EXPECT_EQ(Logger::levelFromString("ERrOr"), Logger::Level::ERROR);
    EXPECT_EQ(Logger::levelFromString("FATAL"), Logger::Level::FATAL);
    EXPECT_EQ(Logger::levelFromString("wrong"), Logger::Level::SIZE);

}


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
