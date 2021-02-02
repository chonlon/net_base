#include "logger.h"

#include <string>
#include <gtest/gtest.h>

using namespace lon;

TEST(LogTest, LogLeveltoString) {
    EXPECT_STREQ(Logger::levelToString(Level::DEBUG),
        "DEBUG");
    EXPECT_STREQ(Logger::levelToString(Level::WARN),
        "WARN");
    EXPECT_STREQ(Logger::levelToString(Level::INFO),
        "INFO");
    EXPECT_STREQ(Logger::levelToString(Level::ERROR),
        "ERROR");
    EXPECT_STREQ(Logger::levelToString(Level::FATAL),
        "FATAL");


    EXPECT_EQ(Logger::levelFromString("debug"), Level::DEBUG);
    EXPECT_EQ(Logger::levelFromString("Warn"), Level::WARN);
    EXPECT_EQ(Logger::levelFromString("InFo"), Level::INFO);
    EXPECT_EQ(Logger::levelFromString("ERrOr"), Level::ERROR);
    EXPECT_EQ(Logger::levelFromString("FATAL"), Level::FATAL);
    EXPECT_EQ(Logger::levelFromString("wrong"), Level::SIZE);

}


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
