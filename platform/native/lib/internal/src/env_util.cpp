//
// Created by scott on 12/24/22.
//

#include <string>
#include <optional>
#include "estate/internal/deps/fmt.h"
#include "estate/internal/logging.h"

namespace estate {
    std::optional <std::string> maybe_get_environment_variable(const std::string& what) {
        auto env_str = std::getenv(what.c_str());
        if (!env_str)
            return std::nullopt;
        return std::string(env_str);
    }
    std::string get_required_environment_variable(const std::string& what) {
        auto r = maybe_get_environment_variable(what);
        if(!r.has_value()) {
            const auto msg = fmt::format("missing environment variable {}", what);
            sys_log_critical(msg);
            throw std::domain_error(msg);
        }
        return r.value();
    }
}