#pragma once
#include "logger.h"
#include "base/typedef.h"
#include "logging/logger_flusher.h"


#include <yaml-cpp/yaml.h>
#include "yaml-cpp/node/iterator.h"
#include <nlohmann/json.hpp>


namespace lon {
    namespace detail {

        struct LogFlusherConfigData
        {
            // log::FlushFrequency frequency;
            String type;
            String name_pattern;
        };


        struct LogConfigData
        {
            String name;
            Level level;
            String formatter;
            std::vector<LogFlusherConfigData> flushers;
        };
    }

}

namespace YAML {
    template <>
    struct convert<lon::detail::LogFlusherConfigData>
    {
        static Node encode(const lon::detail::LogFlusherConfigData& rhs) {
            //not needed.
            Node node;
            return node;
        }

        static bool decode(const Node& node, lon::detail::LogFlusherConfigData& rhs) {


            // if (node["frequency"].IsDefined()) {
            //     rhs.frequency = lon::log::FlushFrequency::Day; //默认按天
            // }
            // else {
            //     rhs.frequency = /*from str*/
            // }
            //
            if (!node["type"].IsDefined()) {
                std::cerr << "log flusher type is not defined" << node << '\n';
                return false;
            }
            else {
                rhs.name_pattern = node["type"].as<lon::String>();
            }

            if (!node["pattern"].IsDefined()) {
                rhs.name_pattern = "%H-%p.log";
            } else {
                rhs.name_pattern = node["pattern"].as<lon::String>();
            }
 

            return true;
        }
    };

    template <>
    struct convert<lon::detail::LogConfigData>
    {
        static Node encode(const lon::detail::LogConfigData& rhs) {
            //not needed.
            Node node;
            return node;
        }

        static bool decode(const Node& node, lon::detail::LogConfigData& rhs) {
            if(!node["name"].IsDefined()) {
                std::cerr << "log config load failed, name is null" << node << '\n';
                return false;
            } else {
                rhs.name = node["name"].as<lon::String>();
            }

            if(!node["level"].IsDefined()) {
                rhs.level = lon::DEBUG; //默认最低
            } else {
                rhs.level = lon::Logger::levelFromString(node["level"].as<lon::String>());
            }

            if(node["formatter"].IsDefined()) {
                rhs.formatter = node["formatter"].as<lon::String>();
            }

            if(node["flusher"].IsDefined()) {
                rhs.flushers = node["flusher"].as<std::vector<lon::detail::LogFlusherConfigData>>();
            }
            
            return true;
        }
    };
}

namespace lon {
namespace detail {

    template <typename BasicJsonType>
    static void from_json(const BasicJsonType& j, lon::detail::LogFlusherConfigData& rhs) {
        if (j.find("type") == j.end()) {
            std::cerr << "log flusher type is not defined" << j << '\n';
            return;
        }
        else {
            rhs.type = j["type"].template get<lon::String>();
        }

        if (j.find("pattern") == j.end()) {
            rhs.name_pattern = "%H-%p.log";
        }
        else {
            rhs.name_pattern = j["pattern"].template get<lon::String>();
        }

    }

    template <typename BasicJsonType>
    static void from_json(const BasicJsonType& j, lon::detail::LogConfigData& rhs) {
        if(j.find("name") != j.end())
            rhs.name = j["name"].template get<lon::String>();
        else {
            std::cerr << "log config load failed, name is null" << j << '\n';
            return;
        }

        if(j.find("level") != j.end()) {
            rhs.level = lon::Logger::levelFromString(j["level"].template get<lon::String>());
        } else {
            rhs.level = lon::DEBUG;
        }

        if (j.find("formatter") != j.end()) {
            rhs.formatter = j["formatter"].template get<lon::String>();
        }

        if(j.find("flusher") != j.end()) {
            rhs.flushers = j["flusher"].template get<std::vector<lon::detail::LogFlusherConfigData>>();
        }
    }

    

}

}