#pragma once

#include "base/macro.h"
#include "base/typedef.h"


#include <any>
#include <unordered_map>
#include <yaml-cpp/node/node.h>

namespace lon
{
// 两种方案, 如果是想用继承, 那么get函数不能是模板函数, 也就是说要么返回值是void*, 要么抽象返回值, 要么使用any(其实还是void*)
// 另一种就是采用类似contract的方式, 当然对于c++17并不能限制满足这样的约定, 需要主动满足这样的约定, 具体就是这些类采样相同的api
// 这里采用第一种方案

// 采用const指针的原因是: 如果没有程序中设置配置文件的需求的话, 直接使用const raw指针返回就不需要考虑多线程同步问题, 也不需要加锁.
// 如果有需求的话, getImpl可能需要使用any来返回shared_ptr了(多线程需要考虑不同线程掌握指针的生命周期, 可能出现空悬指针), 也需要加锁.

    class Config{
    public:
        template <typename T>
        const T* get(const String& key) const {
            return static_cast<T*>(getImpl(key));
        }

        template <typename T>
        const T* getIfExists(const String& key, T default_value) const {
            T* result = nullptr;
            try {
                result = get(key);
            } catch (...) {//TODO use NO_EXISTS EXCEPTION
                miss_map_.insert({key, default_value});
                return &(std::any_cast<T>(miss_map_[key]));
            }
        }

        virtual ~Config() = 0;
    protected:
        LON_NODISCARD virtual const void* getImpl(const String& key) const  = 0;
        std::unordered_map<String, std::any> miss_map_; // store in miss_map_ means not found at file loaded;
    };

    class JsonConfig : public Config {
    public:
        ~JsonConfig() override;
    protected:
        LON_NODISCARD const void*
        getImpl(const String& key) const override;
    private:
        void loadFromNode();
    };

    class YamlConfig : public Config {
    public:
        ~YamlConfig() override;
    protected:
        LON_NODISCARD const void*
        getImpl(const String& key) const override;
    private:
        void loadFromNode();
    private:
        YAML::Node node_;
    };
} // namespace lon
