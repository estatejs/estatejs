//
// Created by scott on 5/18/20.
//

#pragma once

#include <estate/runtime/limits.h>
#include <estate/runtime/result.h>
#include <estate/runtime/model_types.h>

#include "estate/internal/deps/spdlog.h"
#include "local_config.h"

namespace estate {
    ///////////////////////////////////////////////////////////////////////////
    // Logging
    struct LoggingConfig {
        std::string system;
        spdlog::level::level_enum level;
        static LoggingConfig FromRemoteWithWorkerId(const LocalConfigurationReader &reader, WorkerId worker_id) {
            return LoggingConfig{
                    fmt::format(reader.get_string("system"), worker_id),
                    spdlog::level::from_str(reader.get_string("level"))
            };
        }
        static LoggingConfig FromRemote(const LocalConfigurationReader &reader) {
            return LoggingConfig{
                    reader.get_string("system"),
                    spdlog::level::from_str(reader.get_string("level"))
            };
        }
    };

    class LogContext {
        bool _moved{false};
        std::string _formatted;
        std::string _context;
    public:
        explicit LogContext(std::string context);
        LogContext(const LogContext &other);
        LogContext(LogContext &&other);
        ~LogContext() = default;
        const std::string &get_context() const;
        const std::string &get_formatted() const;
    };

    namespace logging {
        bool is_tracing_enabled();
    }

    LogContext &get_system_log_context();
    std::shared_ptr<spdlog::logger> get_logger();
    void init_logging(const LoggingConfig &config, bool overwrite);

    template<typename S, typename... Args>
    void _spdlog_log(const std::optional<LogContext> &maybe_context, spdlog::level::level_enum level, const S &fmt, const Args &... args) {
        auto logger = get_logger();
        if (logger) {
            std::string msg = get_system_log_context().get_formatted();
            if(maybe_context.has_value())
                msg += maybe_context.value().get_formatted();
            msg += fmt::format(fmt, args...);
            logger->log(level, msg);
        }
    }

    template<typename T>
    void _spdlog_log(const std::optional<LogContext> &maybe_context, spdlog::level::level_enum level, const T &msg) {
        auto logger = get_logger();
        if (logger) {
            std::string msg_ = get_system_log_context().get_formatted();
            if(maybe_context.has_value())
                msg_ += maybe_context.value().get_formatted();
            msg_ += msg;
            logger->log(level, msg_);
        }
    }

    template<typename S, typename... Args>
    void sys_log_trace(const S &fmt, const Args &... args) {
        _spdlog_log(std::nullopt, spdlog::level::trace, fmt, args...);
    }

    template<typename... Args>
    void sys_log_debug(const std::string &fmt, const Args &... args) {
        _spdlog_log(std::nullopt, spdlog::level::debug, fmt, args...);
    }

    template<typename... Args>
    void sys_log_info(const std::string &fmt, const Args &... args) {
        _spdlog_log(std::nullopt, spdlog::level::info, fmt, args...);
    }

    template<typename... Args>
    void sys_log_warn(const std::string &fmt, const Args &... args) {
        _spdlog_log(std::nullopt, spdlog::level::warn, fmt, args...);
    }

    template<typename... Args>
    void sys_log_error(const std::string &fmt, const Args &... args) {
        _spdlog_log(std::nullopt, spdlog::level::err, fmt, args...);
    }

    template<typename... Args>
    void sys_log_critical(const std::string &fmt, const Args &... args) {
        _spdlog_log(std::nullopt, spdlog::level::critical, fmt, args...);
    }

    template<typename T>
    void sys_log_trace(const T &msg) {
        _spdlog_log(std::nullopt, spdlog::level::trace, msg);
    }

    template<typename T>
    void sys_log_debug(const T &msg) {
        _spdlog_log(std::nullopt, spdlog::level::debug, msg);
    }

    template<typename T>
    void sys_log_info(const T &msg) {
        _spdlog_log(std::nullopt, spdlog::level::info, msg);
    }

    template<typename T>
    void sys_log_warn(const T &msg) {
        _spdlog_log(std::nullopt, spdlog::level::warn, msg);
    }

    template<typename T>
    void sys_log_error(const T &msg) {
        _spdlog_log(std::nullopt, spdlog::level::err, msg);
    }

    template<typename T>
    void sys_log_critical(const T &msg) {
        _spdlog_log(std::nullopt, spdlog::level::critical, msg);
    }

    template<typename S, typename... Args>
    void log_trace(const LogContext &context, const S &fmt, const Args &... args) {
        _spdlog_log(std::make_optional(context), spdlog::level::trace, fmt, args...);
    }

    template<typename... Args>
    void log_debug(const LogContext &context, const std::string &fmt, const Args &... args) {
        _spdlog_log(context, spdlog::level::debug, fmt, args...);
    }

    template<typename... Args>
    void log_info(const LogContext &context, const std::string &fmt, const Args &... args) {
        _spdlog_log(context, spdlog::level::info, fmt, args...);
    }

    template<typename... Args>
    void log_warn(const LogContext &context, const std::string &fmt, const Args &... args) {
        _spdlog_log(context, spdlog::level::warn, fmt, args...);
    }

    template<typename... Args>
    void log_error(const LogContext &context, const std::string &fmt, const Args &... args) {
        _spdlog_log(context, spdlog::level::err, fmt, args...);
    }

    template<typename... Args>
    void log_critical(const LogContext &context, const std::string &fmt, const Args &... args) {
        _spdlog_log(context, spdlog::level::critical, fmt, args...);
    }

    template<typename T>
    void log_trace(const LogContext &context, const T &msg) {
        _spdlog_log(context, spdlog::level::trace, msg);
    }

    template<typename T>
    void log_debug(const LogContext &context, const T &msg) {
        _spdlog_log(context, spdlog::level::debug, msg);
    }

    template<typename T>
    void log_info(const LogContext &context, const T &msg) {
        _spdlog_log(context, spdlog::level::info, msg);
    }

    template<typename T>
    void log_warn(const LogContext &context, const T &msg) {
        _spdlog_log(context, spdlog::level::warn, msg);
    }

    template<typename T>
    void log_error(const LogContext &context, const T &msg) {
        _spdlog_log(context, spdlog::level::err, msg);
    }

    template<typename T>
    void log_critical(const LogContext &context, const T &msg) {
        _spdlog_log(context, spdlog::level::critical, msg);
    }

    template<typename TProto>
    Result <LogContext> get_log_context(const TProto *proto) {
        using Result = Result<LogContext>;
        if(!proto) {
            sys_log_error("Invalid log context: the proto was empty.");
            return Result::Error();
        }

        if(!proto->log_context()) {
            sys_log_error("Invalid log context: the log_context method returned nothing");
            return Result::Error();
        }

        const auto sz = proto->log_context()->size();
        if(sz != ESTATE_LOG_CONTEXT_LENGTH) {
            sys_log_error("Invalid log context: the string '{}' was the wrong length {}", proto->log_context()->str(), sz);
            return Result::Error();
        }

        return Result::Ok(LogContext{proto->log_context()->str()});
    }
}
