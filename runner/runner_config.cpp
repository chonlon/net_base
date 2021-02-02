#include "base/print_helper.h"

#include <string>
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <fmt/core.h>

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

int main() {
    YAML::Node root = YAML::LoadFile("bin/conf/test.yml");
    std::list<std::pair<std::string, const YAML::Node> > out;
    ListAllMember("", root, out);
    for(auto& i : out) {
        std::cout << i.first << ":\n--\n" << i.second << "\n-----\n\n";
    }

    {
        CaseMarker marker{"mine"};
        std::cout << root["logs"] << '\n';
        fmt::print("{:.3f}\n", root["system"]["port"].as<double>());
    }
}
