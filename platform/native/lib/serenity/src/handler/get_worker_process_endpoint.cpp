//
// Created by scott on 4/5/20.
//

#include "estate/internal/serenity/handler/get_worker_process_endpoint.h"

namespace estate {
    GetWorkerProcessEndpointServiceProvider::GetWorkerProcessEndpointServiceProvider(BufferPoolS buffer_pool, WorkerProcessTableS worker_process_table) :
            _buffer_pool(buffer_pool), _worker_process_table(worker_process_table) {
    }

    BufferPoolS GetWorkerProcessEndpointServiceProvider::get_buffer_pool() {
        return _buffer_pool.get_service();
    }
    WorkerProcessTableS GetWorkerProcessEndpointServiceProvider::get_worker_worker_table() {
        return _worker_process_table.get_service();
    }

    UnitResultCode validate_request(const LogContext &log_context, const GetWorkerProcessEndpointRequestProto *request) {
        using Result = UnitResultCode;

        if (request->worker_id() == 0) {
            log_error(log_context, "invalid worker id");
            return Result::Error(Code::InvalidRequest);
        }

        return Result::Ok();
    }
    Buffer<GetWorkerProcessEndpointResponseProto> create_get_worker_process_endpoint_error_code_response(BufferPoolS buffer_pool, Code code) {
        fbs::Builder builder{};
        return finish_and_copy_to_buffer(builder, buffer_pool, CreateGetWorkerProcessEndpointResponseProto(
                builder,
                GetWorkerProcessEndpointErrorUnionProto::ErrorCodeResponseProto,
                CreateErrorCodeResponseProto(
                        builder, GET_CODE_VALUE(code)
                ).Union()));
    }
    Buffer<GetWorkerProcessEndpointResponseProto> create_get_worker_process_endpoint_ok_response(BufferPoolS buffer_pool,
                                                                                         const WorkerProcessEndpoint& endpoint) {
        fbs::Builder builder{};
        return finish_and_copy_to_buffer(builder, buffer_pool,
                                  CreateGetWorkerProcessEndpointResponseProto(
                                          builder,
                                          GetWorkerProcessEndpointErrorUnionProto::WorkerProcessEndpointProto,
                                          CreateWorkerProcessEndpointProto(builder,
                                                                       endpoint.setup_worker_port,
                                                                       endpoint.delete_worker_port,
                                                                       endpoint.user_port).Union()));
    }
}
