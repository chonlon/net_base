#pragma once

#include "logger.h"
#include "config.h"

namespace lon {
    class BaseMainConfig : public Noncopyable
    {
    public:
        BaseMainConfig() = delete;
        ~BaseMainConfig() = delete;

        static lon::JsonConfig* getInstance() {
            try {
                static lon::JsonConfig instance("conf/main.json");
                return &instance;
            } catch (std::exception& e) {
                fmt::print("error: main config file load failed!!!; {}\n", e.what());
                static lon::JsonConfig instance;
                return &instance;
            }
        }
    };

}
