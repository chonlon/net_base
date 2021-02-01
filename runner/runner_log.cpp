#include <logger.h>
#include <iostream>
#include <algorithm>


int main() {
    using namespace lon;
    std::cout
        << Logger::levelToString(Level::DEBUG) << '\n'
        << Logger::levelToString(Level::INFO) << '\n'
        << Logger::levelToString(Level::ERROR) << '\n'
        << Logger::levelToString(Level::FATAL) << '\n'
        << Logger::levelToString(Logger::levelFromString("WARN")) << '\n';

    auto logger = std::make_shared<Logger>();
    // LON_LOG_DEBUG(logger) << 123;
    for(int i = 0; i < 100000; ++i)
        LON_LOG_DEBUG(logger) << i << "-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$ % &\'()*+,-./:;<=>?@[\\]^_`{|}~";

    std::string s{"hello world"};
    auto iter = std::find(s.begin(), s.end(), 'w');
    std::string ss(lon::toStringPiece(iter, s.end()));
    std::cout << ss.size() << ' ' << ss;
}