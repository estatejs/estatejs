//
// Created by scott on 12/24/22.
//

#pragma once

#include <string>
#include <optional>
#include <cstdlib>

namespace estate {
    std::optional<std::string> maybe_get_environment_variable(const std::string& what);
    std::string get_required_environment_variable(const std::string& what);
}
