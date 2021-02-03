#pragma once

#include "base/macro.h"
#include "base/typedef.h"
#include "config/config_exception.h"
#include "logger.h"

#include <iostream>
#include <fstream>
#include <any>
#include <unordered_map>
#include <fmt/core.h>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <nlohmann/json.hpp>

namespace lon {
// 两种方案, 如果是想用继承, 那么get函数不能是模板函数, 也就是说要么返回值是void*, 要么抽象返回值, 要么使用any(其实还是void*)
// 另一种就是采用类似contract的方式, 当然对于c++17并不能限制满足这样的约定, 需要主动满足这样的约定, 具体就是这些类采样相同的api
// 第一种方案看起来很美好, 实现起来还是颇有难度的, 比如yaml::node.as<>需要提供类型, 而我们的虚函数是不能应付这样的可变类型的
// 所以采用第二种方案

// 采用值方式提供结果
// 原因是值方式+不会修改文件就是线程安全的
// 另外配置读取的大多是比较小的值, copy的开销很小.

namespace detail {
    struct YamlConfigLoader;

    struct JsonConfigLoader;
}

class ConfigBase
{
public:

    virtual ~ConfigBase() = default;
protected:
    std::unordered_map<String, std::any> miss_map_;
    // store in miss_map_ means not found at file loaded;
};

class JsonConfig : public ConfigBase
{
friend detail::JsonConfigLoader;
using json=nlohmann::json;
public:
    ~JsonConfig() override {}
    JsonConfig(const char* filename) {
        std::ifstream i(filename);
        i >> json_object_;
    }

        /**
     * \brief 获取指定key的值
     * \tparam T return type
     * \param key
     * \exception KeyNotFound if key not exists.
     * \exception ConvertFailed if convert to target type failed.
     * \return T if exists
     */
    template <typename T>
    T get(const String& key) const { 
        if (auto iter = miss_map_.find(key); iter != miss_map_.end())
            return std::any_cast<T>(iter->second);
        std::vector<StringPiece> keys = splitStringPiece(key, ".");

        const json* obj_pointer = &json_object_;
        for(auto& _key : keys) {
            if(auto iter = obj_pointer->find(_key); iter != obj_pointer->end()) {
                obj_pointer = &(iter.value());
            } else {
                throw config::KeyNotFound(fmt::format("key:{}", key));
            }
        }
        
        try {
            return obj_pointer->get<T>();
        } catch (nlohmann::detail::type_error&) {
            throw config::ConvertFailed(
                fmt::format("key:{}, type:{}", key, typeid(T).name()));
        }
    }

    /**
     * \brief 获取指定key的值, 如果不存在返回默认值, 内部无锁.
     * \tparam T return type
     * \param key
     * \param default_value 默认值
     * \exception ConvertFailed if convert to target type failed.
     * \return key在配置文件中对应值, 如果不存在返回默认值
     */
    template <typename T>
    T getIfExists(const String& key, T default_value) {
        try {
            return get<T>(key);
        }
        catch (config::KeyNotFound&) {
            miss_map_[key] = default_value;
            return std::any_cast<T>(miss_map_.at(key));
        }
        catch (...) {
            throw;
        }
    }
private:
    JsonConfig(String str) {
        json_object_ = json::parse(str);
    }

    json json_object_;
};

class YamlConfig : public ConfigBase
{
friend detail::YamlConfigLoader;
public:
    ~YamlConfig() override {
    }


    YamlConfig(const char* filename)
        : node_{YAML::LoadFile(filename)} {
        
    }

    /**
     * \brief 获取指定key的值
     * \tparam T return type
     * \param key
     * \exception KeyNotFound if key not exists.
     * \exception ConvertFailed if convert to target type failed.
     * \return T if exists
     */
    template <typename T>
    T get(const String& key) const {
        if (auto iter = miss_map_.find(key); iter != miss_map_.end())
            return std::any_cast<T>(iter->second);
        auto keys = splitStringPiece(key, ".");

        // const YAML::Node* node = &node_;
        YAML::Node node;
        try
        {// 本来是想用下面注释的循环查找的方式的, 但是yaml-cpp的值拷贝会改变Yaml::Node的值, operator[]返回的又是值类型..., 所以只好这么做了, 不过效率应该是一样的.
            switch (keys.size()) {
                case 0:
                    throw config::KeyNotFound(fmt::format("key:{}", key));
                case 1:
                    node = node_[key];
                    break;
                case 2:
                    node = node_[String(keys[0])][String(keys[1])];
                    break;
                case 3:
                    node = node_[String(keys[0])][String(keys[1])][String(
                        keys[2])];
                    break;
                case 4:
                    node = node_[String(keys[0])][String(keys[1])][
                        String(keys[2])][String(keys[3])];
                    break;
                case 5:
                    node = node_[String(keys[0])][String(keys[1])][
                        String(keys[2])][String(keys[3])][String(keys[4])];
                    break;
                default:
                    std::cerr << "too deep\n";
                    break;
            }


            // for (auto& _key : keys) {
            //     if (node->IsNull()) {
            //         throw config::KeyNotFound(fmt::format("key:{}", key));
            //     }
            //
            //     node = &((*node)[String(_key)]);
            // }
        } catch(YAML::InvalidNode&) {
            throw config::KeyNotFound(fmt::format("key:{}", key));
        }
        if (node.IsNull()) {
            throw config::KeyNotFound(fmt::format("key:{}", key));
        }

        try {
            return node.as<T>();
        } catch (YAML::BadConversion& e) {
            throw config::ConvertFailed(
                fmt::format("key:{}, type:{}", key, typeid(T).name()));
        }
    }

    /**
     * \brief 获取指定key的值, 如果不存在返回默认值, 内部无锁.
     * \tparam T return type
     * \param key
     * \param default_value 默认值
     * \exception ConvertFailed if convert to target type failed.
     * \return key在配置文件中对应值, 如果不存在返回默认值
     */
    template <typename T>
    T getIfExists(const String& key, T default_value) {
        try {
            return get<T>(key);
        } catch (config::KeyNotFound&) {
            miss_map_[key] = default_value;
            return std::any_cast<T>(miss_map_.at(key));
        } catch (...) {
            throw;
        }
    }

private:
    YamlConfig(String str) : node_{YAML::Load(str)}{  }

    YAML::Node node_;
};

namespace detail {
    struct YamlConfigLoader
    {
        YamlConfigLoader(String str);
        YamlConfig config;
    };

    struct JsonConfigLoader
    {
        JsonConfigLoader(String str);
        JsonConfig config;
    };

}


} // namespace lon
