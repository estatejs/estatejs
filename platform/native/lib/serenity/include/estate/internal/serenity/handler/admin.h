//
// Created by scott on 1/11/2022.
//
#pragma once

#include <estate/internal/server/server.h>
#include <estate/internal/processor/processor.h>
#include <estate/internal/innerspace/innerspace.h>
#include <estate/internal/processor/service_provider/script_engine_service_provider.h>
#include <estate/internal/local_config.h>
#include <estate/internal/processor/service_provider/database_service_provider.h>
#include <estate/internal/serenity/worker-process-table.h>

#define ADMIN_RESPOND_ERROR_CODE(lc, code) \
            auto response_buffer = ADMIN_CREATE_ERROR_CODE_RESPONSE(code); \
            request_context->async_respond(lc, response_buffer.get_view(), std::nullopt)

#define ADMIN_UNWRAP_OR_RESPOND_ERROR_CODE(v, f) \
        auto __##v##_r = f; \
        if (!__##v##_r) { \
            log_error(log_context, "Responding with error {}", get_code_name(__##v##_r.get_error())); \
            ADMIN_RESPOND_ERROR_CODE(log_context, __##v##_r.get_error()); \
            return; \
        }\
        auto v = __##v##_r.unwrap()

#define ADMIN_WORKED_OR_RESPOND_ERROR_CODE(f) \
            { \
                auto __worked = f; \
                if (!__worked) { \
                    log_error(log_context, "Responding with error {}", get_code_name(__worked.get_error())); \
                    ADMIN_RESPOND_ERROR_CODE(log_context, __worked.get_error()); \
                    return; \
                }\
            }

#define ADMIN_GET_LOG_CONTEXT_OR_RESPOND_ERROR_CODE(req) \
    auto __log_context_r = get_log_context(req); \
    if (!__log_context_r) { \
        auto sys_log_context = get_system_log_context(); \
        log_error(sys_log_context, "Responding with error {}", get_code_name(Code::InvalidLogContext)); \
        ADMIN_RESPOND_ERROR_CODE(std::move(sys_log_context), Code::InvalidLogContext); \
        return; \
    }\
    auto log_context = __log_context_r.unwrap();

// SetupWorker
namespace estate {
    using SetupWorkerInnerspace = Innerspace<SetupWorkerRequestProto, SetupWorkerResponseProto>;
    using SetupServiceProvider = ScriptEngineServiceProvider<engine::ISetupRuntime>;
    using SetupServiceProviderS = ScriptEngineServiceProviderS<engine::ISetupRuntime>;
    using SetupWorkerRequestContext = SetupWorkerInnerspace::Server::ServerRequestContext;
    using SetupWorkerRequestEnvelope = SetupWorkerInnerspace::RequestEnvelope;

    struct SetupWorkerProcessorConfig {
        WorkerId worker_id;
        static SetupWorkerProcessorConfig Create(WorkerId worker_id) {
            return SetupWorkerProcessorConfig{
                    worker_id
            };
        }
    };

    Buffer<SetupWorkerResponseProto> create_setup_worker_ok_response(BufferPoolS buffer_pool);
    Buffer<SetupWorkerResponseProto> create_setup_worker_error_code_response(BufferPoolS buffer_pool, Code code);
    Buffer<SetupWorkerResponseProto> create_setup_worker_exception_response(BufferPoolS buffer_pool, const engine::ScriptException &ex);

    UnitResultCode validate_request(const LogContext &log_context, const SetupWorkerRequestProto *request);

    template<typename TRequestContextS, typename TRequestBuffer>
    void execute(const SetupWorkerProcessorConfig &config,
                 SetupServiceProviderS service_provider,
                 TRequestBuffer &&request_buffer,
                 TRequestContextS request_context) {

#define ADMIN_CREATE_ERROR_CODE_RESPONSE(code) create_setup_worker_error_code_response(service_provider->get_buffer_pool(), code)

        const SetupWorkerRequestProto *request = request_buffer.get_payload();

        ADMIN_GET_LOG_CONTEXT_OR_RESPOND_ERROR_CODE(request);

        //make sure the request's worker_id matches the worker_id this WorkerProcess was launched for.
        if (request->worker_id() != config.worker_id) {
            log_critical(log_context, "Wrong WorkerId {} for this WorkerProcess {}", request->worker_id(), config.worker_id);
            ADMIN_RESPOND_ERROR_CODE(log_context, Code::WorkerProcess_WrongWorkerId);
            assert(false);
            return;
        }

        ADMIN_WORKED_OR_RESPOND_ERROR_CODE(validate_request(log_context, request));
        log_trace(log_context, "Validated request");

        //Create the setup runtime
        engine::ISetupRuntimeS runtime = service_provider->get_object_runtime();
        auto buffer_pool = service_provider->get_buffer_pool();

        //get the database
        auto database_manager = service_provider->get_database_manager();
        auto is_new = request->previous_worker_version() == 0;
        ADMIN_UNWRAP_OR_RESPOND_ERROR_CODE(database, database_manager->get_database(log_context, request->worker_id(), is_new,
                                                                                    is_new ? std::make_optional(request->worker_version())
                                                                                           : std::nullopt));
        if (is_new)
            log_trace(log_context, "Created the new database");
        else
            log_trace(log_context, "Opened the existing database");

        //create the transaction
        auto worker_version_to_open = is_new ? request->worker_version() : request->previous_worker_version();
        ADMIN_UNWRAP_OR_RESPOND_ERROR_CODE(txn, database->create_transaction(log_context, worker_version_to_open));
        log_trace(log_context, "Created the transaction");

        //perform the worker setup
        auto worked = runtime->setup(log_context, txn, request, is_new);
        if (!worked) {
            auto error = worked.get_error();
            if (error.is_code()) {
                log_error(log_context, "Responding with error {}", get_code_name(error.get_code()));
                auto response_buffer = create_setup_worker_error_code_response(service_provider->get_buffer_pool(),
                                                                             error.get_code());
                request_context->async_respond(log_context, response_buffer.get_view(), std::nullopt);
                return;
            } else {
                assert(error.is_exception());
                log_error(log_context, "Responding with script exception");
                auto response_buffer = create_setup_worker_exception_response(service_provider->get_buffer_pool(),
                                                                            error.get_exception());
                request_context->async_respond(log_context, response_buffer.get_view(), std::nullopt);
                return;
            }
        }

        //Commit the changes
        ADMIN_WORKED_OR_RESPOND_ERROR_CODE(txn->commit());
        log_trace(log_context, "Committed the changes");

        //all done
        log_trace(log_context, "Responding OK");
        {
            auto response_buffer = create_setup_worker_ok_response(service_provider->get_buffer_pool());
            request_context->async_respond(std::move(log_context), response_buffer.get_view(), std::nullopt);
        }

#undef ADMIN_CREATE_ERROR_CODE_RESPONSE
    }

    using SetupWorkerProcessor = Processor<
            SetupWorkerProcessorConfig,
            SetupWorkerRequestProto,
            SetupWorkerResponseProto,
            SetupServiceProvider,
            SetupWorkerRequestContext,
            SetupWorkerRequestEnvelope,
            execute>;
}

// DeleteWorker
namespace estate {
    using DeleteWorkerInnerspace = Innerspace<DeleteWorkerRequestProto, DeleteWorkerResponseProto>;
    using DeleteWorkerRequestContext = DeleteWorkerInnerspace::Server::ServerRequestContext;
    using DeleteWorkerRequestEnvelope = DeleteWorkerInnerspace::RequestEnvelope;

    struct DeleteWorkerProcessorConfig {
        WorkerId worker_id;
        bool shutdown_on_delete;
        static DeleteWorkerProcessorConfig Create(WorkerId worker_id, bool shutdown_on_delete) {
            return DeleteWorkerProcessorConfig {
                    worker_id,
                    shutdown_on_delete
            };
        }
        static DeleteWorkerProcessorConfig FromRemoteWithWorkerId(const LocalConfigurationReader &reader, WorkerId worker_id) {
            return DeleteWorkerProcessorConfig{
                    worker_id,
                    reader.get_bool("shutdown_on_delete")
            };
        }
    };

    struct ShutdownRequestor {
        std::function<void()> _request_shutdown;
        ShutdownRequestor(std::function<void()> request_shutdown) :
                _request_shutdown{std::move(request_shutdown)} {}
        void request_shutdown() {
            _request_shutdown();
        }
    };
    using ShutdownRequestorS = std::shared_ptr<ShutdownRequestor>;

    struct DeleteServiceProvider {
        Service<storage::DatabaseManager> _database_manager;
        Service<BufferPool> _buffer_pool;
        std::optional<Service<ShutdownRequestor>> _maybe_shutdown_requestor;
        std::optional<Service<WorkerProcessTable>> _maybe_worker_process_table;
        DeleteServiceProvider(BufferPoolS buffer_pool,
                                  storage::DatabaseManagerS database_manager,
                                  std::optional<ShutdownRequestorS> maybe_shutdown_requestor,
                                  std::optional<WorkerProcessTableS> maybe_worker_process_table) :
                _buffer_pool{std::move(buffer_pool)},
                _database_manager{std::move(database_manager)}{
            if(maybe_shutdown_requestor.has_value()) {
                _maybe_shutdown_requestor.emplace(maybe_shutdown_requestor.value());
            }
            if(maybe_worker_process_table.has_value()){
                _maybe_worker_process_table.emplace(maybe_worker_process_table.value());
            }
        }
        storage::DatabaseManagerS get_database_manager() {
            return _database_manager.get_service();
        }
        ShutdownRequestorS get_shutdown_requestor() {
            assert(_maybe_shutdown_requestor.has_value());
            return _maybe_shutdown_requestor.value().get_service();
        }
        BufferPoolS get_buffer_pool() {
            return _buffer_pool.get_service();
        }
        WorkerProcessTableS get_worker_process_table() {
            assert(_maybe_worker_process_table.has_value());
            return _maybe_worker_process_table.value().get_service();
        }
    };
    using DeleteServiceProviderS = std::shared_ptr<DeleteServiceProvider>;

    struct DeleteWorkerRequest {
        WorkerId worker_id;
        WorkerVersion worker_version;
    };

    ResultCode<DeleteWorkerRequest> parse_request(const LogContext &log_context, const DeleteWorkerRequestProto *request);

    Buffer<DeleteWorkerResponseProto> create_delete_worker_ok_response(BufferPoolS buffer_pool);
    Buffer<DeleteWorkerResponseProto> create_delete_worker_error_code_response(BufferPoolS buffer_pool, Code code);

    template<typename TRequestContextS, typename TRequestBuffer>
    void execute(const DeleteWorkerProcessorConfig &config,
                 DeleteServiceProviderS service_provider,
                 TRequestBuffer&& request_buffer,
                 TRequestContextS request_context) {

#define ADMIN_CREATE_ERROR_CODE_RESPONSE(code) create_delete_worker_error_code_response(service_provider->get_buffer_pool(), code)

        auto unparsed_request = request_buffer.get_payload();

        ADMIN_GET_LOG_CONTEXT_OR_RESPOND_ERROR_CODE(unparsed_request);

        //make sure the request's worker_id matches the worker_id this WorkerProcess was launched for.
        if (unparsed_request->worker_id() != config.worker_id) {
            log_critical(log_context, "Wrong WorkerId {} for this WorkerProcess {}", unparsed_request->worker_id(), config.worker_id);
            ADMIN_RESPOND_ERROR_CODE(log_context, Code::WorkerProcess_WrongWorkerId);
            assert(false);
            return;
        }

        // Parse the request
        ADMIN_UNWRAP_OR_RESPOND_ERROR_CODE(request, parse_request(log_context, unparsed_request));
        log_trace(log_context, "Parsed the request");

        // Get the database
        auto database_manager = service_provider->get_database_manager();
        auto worker_id = request.worker_id;
        ADMIN_UNWRAP_OR_RESPOND_ERROR_CODE(database, database_manager->get_database(log_context, worker_id, false, std::nullopt));
        log_trace(log_context, "Got the database");

        // Mark the database as deleted
        ADMIN_WORKED_OR_RESPOND_ERROR_CODE(database->mark_as_deleted(log_context));
        log_trace(log_context, "Marked the database as deleted");

        database = nullptr;

        //Close the database so it can't be opened again
        database_manager->close_database(log_context, worker_id);
        log_trace(log_context, "Closed the database");

        //All done
        log_trace(log_context, "Responding OK");
        {
            auto response_buffer = create_delete_worker_ok_response(service_provider->get_buffer_pool());
            if(config.shutdown_on_delete) {
                auto shutdown_requestor = service_provider->get_shutdown_requestor();
                auto worker_process_table = service_provider->get_worker_process_table();
                request_context->async_respond(log_context, response_buffer.get_view(), [worker_id, log_context, shutdown_requestor, worker_process_table]() {
                    worker_process_table->mark_worker_process_deleted(log_context, worker_id);
                    shutdown_requestor->request_shutdown();
                });
            } else {
                request_context->async_respond(log_context, response_buffer.get_view(), std::nullopt);
            }
        }
#undef ADMIN_CREATE_ERROR_CODE_RESPONSE
    }

    using DeleteWorkerProcessor = Processor<
            DeleteWorkerProcessorConfig,
            DeleteWorkerRequestProto,
            DeleteWorkerResponseProto,
            DeleteServiceProvider,
            DeleteWorkerRequestContext,
            DeleteWorkerRequestEnvelope,
            execute>;
}

#undef ADMIN_RESPOND_ERROR_CODE
#undef ADMIN_UNWRAP_OR_RESPOND_ERROR_CODE
#undef ADMIN_WORKED_OR_RESPOND_ERROR_CODE
#undef ADMIN_GET_LOG_CONTEXT_OR_RESPOND_ERROR_CODE
