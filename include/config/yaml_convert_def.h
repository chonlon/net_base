#pragma once
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include "yaml-cpp/node/iterator.h"

// define stl container converter not implemented in yaml-cpp

namespace YAML {
    template <typename T>
    struct convert<std::set<T>>
    {
        static Node encode(const std::set<T>& rhs) {
            //not needed.
            Node node;
            return node;
        }

        static bool decode(const Node& node, std::set<T>& rhs) {

            for (size_t i = 0; i < node.size(); ++i) {
                rhs.insert(node[i].as<T>());
            }
            return true;
        }
    };

    template <typename T>
    struct convert<std::unordered_set<T>>
    {
        static Node encode(const std::unordered_set<T>& rhs) {
            //not needed.
            Node node;
            return node;
        }

        static bool decode(const Node& node, std::unordered_set<T>& rhs) {
            for (size_t i = 0; i < node.size(); ++i) {
                rhs.insert(node[i].as<T>());
            }
            return true;
        }
    };

    template <typename K, typename V>
    struct convert<std::unordered_map<K,V>>
    {
        static Node encode(const std::unordered_map<K, V>& rhs) {
            //not needed.
            Node node;
            return node;
        }

        static bool decode(const Node& node, std::unordered_map<K,V>& rhs) {
            for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
#if defined(__GNUC__) && __GNUC__ < 4
                // workaround for GCC 3:
                rhs[it->first.template as<K>()] = it->second.template as<V>();
#else
                rhs[it->first.as<K>()] = it->second.as<V>();
#endif
            }
            return true;
        }
    };
}