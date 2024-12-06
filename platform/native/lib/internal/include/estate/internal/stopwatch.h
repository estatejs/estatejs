//
// Created by scott on 1/26/20.
//
#pragma once

#include <optional>
#include <string>
#include <chrono>
#include <optional>
#include <estate/internal/logging.h>

namespace estate {
    struct Stopwatch {
        using Elapsed = std::chrono::duration<long, std::ratio<1, 1'000'000'000>>;
        Stopwatch(const LogContext& log_context);
        void reset();
        void stop();
        Elapsed elapsed();
        void log_elapsed(const std::string &name, std::optional<size_t> rps_count = std::nullopt);
        void log_elapsed_and_reset(const std::string &name);
        double elapsed_milliseconds();
        double elapsed_microseconds();
        double elapsed_nanoseconds();
    private:
        const LogContext& _log_context;
        std::chrono::steady_clock::time_point started;
        std::optional<std::chrono::steady_clock::time_point> stopped;
    };
}
