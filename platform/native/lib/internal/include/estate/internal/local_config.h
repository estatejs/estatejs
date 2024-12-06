//
// Originally written by Scott R. Jones.
// Copyright (c) 2020 Warpdrive Technologies, Inc. All rights reserved.
//

#pragma once

#include <string>
#include <chrono>
#include <estate/runtime/numeric_types.h>
#include <estate/internal/deps/boost.h>
#include <estate/runtime/result.h>
#include <nlohmann/json.hpp>

namespace estate {
    struct LocalConfigurationReader {
        LocalConfigurationReader(const nlohmann::json& json_config, std::string class_namespace);
        [[nodiscard]] bool get_bool(const std::string &key, std::optional<bool> def = std::nullopt) const;
        [[nodiscard]] std::string get_string(const std::string &key, std::optional<std::string> def = std::nullopt) const;
        [[nodiscard]] u8 get_u8(const std::string &key, std::optional<u8> def = std::nullopt) const;
        [[nodiscard]] u16 get_u16(const std::string &key, std::optional<u16> def = std::nullopt) const;
        [[nodiscard]] u32 get_u32(const std::string &key, std::optional<u32> def = std::nullopt) const;
        [[nodiscard]] u64 get_u64(const std::string &key, std::optional<u64> def = std::nullopt) const;
        [[nodiscard]] i64 get_i64(const std::string &key, std::optional<i64> def = std::nullopt) const;
        [[nodiscard]] std::optional<u8> try_get_u8(const std::string &key) const;
        [[nodiscard]] std::optional<u16> try_get_u16(const std::string &key) const;
        [[nodiscard]] std::optional<u32> try_get_u32(const std::string &key) const;
        [[nodiscard]] std::optional<u64> try_get_u64(const std::string &key) const;
        [[nodiscard]] std::optional<i64> try_get_i64(const std::string &key) const;
        [[nodiscard]] std::optional<std::string> try_get_string(const std::string &key) const;
        [[nodiscard]] std::optional<bool> try_get_bool(const std::string &key) const;
        [[nodiscard]] std::optional<i64> try_get_number(const std::string &key) const;
        [[nodiscard]] bool exists(const std::string &key) const;
    private:
        const std::string _class_namespace;
        const nlohmann::json& _json_config;
    };

    class LocalConfiguration {
    public:
        LocalConfigurationReader create_reader(const std::string &class_namespace);
        static LocalConfiguration FromFileInEnvironmentVariable(const std::string& env_var);
        static LocalConfiguration FromFile(const std::string& path);
    private:
        explicit LocalConfiguration(nlohmann::json config_file);
        const nlohmann::json _config_json;
    };
}
