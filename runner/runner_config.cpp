#include "config.h"
#include "base/print_helper.h"

#include <string>
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <fmt/core.h>
#include <fmt/ranges.h>


template <typename T>
T get(const lon::String& key, YAML::Node node_)  {
    auto keys          = lon::splitStringPiece(key, ".");
    YAML::Node current = node_;
    // std::cout << node_ << "\n\n\n";
    for (auto& _key : keys) {
        if (current.IsNull()) {
            throw lon::config::KeyNotFound(fmt::format("key:{}", key));
        }

        current = current[lon::String(_key)];
    }

    try {
        return current.as<T>();
    } catch (YAML::BadConversion& e) {
        throw lon::config::ConvertFailed(fmt::format("key:{}, type:{}", key, typeid(T).name()));
    }
}

void raw_yaml() {
    CaseMarker marker{"raw_yaml"};
    auto root = YAML::LoadFile("conf/test.yml");
    std::cout << root["system"]["port"].as<int>();
    fmt::print("{}\n", root["system"]["int_vec"].as<std::vector<int>>());
}
void raw2() {
    CaseMarker marker{"raw_2"};
    auto root = YAML::LoadFile("conf/test.yml");
    fmt::print("{}\n", get<int>("system.port", root));

    std::cout << root << "\n\n\n";

    fmt::print("{}\n", get<std::vector<int>>("system.int_vec", root));
}

void yamlConfig() {
    CaseMarker marker{"yaml-config"};
    lon::YamlConfig config("conf/test.yml");
    fmt::print("{}\n", config.get<int>("system.port"));
    fmt::print("{}\n", config.get<std::vector<int>>("system.int_vec"));
    fmt::print("{}\n", config.getIfExists<int>("__not_exists_key", 100));
    fmt::print("{}\n", config.get<int>("__not_exists_key"));
}
struct unkonw{};
void jsonConfig() {
    CaseMarker marker{ "json-config" };
    lon::JsonConfig config("conf/test.json");
    fmt::print("{}\n", config.get<int>("system.port"));
    fmt::print("{}\n", config.get<std::vector<int>>("system.int_vec"));
    fmt::print("{}\n", config.get<std::map<std::string,int>>("system.str_int_map"));
    fmt::print("{}\n", config.getIfExists<int>("__not_exists_key", 100));
    fmt::print("{}\n", config.get<int>("__not_exists_key"));
}


int main() {
    raw_yaml();
    // raw2();
    yamlConfig();
    jsonConfig();
}
