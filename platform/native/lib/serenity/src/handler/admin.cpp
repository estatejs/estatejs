//
// Created by scott on 4/5/20.
//

#include "estate/internal/serenity/handler/admin.h"

// SetupWorker
namespace estate {
    UnitResultCode validate_request(const LogContext &log_context, const SetupWorkerRequestProto *request) {
        using Result = UnitResultCode;

        if (request->worker_id() == 0) {
            log_error(log_context, "invalid worker id");
            return Result::Error(Code::InvalidRequest);
        }
        if (request->worker_version() == 0) {
            log_error(log_context, "invalid worker version");
            return Result::Error(Code::InvalidRequest);
        }
        if (!request->worker_code() || !request->worker_code()->size()) {
            log_error(log_context, "invalid worker code");
            return Result::Error(Code::InvalidRequest);
        }
        if (!request->worker_index() ||
            !request->worker_index_nested_root()->file_names() ||
            request->worker_index_nested_root()->file_names()->size() == 0) {
            log_error(log_context, "invalid worker index");
            return Result::Error(Code::InvalidRequest);
        }
        if(request->worker_code()->size() != request->worker_index_nested_root()->file_names()->size()) {
            log_error(log_context, "invalid worker code or index");
            return Result::Error(Code::InvalidRequest);
        }
        if(request->previous_worker_version() != 0 && request->previous_worker_version() >= request->worker_version()) {
            log_error(log_context, "invalid previous worker version");
            return Result::Error(Code::InvalidRequest);
        }
        std::set<FileId> file_ids{};
        for(int i = 0; i < request->worker_index_nested_root()->file_names()->size(); ++i) {
            //file name id must be valid
            const auto file_id = request->worker_index_nested_root()->file_names()->Get(i)->file_name_id();
            if(file_id > request->worker_code()->size()) {
                log_error(log_context, "invalid file id: out of range");
                return Result::Error(Code::InvalidRequest);
            }
            const auto &[_,inserted] = file_ids.insert(file_id);
            if(!inserted)
            {
                log_error(log_context, "duplicate file id");
                return Result::Error(Code::InvalidRequest);
            }
        }

        return Result::Ok();
    }
    Buffer<SetupWorkerResponseProto> create_setup_worker_exception_response(BufferPoolS buffer_pool, const engine::ScriptException &ex) {
        fbs::Builder builder{};
        return finish_and_copy_to_buffer(builder, buffer_pool, CreateSetupWorkerResponseProto(
                builder,
                SetupWorkerErrorUnionProto::ExceptionResponseProto,
                CreateExceptionResponseProto(
                        builder,
                        ex.stack_trace.has_value() ? builder.CreateString(ex.stack_trace.value()) : 0,
                        ex.message.has_value() ? builder.CreateString(ex.message.value()) : 0
                ).Union()));
    }
    Buffer<SetupWorkerResponseProto> create_setup_worker_error_code_response(BufferPoolS buffer_pool, Code code) {
        fbs::Builder builder{};
        return finish_and_copy_to_buffer(builder, buffer_pool, CreateSetupWorkerResponseProto(
                builder,
                SetupWorkerErrorUnionProto::ErrorCodeResponseProto,
                CreateErrorCodeResponseProto(
                        builder, GET_CODE_VALUE(code)
                ).Union()));
    }
    Buffer<SetupWorkerResponseProto> create_setup_worker_ok_response(BufferPoolS buffer_pool) {
        fbs::Builder builder{};
        return finish_and_copy_to_buffer(builder, buffer_pool, CreateSetupWorkerResponseProto(builder));
    }
    Buffer<DeleteWorkerResponseProto> create_delete_worker_error_code_response(BufferPoolS buffer_pool, Code code) {
        fbs::Builder builder{};
        return finish_and_copy_to_buffer(builder, buffer_pool, CreateDeleteWorkerResponseProto(
                builder,
                CreateErrorCodeResponseProto(
                        builder, GET_CODE_VALUE(code)
                )));
    }
    Buffer<DeleteWorkerResponseProto> create_delete_worker_ok_response(BufferPoolS buffer_pool) {
        fbs::Builder builder{};
        return finish_and_copy_to_buffer(builder, buffer_pool, CreateDeleteWorkerResponseProto(builder));
    }
}

// DeleteWorker
namespace estate {
    ResultCode<DeleteWorkerRequest> parse_request(const LogContext &log_context, const DeleteWorkerRequestProto *request) {
        using Result = ResultCode<DeleteWorkerRequest>;

        if (request->worker_id() == 0) {
            log_error(log_context, "invalid worker id");
            return Result::Error(Code::InvalidRequest);
        }
        if (request->worker_version() == 0) {
            log_error(log_context, "invalid worker version");
            return Result::Error(Code::InvalidRequest);
        }

        auto worker_id = request->worker_id();
        auto worker_version = request->worker_version();

        DeleteWorkerRequest parsed_request{
                worker_id,
                worker_version
        };

        return Result::Ok(std::move(parsed_request));
    }
}
