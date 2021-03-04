#include "base/chrono_helper.h"
#include "base/print_helper.h"
#include "logger.h"
#include "logging/logger_flusher.h"


#include <fmt/core.h>
#include <fmt/format.h>
#include <regex>
#include <string>

using namespace lon;

const char write_str[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$ % "
    "&\'()*+,-./:;<=>?@[\\]^_`{|}~ \t\n\r\x0b\x0c";
const char* log_format  = "%d{%Y-%m-%d %H:%M:%S} %t %E[%p][%c] <%f:%l> %m%n";
const char* log_name    = "log_name";
constexpr int size      = sizeof(write_str) - 1;
constexpr int loop_time = 1000000;

static size_t log_length = 0;


void calculateLength() {
    printDividing("log info");
    auto string_logger  = std::make_shared<Logger>(log_name);
    auto string_flusher = new log::StringFlusher;
    string_logger->addOneFlusher(
        std::unique_ptr<log::StringFlusher>(string_flusher));
    string_logger->setFormatters(log_format);
    LON_LOG_INFO(string_logger) << write_str;
    String str = string_flusher->log;
    fmt::print("size: {}\nstr:{}", str.size(), str);
    log_length = str.size();
}

void runSimple() {
    auto logger = std::make_shared<Logger>(log_name);
    logger->addOneFlusher(
        std::make_unique<log::SimpleFileFlusher>("/tmp/simple.log"));
    logger->setFormatters(log_format);
    size_t time_span;
    {
        lon::measure::GetTimeSpan gettimespan(&time_span);
        for (int i = 0; i < loop_time; ++i) {
            LON_LOG_INFO(logger) << write_str;
        }
    }

    const size_t total_size = loop_time * log_length;
    fmt::print("\n");
    printDividing("simple log speed");

    fmt::print("write {} bytes in {} ms, speed: {:.3f}Mib/s\n",
               total_size,
               time_span,
               static_cast<double>(total_size) / 1024 / 1024 /
                   static_cast<double>(time_span) * 1000);
    fmt::print("write {} times in {} ms, {} time/s\n",
               loop_time,
               time_span,
               loop_time / static_cast<double>(time_span) * 1000);
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void runStr() {
    auto logger         = std::make_shared<Logger>(log_name);
    auto string_flusher = new log::StringFlusher;
    logger->addOneFlusher(std::unique_ptr<log::StringFlusher>(string_flusher));
    logger->setFormatters(log_format);
    size_t time_span;
    {
        lon::measure::GetTimeSpan gettimespan(&time_span);
        for (int i = 0; i < loop_time; ++i) {
            LON_LOG_INFO(logger) << write_str;
            string_flusher->log.clear();  //去除内存分配带来的影响.
        }
    }

    const size_t total_size = loop_time * log_length;
    fmt::print("\n");
    printDividing("str log speed");

    fmt::print("write {} bytes in {} ms, speed: {:.3f}Mib/s\n",
               total_size,
               time_span,
               static_cast<double>(total_size) / 1024 / 1024 /
                   static_cast<double>(time_span) * 1000);
    fmt::print("write {} times in {} ms, {} time/s\n",
               loop_time,
               time_span,
               loop_time / static_cast<double>(time_span) * 1000);
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void runProtected() {
    auto logger = std::make_shared<Logger>(log_name);
    logger->addOneFlusher(
        std::make_unique<log::ProtectedFileFlusher>("/tmp/protected.log"));
    logger->setFormatters(log_format);
    size_t time_span;
    {
        lon::measure::GetTimeSpan gettimespan(&time_span);
        for (int i = 0; i < loop_time; ++i) {
            LON_LOG_INFO(logger) << write_str;
        }
    }

    const size_t total_size = loop_time * log_length;
    fmt::print("\n");
    printDividing("protected log speed");

    fmt::print("write {} bytes in {} ms, speed: {:.3f}Mib/s\n",
               total_size,
               time_span,
               static_cast<double>(total_size) / 1024 / 1024 /
                   static_cast<double>(time_span) * 1000);
    fmt::print("write {} times in {} ms, {} time/s\n",
               loop_time,
               time_span,
               loop_time / static_cast<double>(time_span) * 1000);
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void runSimpleWithoutFormat() {
    auto logger = std::make_shared<Logger>(log_name);
    logger->addOneFlusher(
        std::make_unique<log::SimpleFileFlusher>("/tmp/simple_nf.log"));
    logger->setFormatters("%m");

    size_t time_span;
    {
        lon::measure::GetTimeSpan gettimespan(&time_span);
        for (int i = 0; i < loop_time; ++i) {
            LON_LOG_INFO(logger) << write_str;
        }
    }

    const size_t total_size =
        loop_time * (sizeof(write_str) - 1);  // writestr-'\0'
    fmt::print("\n");
    printDividing("simple log  without formatters speed");
    fmt::print("\tstr size:{}\n", total_size);

    fmt::print("write {} bytes in {} ms, speed: {:.3f}Mib/s\n",
               total_size,
               time_span,
               static_cast<double>(total_size) / 1024 / 1024 /
                   static_cast<double>(time_span) * 1000);
    fmt::print("write {} times in {} ms, {} time/s\n",
               loop_time,
               time_span,
               loop_time / static_cast<double>(time_span) * 1000);
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void runWithoutFormatFlusher() {
    auto logger = std::make_shared<Logger>(log_name);
    logger->setFormatters("%m");

    size_t time_span;
    {
        lon::measure::GetTimeSpan gettimespan(&time_span);
        for (int i = 0; i < loop_time; ++i) {
            LON_LOG_INFO(logger) << write_str;
        }
    }

    const size_t total_size =
        loop_time * (sizeof(write_str) - 1);  // writestr-'\0'
    fmt::print("\n");
    printDividing("log  without formatters and flusher speed");
    fmt::print("\tstr size:{}\n", total_size);

    fmt::print("write {} bytes in {} ms, speed: {:.3f}Mib/s\n",
               total_size,
               time_span,
               static_cast<double>(total_size) / 1024 / 1024 /
                   static_cast<double>(time_span) * 1000);
    fmt::print("write {} times in {} ms, {} time/s\n",
               loop_time,
               time_span,
               loop_time / static_cast<double>(time_span) * 1000);
    std::this_thread::sleep_for(std::chrono::seconds(2));
}


int main() {
    calculateLength();
    runSimple();
    runProtected();
    runStr();
    runSimpleWithoutFormat();
    runWithoutFormatFlusher();
    return 0;
}