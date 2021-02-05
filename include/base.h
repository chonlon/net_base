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
            static lon::JsonConfig instance("conf/main.json");
            return &instance;
        }
    };

}
