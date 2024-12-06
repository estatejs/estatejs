//
// Created by scott on 1/11/2022.
//
#pragma once

#include <estate/internal/server/server.h>
#include <estate/internal/processor/processor.h>
#include <estate/internal/innerspace/innerspace.h>
#include <estate/internal/processor/service_provider/script_engine_service_provider.h>
#include <estate/internal/local_config.h>

#define RESPOND_ERROR_CODE(lc, code, conlog) \
    log_error(lc, "Responding with error {}", get_code_name(code)); \
    request_context->async_respond(lc, create_error_code_user_response(service_provider->get_buffer_pool(), code, conlog).get_view(), std::nullopt)
#define UNWRAP_OR_ERROR_LC(v, f, e, lc, conlog) \
    auto __##v = f; \
    if (!__##v) { \
        RESPOND_ERROR_CODE(lc, e, conlog); \
        return; \
    } \
    auto v = __##v.unwrap();
#define UNWRAP_OR_ERROR(v, f, e, conlog) UNWRAP_OR_ERROR_LC(v,f,e,log_context,conlog)
#define UNWRAP_OR_FORWARD(v, f, conlog) UNWRAP_OR_ERROR(v,f,__##v.get_error(), conlog)
#define WORKED_OR_ERROR_LC(f, e, lc, conlog)\
                {\
                    auto __worked = f;\
                    if (!__worked) {\
                        RESPOND_ERROR_CODE(lc, e, conlog);\
                        return;\
                    }\
                }
#define WORKED_OR_ERROR(f, e, conlog) WORKED_OR_ERROR_LC(f,e,log_context, conlog)
#define WORKED_OR_FORWARD(f, conlog) WORKED_OR_ERROR(f,__worked.get_error(),conlog)

namespace estate {
    using UserInnerspace = Innerspace<UserRequestProto, WorkerProcessUserResponseProto>;
    using UserServiceProvider = ScriptEngineServiceProvider<engine::IObjectRuntime>;
    using UserServiceProviderS = ScriptEngineServiceProviderS<engine::IObjectRuntime>;
    using UserRequestContext = UserInnerspace::Server::ServerRequestContext;
    using UserRequestEnvelope = UserInnerspace::RequestEnvelope;

    struct UserProcessorConfig {
        WorkerId worker_id;
        static UserProcessorConfig Create(WorkerId worker_id) {
            return UserProcessorConfig {
                worker_id
            };
        }
    };

    Buffer<WorkerProcessUserResponseProto> create_error_code_user_response(BufferPoolS buffer_pool, Code code, std::optional<engine::ConsoleLogS> console_log);

    Buffer<WorkerProcessUserResponseProto> create_exception_user_response(BufferPoolS buffer_pool, const engine::ScriptException &ex, std::optional<engine::ConsoleLogS> console_log);

    template<typename TRequestContextS, typename TRequestBuffer>
    void execute(const UserProcessorConfig &config,
                 UserServiceProviderS service_provider,
                 TRequestBuffer &&request_buffer,
                 TRequestContextS request_context) {

        const UserRequestProto *request = request_buffer.get_payload();

        UNWRAP_OR_ERROR_LC(log_context, get_log_context(request), Code::InvalidLogContext, get_system_log_context(), std::nullopt);
        log_trace(log_context, "Got the log context");

        //make sure the request's worker_id matches the worker_id this WorkerProcess was launched for.
        if(request->worker_id() != config.worker_id) {
            log_critical(log_context, "Wrong WorkerId {} for this WorkerProcess {}", request->worker_id(), config.worker_id);
            RESPOND_ERROR_CODE(log_context, Code::WorkerProcess_WrongWorkerId, std::nullopt);
            assert(false);
            return;
        }

        UNWRAP_OR_FORWARD(db, service_provider->get_database_manager()->get_database(log_context, request->worker_id(), false, std::nullopt), std::nullopt);
        log_trace(log_context, "Retrieved database");

        //Handle the request based on its type
        switch (request->request_type()) {
            case UserRequestUnionProto::GetDataRequestProto: {
                UNWRAP_OR_FORWARD(txn, db->create_transaction(log_context, request->worker_version()), std::nullopt);

                auto inner_request = request->request_as_GetDataRequestProto();
                const auto ref = make_object_reference(data::ObjectType::WORKER_OBJECT, inner_request->class_id(), PrimaryKey{inner_request->primary_key()->string_view()});

                auto call_context = std::make_shared<engine::CallContext>(log_context, txn, service_provider->get_buffer_pool(), false);

                auto reusable_builder = call_context->get_reusable_builder(true);
                UNWRAP_OR_FORWARD(data_off, data::export_data(*reusable_builder, ref, call_context->get_working_set()), std::nullopt);

                reusable_builder->Finish(CreateUserResponseUnionWrapperProto(*reusable_builder, UserResponseUnionProto::GetDataResponseProto,
                                                                             CreateGetDataResponseProto(*reusable_builder, data_off).Union()));

                fbs::Builder outer_builder{};
                const auto response_bytes_vec_off = outer_builder.CreateVector(reusable_builder->GetBufferPointer(), reusable_builder->GetSize());
                outer_builder.Finish(CreateWorkerProcessUserResponseProto(outer_builder, response_bytes_vec_off));

                BufferView<WorkerProcessUserResponseProto> response_view{outer_builder};
                request_context->async_respond(log_context, response_view, std::nullopt);
                log_info(log_context, "GetData request completed successfully");
                break;
            }
            case UserRequestUnionProto::CallServiceMethodRequestProto: {
                UNWRAP_OR_FORWARD(txn, db->create_transaction(log_context, request->worker_version()), std::nullopt);
                auto call_context = std::make_shared<engine::CallContext>(log_context, txn, service_provider->get_buffer_pool(), true);

                auto console_log = call_context->get_console_log();
                const auto inner_request = request->request_as_CallServiceMethodRequestProto();

                auto engine_result = service_provider->get_object_runtime()->call_service_method(call_context, inner_request);

                if (engine_result) {
                    auto result = engine_result.unwrap();
                    if (result.has_changes) {
                        log_trace(log_context, "Comitting the transaction");
                        WORKED_OR_FORWARD(txn->commit(), console_log);
                    }
                    request_context->async_respond(log_context, result.response.get_view(), std::nullopt);
                    log_info(log_context, "CallServiceMethod request completed successfully");
                } else {
                    auto error = engine_result.get_error();
                    if (error.is_code()) {
                        log_error(log_context, "Responding with error {}", get_code_name(error.get_code()));
                        auto response_buffer = create_error_code_user_response(service_provider->get_buffer_pool(),
                                                                               error.get_code(), console_log);
                        request_context->async_respond(log_context, response_buffer.get_view(), std::nullopt);
                    } else {
                        assert(error.is_exception());
                        log_error(log_context, "Responding with script exception");
                        request_context->async_respond(log_context, create_exception_user_response(service_provider->get_buffer_pool(),
                                                                                                   error.get_exception(), console_log).get_view(), std::nullopt);
                    }
                }
                break;
            }
            case UserRequestUnionProto::SaveDataRequestProto: {
                UNWRAP_OR_FORWARD(txn, db->create_transaction(log_context, request->worker_version()), std::nullopt);
                auto call_context = std::make_shared<engine::CallContext>(log_context, txn, service_provider->get_buffer_pool(), true);

                auto working_set = call_context->get_working_set();
                auto buffer_pool = call_context->get_buffer_pool();
                auto reusable_builder = call_context->get_reusable_builder(false);

                // Apply the inbound deltas
                const auto inner_request = request->request_as_SaveDataRequestProto();
                std::vector<data::ObjectHandleS> handles{};
                if (inner_request->data_deltas() && inner_request->data_deltas()->size() > 0) {
                    for (int i = 0; i < inner_request->data_deltas()->size(); ++i) {
                        const auto delta = inner_request->data_deltas()->Get(i);
                        UNWRAP_OR_FORWARD(maybe_handle, data::apply_inbound_delta(*reusable_builder, *delta, working_set, buffer_pool, true), std::nullopt);
                        if (maybe_handle.has_value())
                            handles.emplace_back(std::move(maybe_handle.value()));
                    }
                }

                // Get new deltas because some of the changes may be no-op
                auto deltas = data::export_deltas_from_working_set(log_context, buffer_pool, reusable_builder, working_set);

                reusable_builder->Reset();

                bool has_changes{false};
                fbs::Offset<fbs::Vector<fbs::Offset<DataDeltaBytesProto>>> delta_bytes_off{};
                if (!deltas.empty()) {
                    std::vector<fbs::Offset<DataDeltaBytesProto>> delta_bytes_vec{};
                    for (const auto &delta : deltas) {
                        auto vec = reusable_builder->CreateVector(delta.as_u8(), delta.size());
                        delta_bytes_vec.push_back(CreateDataDeltaBytesProto(*reusable_builder, vec));
                    }
                    delta_bytes_off = reusable_builder->CreateVector(delta_bytes_vec);
                    has_changes = true;
                }

                std::vector<fbs::Offset<DataHandleProto>> handle_offsets{};
                for (auto handle: handles) {
                    handle_offsets.emplace_back(CreateDataHandleProto(*reusable_builder,
                                                                            handle->get_reference()->class_id,
                                                                            handle->get_version(),
                                                                            reusable_builder->CreateString(handle->get_reference()->get_primary_key().view())
                    ));
                }

                reusable_builder->Finish(
                        CreateUserResponseUnionWrapperProto(*reusable_builder,
                                                            UserResponseUnionProto::SaveDataResponseProto,
                                                            CreateSaveDataResponseProto(*reusable_builder,
                                                                                               reusable_builder->CreateVector(handle_offsets)).Union()));

                fbs::Builder outer_builder{};
                outer_builder.Finish(CreateWorkerProcessUserResponseProto(outer_builder,
                                                                     outer_builder.CreateVector(reusable_builder->GetBufferPointer(), reusable_builder->GetSize()),
                                                                     delta_bytes_off));

                if (has_changes) {
                    log_trace(log_context, "Comitting the transaction");
                    WORKED_OR_FORWARD(txn->commit(), std::nullopt);
                }

                BufferView<WorkerProcessUserResponseProto> response_view{outer_builder};
                request_context->async_respond(log_context, response_view, std::nullopt);
                log_info(log_context, "SaveData request completed successfully");
                break;
            }
            default: {
                RESPOND_ERROR_CODE(log_context, Code::InvalidRequest, std::nullopt);
                assert(false);
                return;
            }
        }
    }

    using UserProcessor = Processor<
            UserProcessorConfig,
            UserRequestProto,
            WorkerProcessUserResponseProto,
            UserServiceProvider,
            UserRequestContext,
            UserRequestEnvelope,
            execute>;
}
