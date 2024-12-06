//
// Created by scott on 1/26/20.
//

#include "estate/internal/stopwatch.h"
#include "estate/internal/logging.h"

namespace estate {
    Stopwatch::Stopwatch(const LogContext& log_context) : _log_context{log_context}, started(std::chrono::steady_clock::now()), stopped(std::nullopt) {
    }
    void Stopwatch::reset() {
        started = std::chrono::steady_clock::now();
        stopped.reset();
    }
    void Stopwatch::stop() {
        stopped = std::chrono::steady_clock::now();
    }
    Stopwatch::Elapsed Stopwatch::elapsed() {
        if (stopped.has_value())
            return stopped.value() - started;
        else
            return std::chrono::steady_clock::now() - started;
    }
    double Stopwatch::elapsed_milliseconds() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed()).count();
    }
    double Stopwatch::elapsed_microseconds() {
        return std::chrono::duration_cast<std::chrono::seconds>(elapsed()).count();
    }
    double Stopwatch::elapsed_nanoseconds() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed()).count();
    }
    void Stopwatch::log_elapsed_and_reset(const std::string &name) {
        log_elapsed(name);
        reset();
    }
    void Stopwatch::log_elapsed(const std::string &name, std::optional<size_t> rps_count) {
        static const auto PREFIX = "[TIMING]";
        auto elapsed = this->elapsed();
        long duration = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        long rps  = 1000000 / ((duration / (double) rps_count.value_or(1)));
        if (elapsed < std::chrono::milliseconds{1}) {
            if (elapsed < std::chrono::microseconds{1}) {
                if (rps_count.has_value())
                    log_trace(_log_context, "{} ({}) took <1μs {}r/s", PREFIX, name, rps);
                else
                    log_trace(_log_context, "{} ({}) took <1μs", PREFIX, name);
            } else {
                if (rps_count.has_value())
                    log_trace(_log_context, "{} ({}) took {}μs {}r/s", PREFIX, name, duration, rps);
                else
                    log_trace(_log_context, "{} ({}) took {}μs", PREFIX, name, duration);
            }
        } else {
            if (rps_count.has_value())
                log_trace(_log_context, "{} ({}) took {:.3f}ms {}r/s", PREFIX, name, (double) duration / 1000, rps);
            else
                log_trace(_log_context, "{} ({}) took {:.3f}ms", PREFIX, name, (double) duration / 1000);
        }
    }
}
