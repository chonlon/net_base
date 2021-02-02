#include "config.h"
#include "base/print_helper.h"

#include <string>
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <fmt/core.h>
#include <fmt/ranges.h>

static void ListAllMember(const std::string& prefix,
                          const YAML::Node& node,
                          std::list<std::pair<std::string, const YAML::Node> >& output) {
    if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678")
        != std::string::npos) {
        // SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    if (node.IsMap()) {
        for (auto it = node.begin();
            it != node.end(); ++it) {
            ListAllMember(prefix.empty() ? it->first.Scalar()
                : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

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

void raw() {
    CaseMarker marker{"raw"};
    auto root = YAML::LoadFile("bin/conf/test.yml");
    std::cout << root["system"]["port"].as<int>();
    fmt::print("{}\n", root["system"]["int_vec"].as<std::vector<int>>());
}
void raw2() {
    CaseMarker marker{"raw"};
    auto root = YAML::LoadFile("bin/conf/test.yml");
    fmt::print("{}\n", get<int>("system.port", root));

    std::cout << root << "\n\n\n";

    fmt::print("{}\n", get<std::vector<int>>("system.int_vec", root));
}

void yamlConfig() {
    CaseMarker marker{"config"};
    lon::YamlConfig config("bin/conf/test.yml");
    fmt::print("{}\n", config.get<int>("system.port"));
    fmt::print("{}\n", config.get<std::vector<int>>("system.int_vec"));
    fmt::print("{}\n", config.getIfExists<int>("__not_exists_key", 100));
    fmt::print("{}\n", config.get<int>("__not_exists_key"));
}


int main() {
    raw();
    // raw2();
    yamlConfig();

}
