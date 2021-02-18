#include "config.h"

#include <logger.h>
#include <iostream>
#include <algorithm>
#include "logging/logger_data_convert.h"

const char* printable = "-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$ % &\'()*+,-./:;<=>?@[\\]^_`{|}~";

int main() {
    using namespace lon;
    std::cout
        << Logger::levelToString(Level::DEBUG) << '\n'
        << Logger::levelToString(Level::INFO) << '\n'
        << Logger::levelToString(Level::ERROR) << '\n'
        << Logger::levelToString(Level::FATAL) << '\n'
        << Logger::levelToString(Logger::levelFromString("WARN")) << '\n';

    auto logger = std::make_shared<Logger>("test");
    logger->addOneFlusher(std::make_unique<log::SimpleFileFlusher>("/tmp/log1.txt"));
    // flusher2);
    logger->addOneFlusher(std::make_unique<log::ProtectedFileFlusher>("/tmp/log2.txt"));
    logger->addOneFlusher(std::make_unique< log::AsyncFileLogFlusher>("/tmp/log3.txt"));
    // LON_LOG_DEBUG(logger) << 123;
    for(int i = 0; i < 1000; ++i)
        LON_LOG_DEBUG(logger) << i << printable;

    auto root_logger = lon::LogManager::getInstance()->getDefault();
    for (int i = 0; i < 100; ++i)
        LON_LOG_DEBUG(root_logger) << i << printable;
    auto system_logger = lon::LogManager::getInstance()->getLogger("system");
    for(int i = 0; i < 100; ++i) {
        LON_LOG_DEBUG(system_logger) << i << printable;
    }

    LON_LOG_DEFAULT_DEBUG() << "debug";
    LON_LOG_DEFAULT_WARN() << "warn";
    LON_LOG_DEFAULT_ERROR() << "error";

    std::cout << "generate filename: " << lon::logFileNameGenerate(lon::logFileNameParse("~/.logs/%d{%m-%d_%H}_%H-%P.log")) << '\n';

    lon::JsonConfig config("conf/test.json");
    auto data = config.get<std::vector<detail::LogConfigData>>("logs");
    std::cout << data[0].name;

    lon::YamlConfig config2("conf/test.yml");
    auto data2 = config.get<std::vector<detail::LogConfigData>>("logs");
    std::cout << data2[0].name;

}