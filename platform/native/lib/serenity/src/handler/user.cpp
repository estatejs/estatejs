//
// Created by scott on 4/5/20.
//

#include "estate/internal/serenity/handler/user.h"

namespace estate {
    Buffer<WorkerProcessUserResponseProto> create_error_code_user_response(BufferPoolS buffer_pool, Code code, std::optional<engine::ConsoleLogS> console_log) {
        fbs::Builder outer_builder{};

        fbs::Offset<fbs::Vector<u8>> console_log_vec_off = 0;
        if (console_log.has_value()) {
            auto maybe_console_log_buffer = create_console_log_proto(buffer_pool, console_log.value());
            if (maybe_console_log_buffer.has_value()) {
                auto console_log_buffer = std::move(maybe_console_log_buffer.value());
                console_log_vec_off = outer_builder.CreateVector(console_log_buffer.as_u8(), console_log_buffer.size());
            }
        }

        fbs::Builder inner_builder{};
        inner_builder.Finish(CreateUserResponseUnionWrapperProto(
                inner_builder, UserResponseUnionProto::ErrorCodeResponseProto, CreateErrorCodeResponseProto(
                        inner_builder, GET_CODE_VALUE(code)).Union()));

        return finish_and_copy_to_buffer(outer_builder, buffer_pool, CreateWorkerProcessUserResponseProto(
                outer_builder,
                outer_builder.CreateVector(inner_builder.GetBufferPointer(), inner_builder.GetSize()),
                0,
                0,
                console_log_vec_off
        ));
    }
    Buffer<WorkerProcessUserResponseProto> create_exception_user_response(BufferPoolS buffer_pool, const engine::ScriptException &ex, std::optional<engine::ConsoleLogS> console_log) {
        fbs::Builder outer_builder{};

        fbs::Offset<fbs::Vector<u8>> console_log_vec_off = 0;
        if (console_log.has_value()) {
            auto maybe_console_log_buffer = create_console_log_proto(buffer_pool, console_log.value());
            if (maybe_console_log_buffer.has_value()) {
                auto console_log_buffer = std::move(maybe_console_log_buffer.value());
                console_log_vec_off = outer_builder.CreateVector(console_log_buffer.as_u8(), console_log_buffer.size());
            }
        }

        fbs::Builder inner_builder{};
        inner_builder.Finish(CreateUserResponseUnionWrapperProto(
                inner_builder, UserResponseUnionProto::ExceptionResponseProto, CreateExceptionResponseProto(
                        inner_builder,
                        ex.stack_trace.has_value() ? inner_builder.CreateString(ex.stack_trace.value()) : 0,
                        ex.message.has_value() ? inner_builder.CreateString(ex.message.value()) : 0
                ).Union()));

        return finish_and_copy_to_buffer(
                outer_builder, buffer_pool, CreateWorkerProcessUserResponseProto(
                        outer_builder,
                        outer_builder.CreateVector(inner_builder.GetBufferPointer(), inner_builder.GetSize()),
                        0,
                        0,
                        console_log_vec_off));
    }
}
