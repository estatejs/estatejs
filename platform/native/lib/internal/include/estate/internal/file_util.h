//
// Created by scott on 5/18/20.
//

#pragma once

#include "logging.h"
#include <string>

namespace estate {
    void write_whole_binary_file(const char *file_name, const char *bytes, const size_t len);
    std::optional<std::string> maybe_read_whole_binary_file(const std::string& file_name);
    bool touch(const LogContext& log_context, const std::string &pathname);
}
