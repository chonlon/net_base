#include "config.h"

#include <string>
#include <unordered_set>
#include <gtest/gtest.h>
#include "config/yaml_convert_def.h"
#include <fmt/ranges.h>

//部分测试结果依赖于文件, 文件一般来说不会修改
//如果文件部分测试结果错误可能是文件被修改了


#define LON_XX(config) \
        EXPECT_EQ(9900, config.get<int>("system.port")); \
        EXPECT_EQ(15, config.get<int>("system.value")); \
        { \
            std::vector<int> container{ {10, 30} }; \
            EXPECT_EQ(container, config.get<std::vector<int>>("system.int_vec")); \
        } \
        { \
            std::list<int> container{ {20, 40, 50} }; \
            EXPECT_EQ(container, config.get<std::list<int>>("system.int_list")); \
        } \
        { \
            std::set<int> container{ {30, 20, 60, 20} }; \
            EXPECT_EQ(container, config.get<std::set<int>>("system.int_set")); \
        } \
        { \
            std::unordered_set<int> container{ {30, 20, 60, 20} }; \
            EXPECT_EQ(container, \
                config.get<std::unordered_set<int>>("system.int_uset")); \
        } \
        { \
            std::map<std::string, int> container{ {"k", 30}, {"k2", 20}, {"k3", 10} }; \
            EXPECT_EQ(container, \
                config.get<decltype(container)>("system.str_int_map")); \
        } \
        { \
            std::unordered_map<std::string, int> container{ \
                {"k", 130}, {"k2", 120}, {"k3", 110} }; \
            EXPECT_EQ(container, \
                config.get<decltype(container)>("system.str_int_umap")); \
        } \
        { \
            std::map<std::string, std::vector<int>> container{ \
                {"x", {10, 20, 30}}, {"k", {30, 40, 10}} }; \
            EXPECT_EQ(container, \
                config.get<decltype(container)>("system.str_int_vec_umap")); \
        }


TEST(ConfigTest, YamlConfigFileTest) {
    lon::YamlConfig config("conf/test.yml");

    LON_XX(config)
}

TEST(ConfigTest, JsonConfigFileTest) {
    lon::JsonConfig config("conf/test.json");

    LON_XX(config)
}

TEST(ConfigTest, YamlConfigTest) {
    const char* raw_yaml =
        R"(
system:
    port: 9900
    value: 15
    int_vec:
        - 10
        - 30
    int_list: [20, 40, 50]
    int_set: [30, 20, 60, 20]
    int_uset: [30, 20, 60, 20]
    str_int_map:
        k: 30
        k2: 20
        k3: 10
    str_int_umap:
        k: 130
        k2: 120
        k3: 110
    str_int_vec_umap:
        x: [10,20,30]
        k: [30,40,10]
    )";

    lon::detail::YamlConfigLoader loader(raw_yaml);
    LON_XX(loader.config)
}

TEST(ConfigTest, JsonConfigTest) {
    const char* raw_json =
        R"(
{
    "system": {
        "port": 9900,
        "value": 15,
        "int_vec": [
            10,
            30
        ],
        "int_list": [
            20,
            40,
            50
        ],
        "int_set": [
            30,
            20,
            60,
            20
        ],
        "int_uset": [
            30,
            20,
            60,
            20
        ],
        "str_int_map": {
            "k": 30,
            "k2": 20,
            "k3": 10
        },
        "str_int_umap": {
            "k": 130,
            "k2": 120,
            "k3": 110
        },
        "str_int_vec_umap": {
            "x": [
                10,
                20,
                30
            ],
            "k": [
                30,
                40,
                10
            ]
        }
    }
}
)";

    lon::detail::JsonConfigLoader loader(raw_json);
    LON_XX(loader.config);
}

#undef LON_XX

TEST(ConfigTest, JsonConfigException) {
    const char* raw_json =
        R"({
        "happy": true,
        "pi": 3.141
    })";
    lon::detail::JsonConfigLoader loader(raw_json);
    try {
        loader.config.get<int>("not_exists_key");
        EXPECT_FALSE(true);
    } catch (lon::config::KeyNotFound&) {
        EXPECT_FALSE(false);
    }
    catch (...) {
        EXPECT_FALSE(true);
    }

    try {
        [[maybe_unused]] auto i = loader.config.get<std::map<std::string,int>>("happy");
        EXPECT_FALSE(true);
    } catch (lon::config::ConvertFailed&) {
        EXPECT_FALSE(false);
    }
    catch (...) {
        EXPECT_FALSE(true);
    }
}

TEST(ConfigTest, YamlConfigException) {
    const char* raw_yaml = R"(
        happy: true
        pi: 3.141
    )";
    lon::detail::YamlConfigLoader loader(raw_yaml);
    try {
        loader.config.get<int>("not_exists_key");
        EXPECT_FALSE(true);
    }
    catch (lon::config::KeyNotFound&) {
        EXPECT_FALSE(false);
    }
    catch (...) {
        EXPECT_FALSE(true);
    }

    try {
        [[maybe_unused]] auto i = loader.config.get<std::map<std::string, int>>("happy");
        EXPECT_FALSE(true);
    }
    catch (lon::config::ConvertFailed&) {
        EXPECT_FALSE(false);
    }
    catch (...) {
        EXPECT_FALSE(true);
    }
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
