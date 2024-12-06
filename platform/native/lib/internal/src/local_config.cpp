//
// Originally written by Scott R. Jones.
// Copyright (c) 2020 Warpdrive Technologies, Inc. All rights reserved.
//

#include <estate/internal/local_config.h>
#include <estate/internal/deps/fmt.h>
#include <iostream>
#include <fstream>
#include <utility>
#include "estate/internal/env_util.h"

namespace estate {

    LocalConfigurationReader::LocalConfigurationReader(const nlohmann::json &json_config, std::string class_namespace) :
            _json_config(json_config), _class_namespace{std::move(class_namespace)} {
    }

    std::domain_error log_invalid_config(const std::string &missing_key) {
        const auto str = fmt::format("Missing required configuration value for key {}", missing_key);
        std::cerr << str << std::endl;
        return std::domain_error(str.c_str());
    }

    bool LocalConfigurationReader::get_bool(const std::string &key, std::optional<bool> def) const {
        const auto result = try_get_bool(key);
        if (result && result.value()) {
            return true;
        }

        if (def.has_value())
            return def.value();

        throw log_invalid_config(key);
    }
    std::string LocalConfigurationReader::get_string(const std::string &key, std::optional<std::string> def) const {
        const auto result = try_get_string(key);
        if (result)
            return result.value();

        if (def.has_value())
            return def.value();

        throw log_invalid_config(key);
    }
    u8 LocalConfigurationReader::get_u8(const std::string &key, std::optional<u8> def) const {
        const auto result = try_get_number(key);
        if (result) {
            return (u8) result.value();
        }

        if (def.has_value())
            return def.value();

        throw log_invalid_config(key);
    }
    u16 LocalConfigurationReader::get_u16(const std::string &key, std::optional<u16> def) const {
        const auto result = try_get_number(key);
        if (result) {
            return (u16) result.value();
        }

        if (def.has_value())
            return def.value();

        throw log_invalid_config(key);
    }
    u32 LocalConfigurationReader::get_u32(const std::string &key, std::optional<u32> def) const {
        const auto result = try_get_number(key);
        if (result) {
            return (u32) result.value();
        }

        if (def.has_value())
            return def.value();

        throw log_invalid_config(key);
    }
    u64 LocalConfigurationReader::get_u64(const std::string &key, std::optional<u64> def) const {
        const auto result = try_get_number(key);
        if (result) {
            return (u64) result.value();
        }

        if (def.has_value())
            return def.value();

        throw log_invalid_config(key);
    }
    i64 LocalConfigurationReader::get_i64(const std::string &key, std::optional<i64> def) const {
        const auto result = try_get_number(key);
        if (result) {
            return (i64) result.value();
        }

        if (def.has_value())
            return def.value();

        throw log_invalid_config(key);
    }

    std::optional<std::string> LocalConfigurationReader::try_get_string(const std::string &key) const {
        if (this->exists(key)) {
            return _json_config[_class_namespace][key];
        }
        return std::nullopt;
    }

    std::optional<i64> LocalConfigurationReader::try_get_number(const std::string &key) const {
        if (this->exists(key)) {
            return _json_config[_class_namespace][key];
        }
        return std::nullopt;
    }

    std::optional<bool> LocalConfigurationReader::try_get_bool(const std::string &key) const {
        if (this->exists(key)) {
            return _json_config[_class_namespace][key];
        }
        return std::nullopt;
    }

    bool LocalConfigurationReader::exists(const std::string &key) const {
        return _json_config.contains(_class_namespace) && _json_config[_class_namespace].contains(key);
    }
    std::optional<u8> LocalConfigurationReader::try_get_u8(const std::string &key) const {
        auto num = try_get_number(key);
        if(num.has_value())
            return (u8) num.value();
        return std::nullopt;
    }
    std::optional<u32> LocalConfigurationReader::try_get_u32(const std::string &key) const {
        auto num = try_get_number(key);
        if(num.has_value())
            return (u32) num.value();
        return std::nullopt;
    }
    std::optional<u16> LocalConfigurationReader::try_get_u16(const std::string &key) const {
        auto num = try_get_number(key);
        if(num.has_value())
            return (u16) num.value();
        return std::nullopt;
    }
    std::optional<u64> LocalConfigurationReader::try_get_u64(const std::string &key) const {
        auto num = try_get_number(key);
        if(num.has_value())
            return (u64) num.value();
        return std::nullopt;
    }
    std::optional<i64> LocalConfigurationReader::try_get_i64(const std::string &key) const {
        auto num = try_get_number(key);
        if(num.has_value())
            return (i64) num.value();
        return std::nullopt;
    }

    LocalConfigurationReader LocalConfiguration::create_reader(const std::string &class_namespace) {
        return LocalConfigurationReader{
                _config_json,
                class_namespace
        };
    }
    LocalConfiguration LocalConfiguration::FromFile(const std::string &config_file) {
        std::ifstream i{config_file};
        if (!i) {
            std::cerr << "Failed reading config file " << config_file << std::endl;
            throw std::domain_error("Failed reading config file");
        }

        nlohmann::json config_json;
        i >> config_json;

        return LocalConfiguration{
                config_json
        };
    }

    LocalConfiguration LocalConfiguration::FromFileInEnvironmentVariable(const std::string &env_var) {
        const std::string env_val = get_required_environment_variable(env_var);
        return FromFile(env_val);
    }

    LocalConfiguration::LocalConfiguration(nlohmann::json config_json) : _config_json(std::move(config_json)) {
    }
}
