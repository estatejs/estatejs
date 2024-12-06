//
// Created by scott on 5/18/20.
//

#include <estate/internal/file_util.h>

#include "estate/internal/logging.h"

#include <sys/stat.h>
#include <fstream>

namespace estate {
    std::optional<std::string> maybe_read_whole_binary_file(const std::string& file_name) {
        std::ifstream file(file_name, std::ios::binary);
        if(!file)
            return std::nullopt;
        std::ostringstream ostrm;
        ostrm << file.rdbuf();
        return ostrm.str();
    }

    void write_whole_binary_file(const char *file_name, const char *bytes, const size_t len) {
        auto file = std::fstream(file_name, std::ios::out | std::ios::binary);
        file.write(bytes, len);
        file.close();
    }

    bool touch(const LogContext& log_context, const std::string &pathname) {
        int fd = open(pathname.c_str(), O_WRONLY | O_CREAT | O_NOCTTY | O_NONBLOCK, 0666);
        if (fd < 0) // Couldn't open that path.
        {
            log_critical(log_context, "couldn't open path when trying to touch file {0}", pathname);
            return false;
        }
        int rc = utimensat(AT_FDCWD, pathname.c_str(), nullptr, 0);
        if (rc) {
            log_critical(log_context, "couldn't utimensat path when trying to touch file {0}", pathname);
            return false;
        }
        return true;
    }
}
