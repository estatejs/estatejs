//
// Created by scott on 4/5/20.
//
#pragma once

#include <estate/internal/server/server.h>
#include <estate/internal/processor/processor.h>
#include <estate/internal/innerspace/innerspace.h>
#include <estate/internal/processor/service_provider/script_engine_service_provider.h>
#include <estate/internal/local_config.h>
#include <estate/internal/processor/service_provider/database_service_provider.h>
#include <estate/runtime/protocol/serenity_interface_generated.h>
#include "estate/internal/serenity/worker-process-table.h"

#define SERENITY_RESPOND_ERROR_CODE(lc, code) \
            auto response_buffer = SERENITY_CREATE_ERROR_CODE_RESPONSE(code); \
            request_context->async_respond(lc, response_buffer.get_view(), std::nullopt)

#define SERENITY_WORKED_OR_RESPOND_ERROR_CODE(f) \
            { \
                auto __worked = f; \
                if (!__worked) { \
                    log_error(log_context, "Responding with error {}", get_code_name(__worked.get_error())); \
                    SERENITY_RESPOND_ERROR_CODE(log_context, __worked.get_error()); \
                    return; \
                }\
            }

#define SERENITY_GET_LOG_CONTEXT_OR_RESPOND_ERROR_CODE(req) \
    auto __log_context_r = get_log_context(req); \
    if (!__log_context_r) { \
        auto sys_log_context = get_system_log_context(); \
        log_error(sys_log_context, "Responding with error {}", get_code_name(Code::InvalidLogContext)); \
        SERENITY_RESPOND_ERROR_CODE(std::move(sys_log_context), Code::InvalidLogContext); \
        return; \
    }\
    auto log_context = __log_context_r.unwrap();

// GetWorkerProcessEndpoint
namespace estate {
    using GetWorkerProcessEndpointInnerspace = Innerspace<GetWorkerProcessEndpointRequestProto, GetWorkerProcessEndpointResponseProto>;
    using GetWorkerProcessEndpointRequestContext = GetWorkerProcessEndpointInnerspace::Server::ServerRequestContext;
    using GetWorkerProcessEndpointRequestEnvelope = GetWorkerProcessEndpointInnerspace::RequestEnvelope;

    struct GetWorkerProcessEndpointServiceProvider {
        Service<WorkerProcessTable> _worker_process_table;
        Service<BufferPool> _buffer_pool;
    public:
        GetWorkerProcessEndpointServiceProvider(BufferPoolS buffer_pool, WorkerProcessTableS worker_process_table);
        [[nodiscard]] BufferPoolS get_buffer_pool();
        [[nodiscard]] WorkerProcessTableS get_worker_worker_table();
    };
    using GetWorkerProcessEndpointServiceProviderS = std::shared_ptr<GetWorkerProcessEndpointServiceProvider>;

    struct GetWorkerProcessEndpointProcessorConfig {
        static GetWorkerProcessEndpointProcessorConfig FromRemote(const LocalConfigurationReader &reader) {
            return GetWorkerProcessEndpointProcessorConfig{};
        }
    };

    Buffer<GetWorkerProcessEndpointResponseProto> create_get_worker_process_endpoint_ok_response(BufferPoolS buffer_pool,
                                                                                         const WorkerProcessEndpoint& endpoint);
    Buffer<GetWorkerProcessEndpointResponseProto> create_get_worker_process_endpoint_error_code_response(BufferPoolS buffer_pool, Code code);

    UnitResultCode validate_request(const LogContext &log_context, const GetWorkerProcessEndpointRequestProto *request);

    template<typename TRequestContextS, typename TRequestBuffer>
    void execute(const GetWorkerProcessEndpointProcessorConfig &config,
                 GetWorkerProcessEndpointServiceProviderS service_provider,
                 TRequestBuffer &&request_buffer,
                 TRequestContextS request_context) {

#define SERENITY_CREATE_ERROR_CODE_RESPONSE(code) create_get_worker_process_endpoint_error_code_response(service_provider->get_buffer_pool(), code)

        const GetWorkerProcessEndpointRequestProto *request = request_buffer.get_payload();

        SERENITY_GET_LOG_CONTEXT_OR_RESPOND_ERROR_CODE(request);

        SERENITY_WORKED_OR_RESPOND_ERROR_CODE(validate_request(log_context, request));
        log_trace(log_context, "Validated request");

        auto buffer_pool = service_provider->get_buffer_pool();

        auto worker_process_table = service_provider->get_worker_worker_table();

        auto endpoint_r = worker_process_table->loader_get_endpoint(log_context, request->worker_id());
        if(!endpoint_r) {
            log_error(log_context, "WorkerLoader encountered an error when requesting WorkerProcess for the WorkerId {}: Error Code: {}", request->worker_id(), get_code_name(endpoint_r.get_error()));
            const auto response_buffer = create_get_worker_process_endpoint_error_code_response(buffer_pool, endpoint_r.get_error());
            request_context->async_respond(std::move(log_context), response_buffer.get_view(), std::nullopt);
        } else {
            auto endpoint = endpoint_r.unwrap();
            log_trace(log_context, "WorkerLoader received endpoint setup_worker: {}, delete_worker: {}, user_worker: {} for the WorkerId: {}",
                      endpoint.setup_worker_port, endpoint.delete_worker_port, endpoint.user_port, request->worker_id());
            const auto response_buffer = create_get_worker_process_endpoint_ok_response(buffer_pool, endpoint);
            request_context->async_respond(std::move(log_context), response_buffer.get_view(), std::nullopt);
        }
    }

    using GetWorkerProcessEndpointProcessor = Processor<
            GetWorkerProcessEndpointProcessorConfig,
            GetWorkerProcessEndpointRequestProto,
            GetWorkerProcessEndpointResponseProto,
            GetWorkerProcessEndpointServiceProvider,
            GetWorkerProcessEndpointRequestContext,
            GetWorkerProcessEndpointRequestEnvelope,
            execute>;
}

#undef SERENITY_RESPOND_ERROR_CODE
#undef SERENITY_WORKED_OR_RESPOND_ERROR_CODE
#undef SERENITY_GET_LOG_CONTEXT_OR_RESPOND_ERROR_CODE
