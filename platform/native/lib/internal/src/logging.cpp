//
// Created by scott on 5/18/20.
//

#include "estate/internal/deps/spdlog.h"

#include "estate/internal/logging.h"

namespace estate {
    std::shared_ptr<spdlog::logger> _logger{};
    std::optional<LogContext> _system_log_context;

    namespace logging {
        bool is_tracing_enabled() {
            return _logger->should_log(spdlog::level::trace);
        }
    }

    void init_logging(const LoggingConfig& config, bool overwrite) {
        if (_logger && !overwrite)
            return;

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>());

        auto logger = std::make_shared<spdlog::logger>("file_console", begin(sinks), end(sinks));

        const auto *pattern = "%^[%H:%M:%S] [%P] [%L] %v%$";

        logger->set_pattern(pattern);
        logger->set_level(config.level);

        _system_log_context.emplace(LogContext{config.system});

        _logger = logger;
    }

    std::shared_ptr<spdlog::logger> get_logger() {
        return _logger;
    }
    LogContext &get_system_log_context() {
        assert(_system_log_context.has_value());
        return _system_log_context.value();
    }
    LogContext::LogContext(std::string context) {
        std::string formatted{};
        formatted.append("[");
        formatted.append(context);
        formatted.append("] ");
        _formatted = std::move(formatted);
        _context = std::move(context);
    }
    const std::string &LogContext::get_context() const {
        assert(!_moved);
        return _context;
    }
    const std::string &LogContext::get_formatted() const {
        assert(!_moved);
        return _formatted;
    }
    LogContext::LogContext(const LogContext &other) : _formatted(other._formatted), _context(other._context) {
        assert(!other._moved);
    }
    LogContext::LogContext(LogContext &&other) : _formatted(std::move(other._formatted)), _context(std::move(other._context)) {
        other._moved = true;
    }
}
