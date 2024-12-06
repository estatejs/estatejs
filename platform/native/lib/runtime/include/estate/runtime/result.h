//
// Created by scott on 4/14/20.
//
#pragma once

#include "code.h"

#include <optional>
#include <variant>
#include <stdexcept>

namespace estate {
#define ASSIGN_OR_RETURN(v, f) \
    auto __##v##_r = f; \
    if(!__##v##_r) \
        return Result::Error(__##v##_r.get_error()); \
    v = std::move(__##v##_r.unwrap())

#define UNWRAP_OR_RETURN(v, f) \
    auto __##v##_r = f; \
    if(!__##v##_r) \
        return Result::Error(__##v##_r.get_error()); \
    auto v = std::move(__##v##_r.unwrap())

#define WORKED_OR_RETURN(f) \
    { \
        auto __worked_r = f; \
        if(!__worked_r) \
            return Result::Error(__worked_r.get_error()); \
    }

    template<typename R>
    class Result {
        explicit Result(std::optional<R> result) : _result(std::move(result)) {}
        std::optional<R> _result;
    public:
        [[nodiscard]] static Result<R> Ok(R result) {
            return Result(std::move(std::optional<R>{std::forward<R>(result)}));
        }
        [[nodiscard]] static Result<R> Error() {
            return Result(std::move(std::optional<R>{}));
        }
        explicit operator bool() const {
            return _result.has_value();
        }
        [[nodiscard]] R unwrap() {
            if (!_result)
                throw std::domain_error("unwrapped empty value");

            return std::move(_result).value();
        }
    };

    template<typename C = Code>
    class _UnitResultCode {
        explicit _UnitResultCode(std::optional<C> error)
                : _error(error) {}
        const std::optional<C> _error;
    public:
        [[nodiscard]] static _UnitResultCode Ok() {
            return _UnitResultCode(std::nullopt);
        }
        [[nodiscard]] static _UnitResultCode Error(C error) {
            return _UnitResultCode(error);
        }
        explicit operator bool() const {
            return !_error.has_value();
        }
        [[nodiscard]] C get_error() const {
            if(!_error.has_value()) {
                assert(false);
                throw std::domain_error("Invalid: error retrieved when no error present");
            }
            return std::move(_error.value());
        }
    };
    using UnitResultCode = _UnitResultCode<Code>;

    template<typename R, typename C = Code>
    class ResultCode {
        explicit ResultCode(std::variant<R,C> result)
                : _result(std::move(result)) {}
        std::variant<R,C> _result;
        [[nodiscard]] constexpr bool is_error() const {
            return _result.index() == 1;
        }
        [[nodiscard]] constexpr bool is_result() const {
            return _result.index() == 0;
        }
    public:
        [[nodiscard]] static ResultCode<R, C> Ok(R result) {
            return ResultCode(std::move(result));
        }
        [[nodiscard]] static ResultCode<R, C> Error(C error) {
            return ResultCode(std::move(error));
        }
        explicit operator bool() const {
            return is_result();
        }
        [[nodiscard]] C get_error() {
            if(!is_error()){
                assert(false);
                throw std::domain_error("Invalid: error retrieved when result present");
            }
            return std::move(std::get<1>(std::move(_result)));
        }
        [[nodiscard]] R unwrap() {
            if(!is_result()){
                assert(false);
                throw std::domain_error("Invalid: result retrieved when error present");
            }
            return std::move(std::get<0>(std::move(_result)));
        }
    };
}
