#include "logger.h"
#include "logging/logger_flusher.h"


#include <regex>
#include <string>
#include <gtest/gtest.h>

using namespace lon;
const char* logger_name = "Test";

#define DECLEAR_LOGGER Logger::ptr logger = std::make_shared<Logger>(logger_name); \
        auto string_flusher = new log::StringFlusher; \
        logger->addOneFlusher(std::unique_ptr<log::Flusher>(string_flusher));

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

TEST(LogTest, LogLevel) {
    DECLEAR_LOGGER
    logger->setFormatters(
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T<%f:%l>%T%m%n");

    logger->setLevel(DEBUG);
    LON_LOG_DEBUG(logger) << "test";
    EXPECT_FALSE(string_flusher->log.empty());
    string_flusher->log.clear();
    LON_LOG_INFO(logger) << "test";
    EXPECT_FALSE(string_flusher->log.empty());
    string_flusher->log.clear();
    LON_LOG_WARN(logger) << "test";
    EXPECT_FALSE(string_flusher->log.empty());
    string_flusher->log.clear();
    LON_LOG_ERROR(logger) << "test";
    EXPECT_FALSE(string_flusher->log.empty());
    string_flusher->log.clear();


    logger->setLevel(WARN);
    LON_LOG_DEBUG(logger) << "test";
    EXPECT_TRUE(string_flusher->log.empty());
    string_flusher->log.clear();
    LON_LOG_INFO(logger) << "test";
    EXPECT_TRUE(string_flusher->log.empty());
    string_flusher->log.clear();
    LON_LOG_WARN(logger) << "test";
    EXPECT_FALSE(string_flusher->log.empty());
    string_flusher->log.clear();
    LON_LOG_ERROR(logger) << "test";
    EXPECT_FALSE(string_flusher->log.empty());
    string_flusher->log.clear();
}

TEST(LogTest, LogOut) {
    DECLEAR_LOGGER
    logger->setFormatters(
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T<%f:%l>%T%m%n");

    const char* file_name = "test.cc";
    int32_t line          = 10;
    uint32_t thread_id    = 0;
    uint32_t elapsed_ms   = 1;
    uint32_t fiber_id     = 2;
    uint64_t _time        = static_cast<uint64_t>(time(nullptr));
    lon::Level level      = lon::Level::DEBUG;

    LogEvent event(
        file_name,
        line,
        level,
        thread_id,
        elapsed_ms,
        fiber_id,
        _time);
    {
        LogWrapper w(logger, event);
        w.stream << "log";
    }


    std::string string_log = string_flusher->log;
    std::smatch result;
    std::regex pattern("(\\d+-\\d+-\\d+\\s\\d+:\\d+:\\d+)\\s*(.*)\\r\\n");


    if (std::regex_match(string_log, result, pattern)) {
        // part1 for time fmt check
        struct tm tm;
        time_t time = static_cast<time_t>(event.time);
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        EXPECT_STREQ(result.str(1).c_str(), buf);

        // part2 for rest check
        auto part2 = result.str(2);
        std::stringstream str_result;
        str_result << thread_id << '\t' << fiber_id
            << '\t' << '[' << lon::Logger::levelToString(level) << ']' << '\t'
            << '[' << logger_name << ']' << '\t'
            << '<' << file_name << ':' << line << '>'
            << '\t' << "log";
        auto s = str_result.str();
        EXPECT_STREQ(part2.c_str(), s.c_str());
    } else {
        EXPECT_FALSE(true);
    }
}

TEST(LogTest, LogFormat) {
    DECLEAR_LOGGER
    const char* msg = "log msg";
    logger->setFormatters("%t%T%m%n");
    LON_LOG_DEBUG(logger) << msg;


    std::stringstream ss;
    ss << lon::getThreadId() << '\t' << msg << "\r\n";
    EXPECT_STREQ(string_flusher->log.c_str(),
                 ss.str().c_str());
}


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
