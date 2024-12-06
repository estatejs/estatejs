//
// Created by scott on 5/18/20.
//

#include "estate/internal/river/handler.h"

#include <estate/internal/logging.h>

#include <algorithm>

#define RESPOND_SYS_ERROR(e) \
{ \
    const auto &sys_log_context = get_system_log_context(); \
    auto buffer = create_error(service_provider, e); \
    log_error(sys_log_context, "Responding with error {}", get_code_name(e)); \
    request_context->async_respond(sys_log_context, buffer.get_view()); \
    return; \
}
#define RESPOND_ERROR(e) \
{ \
    auto buffer = create_error(service_provider, e); \
    log_error(log_context, "Responding with error {}", get_code_name(e)); \
    request_context->async_respond(log_context, buffer.get_view()); \
    return; \
}

namespace estate {
    class Validator { ;
        const WorkerId _authorized_worker_id;
        const RiverProcessorConfig &_config;
        std::optional<LogContext> _maybe_log_context;
        const LogContext &get_log_context() {
            assert(_maybe_log_context.has_value());
            return _maybe_log_context.value();
        }
    public:
        Validator(const RiverProcessorConfig &config, const WorkerId authorized_worker_id) :
                _config{config}, _authorized_worker_id{authorized_worker_id} {}
    private:
        [[nodiscard]] inline UnitResultCode validate_class_id(const ClassId class_id) {
            using Result = UnitResultCode;
            if (class_id < ESTATE_MIN_CLASS_ID) {
                log_warn(get_log_context(), "Invalid class id");
                return Result::Error(Code::Validator_InvalidRequest);
            }
            return Result::Ok();
        }
        [[nodiscard]] inline UnitResultCode validate_method_id(const MethodId method_id) {
            using Result = UnitResultCode;
            if (method_id < ESTATE_MIN_METHOD_ID) {
                log_warn(get_log_context(), "Invalid method id");
                return Result::Error(Code::Validator_InvalidRequest);
            }
            return Result::Ok();
        }
        [[nodiscard]] inline UnitResultCode validate_primary_key(const fbs::String *pk) {
            using Result = UnitResultCode;
            if (pk == nullptr) {
                log_warn(get_log_context(), "Invalid primary key");
                return Result::Error(Code::Validator_InvalidRequest);
            } else if (pk->size() > _config.max_primary_key_length) {
                log_warn(get_log_context(), "Primary key too large");
                return Result::Error(Code::Validator_PrimaryKeyTooLarge);
            } else if (pk->size() < _config.min_primary_key_length) {
                log_warn(get_log_context(), "Primary key too small");
                return Result::Error(Code::Validator_PrimaryKeyTooSmall);
            }
            return Result::Ok();
        }
        [[nodiscard]] inline UnitResultCode validate_value(const ValueProto *value) {
            using Result = UnitResultCode;

            if (!value || !value->value()) {
                log_warn(get_log_context(), "Invalid value: null");
                return Result::Error(Code::Validator_InvalidRequest);
            }

            switch (value->value_type()) {
                case ValueUnionProto::StringValueProto: {
                    if (!value->value_as_StringValueProto()->value()) {
                        log_warn(get_log_context(), "Invalid string value: null");
                        return Result::Error(Code::Validator_InvalidRequest);
                    }
                    break;
                }
                case ValueUnionProto::ObjectValueProto: {
                    const auto ps = value->value_as_ObjectValueProto()->properties();
                    if (ps && ps->size() > 0) {
                        for (int i = 0; i < ps->size(); ++i) {
                            WORKED_OR_RETURN(validate_property(ps->Get(i)))
                        }
                    }
                    break;
                }
                case ValueUnionProto::ArrayValueProto: {
                    const auto items = value->value_as_ArrayValueProto()->items();
                    if (items && items->size() > 0) {
                        for (int i = 0; i < items->size(); ++i) {
                            WORKED_OR_RETURN(validate_value(items->Get(i)->value()));
                        }
                    }
                    break;
                }
                case ValueUnionProto::DataReferenceValueProto: {
                    const auto ref = value->value_as_DataReferenceValueProto();
                    WORKED_OR_RETURN(validate_primary_key(ref->primary_key()));
                    WORKED_OR_RETURN(validate_class_id(ref->class_id()));
                    break;
                }
                case ValueUnionProto::ServiceReferenceValueProto: {
                    const auto ref = value->value_as_ServiceReferenceValueProto();
                    WORKED_OR_RETURN(validate_primary_key(ref->primary_key()));
                    WORKED_OR_RETURN(validate_class_id(ref->class_id()));
                    break;
                }
                case ValueUnionProto::MapValueProto: {
                    const auto items = value->value_as_MapValueProto()->items();
                    if (items && items->size() > 0) {
                        for (int i = 0; i < items->size(); ++i) {
                            const auto item = items->Get(i);
                            if (!item) {
                                log_warn(get_log_context(), "Invalid map item: null");
                                return Result::Error(Code::Validator_InvalidRequest);
                            }
                            WORKED_OR_RETURN(validate_value(item->value()));
                            WORKED_OR_RETURN(validate_value(item->key()));
                        }
                    }
                    break;
                }
                case ValueUnionProto::SetValueProto: {
                    const auto items = value->value_as_SetValueProto()->items();
                    if (items && items->size() > 0) {
                        for (int i = 0; i < items->size(); ++i) {
                            WORKED_OR_RETURN(validate_value(items->Get(i)));
                        }
                    }
                    break;
                }
                case ValueUnionProto::UndefinedValueProto:
                case ValueUnionProto::NullValueProto:
                case ValueUnionProto::BooleanValueProto:
                case ValueUnionProto::NumberValueProto:
                case ValueUnionProto::DateValueProto:
                    break;
                default: {
                    log_warn(get_log_context(), "Invalid argument type");
                    return Result::Error(Code::Validator_InvalidRequest);
                }
            }
            return Result::Ok();
        }
        [[nodiscard]] inline UnitResultCode validate_property(const PropertyProto *prop) {
            using Result = UnitResultCode;
            if (!prop) {
                log_warn(get_log_context(), "Invalid property: null");
                return Result::Error(Code::Validator_InvalidRequest);
            }
            if (!prop->name() || prop->name()->size() == 0) {
                log_warn(get_log_context(), "Invalid object argument property name: null");
                return Result::Error(Code::Validator_InvalidRequest);
            }
            return validate_value(prop->value());
        }
        [[nodiscard]] inline UnitResultCode validate_property(const NestedPropertyProto *prop) {
            using Result = UnitResultCode;
            if (!prop) {
                log_warn(get_log_context(), "Invalid property: null");
                return Result::Error(Code::Validator_InvalidRequest);
            }
            if (!prop->name() || prop->name()->size() == 0) {
                log_warn(get_log_context(), "Invalid object argument property name: null");
                return Result::Error(Code::Validator_InvalidRequest);
            }
            return validate_value(prop->value_bytes_nested_root());
        }
    public:
        [[nodiscard]] inline ResultCode<LogContext> validate_user_request(const UserRequestProto *user_request) {
            using Result = ResultCode<LogContext>;

            if (!user_request || !user_request->request()) {
                auto sys_log_context = get_system_log_context();
                log_warn(sys_log_context, "Invalid request: null");
                return Result::Error(Code::Validator_InvalidRequest);
            }

            if (user_request->protocol_version() != ESTATE_RIVER_PROTOCOL_VERSION) {
                auto sys_log_context = get_system_log_context();
                log_warn(sys_log_context, "Invalid request: client must upgrade");
                return Result::Error(Code::Validator_ClientMustUpgrade);
            }

            if (!user_request->log_context() || user_request->log_context()->size() != ESTATE_LOG_CONTEXT_LENGTH) {
                auto sys_log_context = get_system_log_context();
                log_warn(sys_log_context, "Invalid request: invalid log context");
                return Result::Error(Code::Validator_InvalidRequest);
            }

            _maybe_log_context.emplace(user_request->log_context()->str());

            if (user_request->worker_id() < ESTATE_MIN_WORKER_ID) {
                log_warn(get_log_context(), "Invalid worker id");
                return Result::Error(Code::Validator_InvalidRequest);
            }
            if (user_request->worker_id() != _authorized_worker_id) {
                log_warn(get_log_context(), "Forbidden worker id");
                return Result::Error(Code::Validator_Forbidden);
            }
            if (user_request->worker_version() < ESTATE_MIN_WORKER_VERSION) {
                log_warn(get_log_context(), "Invalid worker version");
                return Result::Error(Code::Validator_InvalidRequest);
            }

            switch (user_request->request_type()) {
                case UserRequestUnionProto::CallServiceMethodRequestProto: {
                    const auto request = user_request->request_as_CallServiceMethodRequestProto();
                    WORKED_OR_RETURN(validate_primary_key(request->primary_key()));
                    WORKED_OR_RETURN(validate_class_id(request->class_id()));
                    WORKED_OR_RETURN(validate_method_id(request->method_id()));
                    if (request->arguments() && request->arguments()->size() > 0) {
                        for (int i = 0; i < request->arguments()->size(); ++i) {
                            WORKED_OR_RETURN(validate_value(request->arguments()->Get(i)));
                        }
                    }
                    if (request->referenced_data_deltas() && request->referenced_data_deltas()->size() > 0) {
                        for (int i = 0; i < request->referenced_data_deltas()->size(); ++i) {
                            auto delta = request->referenced_data_deltas()->Get(i);
                            if (!delta) {
                                log_warn(get_log_context(), "Invalid referenced delta");
                                return Result::Error(Code::Validator_InvalidRequest);
                            }
                            WORKED_OR_RETURN(validate_primary_key(delta->primary_key()));
                            WORKED_OR_RETURN(validate_class_id(delta->class_id()));
                            if (delta->properties() && delta->properties()->size() > 0) {
                                for (int j = 0; j < delta->properties()->size(); ++j) {
                                    WORKED_OR_RETURN(validate_property(delta->properties()->Get(j)));
                                }
                            }
                            if (delta->deleted_properties() && delta->deleted_properties()->size() > 0) {
                                for (int j = 0; j < delta->deleted_properties()->size(); ++j) {
                                    auto p = delta->deleted_properties()->Get(j);
                                    if (!p || p->size() == 0) {
                                        log_warn(get_log_context(), "Invalid deleted property: null");
                                        return Result::Error(Code::Validator_InvalidRequest);
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
                case UserRequestUnionProto::GetDataRequestProto: {
                    const auto request = user_request->request_as_GetDataRequestProto();
                    WORKED_OR_RETURN(validate_primary_key(request->primary_key()));
                    WORKED_OR_RETURN(validate_class_id(request->class_id()));
                    break;
                }
                case UserRequestUnionProto::SaveDataRequestProto: {
                    const auto request = user_request->request_as_SaveDataRequestProto();
                    if (!request->data_deltas() || request->data_deltas()->size() == 0) {
                        log_warn(get_log_context(), "Invalid request: no worker object deltas");
                        return Result::Error(Code::Validator_InvalidRequest);
                    }
                    for (int i = 0; i < request->data_deltas()->size(); ++i) {
                        auto delta = request->data_deltas()->Get(i);
                        if (!delta) {
                            log_warn(get_log_context(), "Invalid delta");
                            return Result::Error(Code::Validator_InvalidRequest);
                        }
                        WORKED_OR_RETURN(validate_primary_key(delta->primary_key()));
                        WORKED_OR_RETURN(validate_class_id(delta->class_id()));
                        if (delta->properties() && delta->properties()->size() > 0) {
                            for (int j = 0; j < delta->properties()->size(); ++j) {
                                WORKED_OR_RETURN(validate_property(delta->properties()->Get(j)));
                            }
                        }
                        if (delta->deleted_properties() && delta->deleted_properties()->size() > 0) {
                            for (int j = 0; j < delta->deleted_properties()->size(); ++j) {
                                auto p = delta->deleted_properties()->Get(j);
                                if (!p || p->size() == 0) {
                                    log_warn(get_log_context(), "Invalid deleted property: null");
                                    return Result::Error(Code::Validator_InvalidRequest);
                                }
                            }
                        }
                    }
                    break;
                }
                case UserRequestUnionProto::SubscribeMessageRequestProto: {
                    const auto request = user_request->request_as_SubscribeMessageRequestProto();
                    WORKED_OR_RETURN(validate_class_id(request->class_id()));
                    if (request->source_type() == WorkerReferenceUnionProto::DataReferenceValueProto) {
                        const auto &ref = request->source_as_DataReferenceValueProto();
                        WORKED_OR_RETURN(validate_class_id(ref->class_id()));
                        WORKED_OR_RETURN(validate_primary_key(ref->primary_key()));
                    } else {
                        const auto &ref = request->source_as_ServiceReferenceValueProto();
                        WORKED_OR_RETURN(validate_class_id(ref->class_id()));
                        WORKED_OR_RETURN(validate_primary_key(ref->primary_key()));
                    }
                    break;
                }
                case UserRequestUnionProto::UnsubscribeMessageRequestProto: {
                    const auto request = user_request->request_as_UnsubscribeMessageRequestProto();
                    WORKED_OR_RETURN(validate_class_id(request->class_id()));
                    if (request->source_type() == WorkerReferenceUnionProto::DataReferenceValueProto) {
                        const auto &ref = request->source_as_DataReferenceValueProto();
                        WORKED_OR_RETURN(validate_class_id(ref->class_id()));
                        WORKED_OR_RETURN(validate_primary_key(ref->primary_key()));
                    } else {
                        const auto &ref = request->source_as_ServiceReferenceValueProto();
                        WORKED_OR_RETURN(validate_class_id(ref->class_id()));
                        WORKED_OR_RETURN(validate_primary_key(ref->primary_key()));
                    }
                    break;
                }
                case UserRequestUnionProto::SubscribeDataUpdatesRequestProto: {
                    const auto request = user_request->request_as_SubscribeDataUpdatesRequestProto();
                    if (!request->references() || request->references()->size() == 0) {
                        log_warn(get_log_context(), "Invalid request: no references");
                        return Result::Error(Code::Validator_InvalidRequest);
                    }
                    for (int i = 0; i < request->references()->size(); ++i) {
                        auto r = request->references()->Get(i);
                        if (!r) {
                            log_warn(get_log_context(), "Invalid reference: null");
                            return Result::Error(Code::Validator_InvalidRequest);
                        }
                        WORKED_OR_RETURN(validate_class_id(r->class_id()));
                        WORKED_OR_RETURN(validate_primary_key(r->primary_key()));
                    }
                    break;
                }
                case UserRequestUnionProto::UnsubscribeDataUpdatesRequestProto: {
                    const auto request = user_request->request_as_UnsubscribeDataUpdatesRequestProto();
                    if (!request->references() || request->references()->size() == 0) {
                        log_warn(get_log_context(), "Invalid request: no references");
                        return Result::Error(Code::Validator_InvalidRequest);
                    }
                    for (int i = 0; i < request->references()->size(); ++i) {
                        auto r = request->references()->Get(i);
                        if (!r) {
                            log_warn(get_log_context(), "Invalid reference: null");
                            return Result::Error(Code::Validator_InvalidRequest);
                        }
                        WORKED_OR_RETURN(validate_class_id(r->class_id()));
                        WORKED_OR_RETURN(validate_primary_key(r->primary_key()));
                    }
                    break;
                }
                default:
                    log_warn(get_log_context(), "invalid request type");
                    return Result::Error(Code::Validator_InvalidRequest);
            }

            return Result::Ok(std::move(_maybe_log_context.value()));
        }
    };

    RiverServiceProvider::RiverServiceProvider(BufferPoolS buffer_pool,
                                               outerspace::SubscriptionManagerS subscription_manager,
                                               innerspace::InnerspaceClientS innerspace_client) :
            _buffer_pool{buffer_pool},
            _object_update_manager{subscription_manager},
            _innerspace_client{innerspace_client} {
    }
    BufferPoolS RiverServiceProvider::get_buffer_pool() {
        return _buffer_pool.get_service();
    }
    outerspace::SubscriptionManagerS RiverServiceProvider::get_subscription_manager() {
        return _object_update_manager.get_service();
    }
    innerspace::InnerspaceClientS RiverServiceProvider::get_innerspace_client() {
        return _innerspace_client.get_service();
    }

    template<typename T, typename B>
    bool validate_flatbuffer(B &buffer) {
        auto verifier = flatbuffers::Verifier(buffer.as_u8(), buffer.size());
        return verifier.VerifyBuffer<T>(nullptr);
    }

    inline auto create_error(RiverServiceProviderS service_provider, Code code) {
        fbs::Builder outer_builder{};
        fbs::Builder inner_builder{};
        inner_builder.Finish(CreateUserResponseUnionWrapperProto(inner_builder,
                                                                 UserResponseUnionProto::ErrorCodeResponseProto,
                                                                 CreateErrorCodeResponseProto(inner_builder,
                                                                                              (u16) code).Union()));
        return finish_and_copy_to_buffer(outer_builder,
                                         service_provider->get_buffer_pool(),
                                         CreateRiverUserResponseProto(outer_builder,
                                                                      CreateUserResponseUnionWrapperBytesProto(outer_builder,
                                                                                                               outer_builder.CreateVector(
                                                                                                                       inner_builder.GetBufferPointer(),
                                                                                                                       inner_builder.GetSize()))));
    }

    inline Buffer<RiverUserResponseProto> create_river_user_response(RiverServiceProviderS service_provider,
                                                                     const BufferView<UserResponseUnionWrapperProto> response,
                                                                     std::optional<std::vector<Buffer<UserMessageUnionWrapperProto>>> maybe_messages,
                                                                     std::optional<const BufferView<ConsoleLogProto>> maybe_conlog) {
        fbs::Builder builder{};

        fbs::Offset<ConsoleLogBytesProto> conlog_off{};
        if (maybe_conlog.has_value()) {
            conlog_off = CreateConsoleLogBytesProto(builder, builder.CreateVector(maybe_conlog->as_u8(), maybe_conlog->size()));
        }

        fbs::Offset<fbs::Vector<fbs::Offset<UserMessageUnionWrapperBytesProto>>> messages_off{};
        if (maybe_messages.has_value()) {
            std::vector<fbs::Offset<UserMessageUnionWrapperBytesProto>> message_wrappers{};
            for (auto &message: maybe_messages.value()) {
                message_wrappers.push_back(CreateUserMessageUnionWrapperBytesProto(builder, builder.CreateVector(message.as_u8(), message.size())));
            }
            messages_off = builder.CreateVector(message_wrappers);
        }

        return finish_and_copy_to_buffer(builder,
                                         service_provider->get_buffer_pool(),
                                         CreateRiverUserResponseProto(builder,
                                                                      CreateUserResponseUnionWrapperBytesProto(builder,
                                                                                                               builder.CreateVector(response.as_u8(),
                                                                                                                                    response.size())),
                                                                      messages_off,
                                                                      conlog_off));
    }

    using MaybeDeltasSent = std::optional<std::unordered_map<outerspace::SessionHandle, std::unordered_set<const DataDeltaBytesProto *>>>;
    using MaybeRequestorEvents = std::optional<std::vector<Buffer<UserMessageUnionWrapperProto>>>;
    [[nodiscard]] inline std::tuple<MaybeDeltasSent, MaybeRequestorEvents>
    broadcast_events(const LogContext &log_context,
                     WorkerId worker_id,
                     WorkerVersion worker_version,
                     RequestContextS request_context,
                     outerspace::SubscriptionManagerS subscription_manager,
                     BufferPoolS buffer_pool,
                     const fbs::Vector<fbs::Offset<MessageBytesProto>> &event_bytes_vec,
                     std::optional<const fbs::Vector<fbs::Offset<DataDeltaBytesProto>> *> maybe_delta_bytes_vec) {

        const auto requestor = request_context->get_session_handle();

        // Organize the deltas so their lookup is quick
        using ObjectVersionMap = std::map<ObjectVersion, const DataDeltaBytesProto *, std::greater<ObjectVersion>>;
        using PrimaryKeyMap = std::unordered_map<std::string_view, ObjectVersionMap>;
        using ClassIdMap = std::unordered_map<ClassId, PrimaryKeyMap>;
        std::optional<ClassIdMap> maybe_delta_map{};
        if (maybe_delta_bytes_vec.has_value()) {
            maybe_delta_map.emplace();
            auto &delta_map = maybe_delta_map.value();
            auto delta_bytes_vec = std::move(maybe_delta_bytes_vec.value());
            for (int i = 0; i < delta_bytes_vec->size(); ++i) {
                const auto delta_bytes = delta_bytes_vec->Get(i);
                const auto delta = delta_bytes->bytes_nested_root();

                auto class_id = delta->class_id();
                auto primary_key = delta->primary_key()->string_view();
                auto object_version = delta->object_version();

                const auto cls_it = delta_map.find(class_id);
                if (cls_it == delta_map.end()) {
                    delta_map.emplace(class_id, PrimaryKeyMap({{primary_key, ObjectVersionMap({{object_version, delta_bytes}})}}));
                } else {
                    auto &pk_map = cls_it->second;
                    const auto pk_it = pk_map.find(primary_key);
                    if (pk_it == pk_map.end()) {
                        pk_map.emplace(primary_key, ObjectVersionMap({{object_version, delta_bytes}}));
                    } else {
                        auto &ov_map = pk_it->second;
                        const auto ov_it = ov_map.find(object_version);
                        if (ov_it == ov_map.end()) {
                            const auto[_, inserted] = ov_map.insert(std::make_pair(object_version, delta_bytes));
                            assert(inserted); //should always be inserted
                        } else {
                            assert(false); //duplicates shouldn't happen
                        }
                    }
                }
            }
        }

        // Get the events each subscriber needs and match the deltas to those events.
        using DeltaBytesSet = std::unordered_set<const DataDeltaBytesProto *>;
        std::unordered_map<const MessageBytesProto *, DeltaBytesSet> event_deltas{};
        std::unordered_multimap<outerspace::SessionHandle, const MessageBytesProto *> subscriber_events{};
        for (int i = 0; i < event_bytes_vec.size(); ++i) {
            const auto event_bytes = event_bytes_vec.Get(i);
            const auto event = event_bytes->bytes_nested_root();
            auto sub = outerspace::MessageSubscription::CreateFrom(event);
            auto maybe_subscribers = subscription_manager->maybe_get_subscribers(worker_id, sub);
            if (!maybe_subscribers.has_value())
                continue;

            if (maybe_delta_map.has_value() && event->referenced_data_handles() && event->referenced_data_handles()->size() > 0) {
                const auto &delta_map = maybe_delta_map.value();
                for (int j = 0; j < event->referenced_data_handles()->size(); ++j) {
                    auto handle = event->referenced_data_handles()->Get(j);

                    const auto cls_it = delta_map.find(handle->class_id());
                    if (cls_it == delta_map.end())
                        continue;

                    const auto &pk_map = cls_it->second;
                    const auto pk_it = pk_map.find(handle->primary_key()->string_view());
                    if (pk_it == pk_map.end())
                        continue;

                    const auto &ov_map = pk_it->second;
                    auto ov_it = ov_map.find(handle->object_version());

                    //grab the delta the handle points to and all the older deltas for the same object
                    while (ov_it != ov_map.end()) {
                        const auto object_version = ov_it->first;
                        const auto delta_bytes = ov_it->second;
                        const auto event_it = event_deltas.find(event_bytes);
                        if (event_it == event_deltas.end()) {
                            event_deltas.emplace(std::make_pair(event_bytes, DeltaBytesSet({delta_bytes})));
                        } else {
                            const auto[_, inserted] = event_it->second.insert(delta_bytes);
                            assert(inserted); //shouldn't have duplicates
                        }
                        ++ov_it;
                    }
                }
            }

            for (auto subscriber: maybe_subscribers.value()) {
                subscriber_events.insert(std::make_pair(subscriber, event_bytes));
            }
        }

        // Create and send the events
        MaybeRequestorEvents maybe_requestor_events{};
        MaybeDeltasSent maybe_deltas_sent{};
        fbs::Builder outer_builder{};
        fbs::Builder inner_builder{};
        for (const auto[subscriber, event_bytes]: subscriber_events) {
            outer_builder.Reset();
            inner_builder.Reset();

            fbs::Offset<fbs::Vector<fbs::Offset<DataDeltaBytesProto>>> delta_bytes_off{};

            const auto delta_it = event_deltas.find(event_bytes);
            if (delta_it != event_deltas.end()) {
                std::vector<fbs::Offset<DataDeltaBytesProto>> delta_bytes_vec{};
                auto delta_bytes_list = delta_it->second;
                for (const auto delta_bytes: delta_bytes_list) {
                    delta_bytes_vec.push_back(
                            CreateDataDeltaBytesProto(inner_builder,
                                                            inner_builder.CreateVector(delta_bytes->bytes()->Data(), delta_bytes->bytes()->size())));
                }

                delta_bytes_off = inner_builder.CreateVector(delta_bytes_vec);
                if (!maybe_deltas_sent.has_value()) {
                    maybe_deltas_sent.emplace();
                }
                maybe_deltas_sent->emplace(std::make_pair(subscriber, std::move(delta_bytes_list)));
            }

            const auto event_message_off = CreateMessageMessageProto(inner_builder,
                                                                       CreateMessageBytesProto(inner_builder,
                                                                                                 inner_builder.CreateVector(
                                                                                                         event_bytes->bytes()->Data(),
                                                                                                         event_bytes->bytes()->size())),
                                                                       delta_bytes_off).Union();

            auto inner_message = finish_and_copy_to_buffer(inner_builder,
                                                           buffer_pool,
                                                           CreateUserMessageUnionWrapperProto(inner_builder,
                                                                                              UserMessageUnionProto::MessageMessageProto,
                                                                                              event_message_off));

            const auto wrapper_off = CreateUserMessageUnionWrapperBytesProto(outer_builder,
                                                                             outer_builder.CreateVector(inner_message.as_u8(),
                                                                                                        inner_message.size()));

            if (requestor == subscriber) {
                //don't actually send the message to the requestor, it's returned so it can be sent in the response
                if (!maybe_requestor_events.has_value()) {
                    maybe_requestor_events.emplace();
                }
                maybe_requestor_events->push_back(std::move(inner_message));
            } else {
                //broadcast to everyone else
                const auto message = finish_and_copy_to_buffer(outer_builder,
                                                               buffer_pool,
                                                               CreateRiverUserMessageProto(outer_builder,
                                                                                           outer_builder.CreateString(log_context.get_context()),
                                                                                           worker_id,
                                                                                           worker_version,
                                                                                           wrapper_off));
                request_context->async_send_to_session(log_context, subscriber, message.get_view());
            }
        }

        return {std::move(maybe_deltas_sent), std::move(maybe_requestor_events)};
    }

    using MaybeRequestorObjectUpdate = std::optional<Buffer<UserMessageUnionWrapperProto>>;
    [[nodiscard]] inline MaybeRequestorObjectUpdate
    broadcast_object_updates(const LogContext &log_context,
                             WorkerId worker_id,
                             WorkerVersion worker_version,
                             RequestContextS request_context,
                             outerspace::SubscriptionManagerS subscription_manager,
                             BufferPoolS buffer_pool,
                             MaybeDeltasSent maybe_deltas_sent,
                             const fbs::Vector<fbs::Offset<DataDeltaBytesProto>> &delta_bytes_vec) {
        const auto requestor = request_context->get_session_handle();

        std::unordered_map<outerspace::SessionHandle, std::vector<const DataDeltaBytesProto *>> subscribers_deltas{};
        for (int i = 0; i < delta_bytes_vec.size(); ++i) {
            const auto delta_bytes = delta_bytes_vec.Get(i);
            const auto delta = delta_bytes->bytes_nested_root();

            std::unordered_set<outerspace::SessionHandle> targets{(requestor)}; //the requestor gets all deltas
            auto maybe_subscribers = subscription_manager->maybe_get_subscribers(worker_id, outerspace::DataUpdateSubscription{
                    delta->class_id(),
                    delta->primary_key()->str()
            });

            if (maybe_subscribers.has_value()) {
                targets.reserve(targets.size() + maybe_subscribers->size());
                for (auto subscriber: maybe_subscribers.value()) {
                    targets.insert(std::move(subscriber));
                }
            }

            for (auto subscriber: targets) {
                if (maybe_deltas_sent.has_value()) {
                    const auto &deltas_sent = maybe_deltas_sent.value();
                    const auto it = deltas_sent.find(subscriber);
                    if (it != deltas_sent.end() && it->second.contains(delta_bytes)) {
                        continue; //don't need to send this delta to this subscriber because I've already sent it as part of an event.
                    }
                }
                const auto it = subscribers_deltas.find(subscriber);
                if (it != subscribers_deltas.end()) {
                    it->second.push_back(delta_bytes);
                } else {
                    subscribers_deltas.insert(std::make_pair(subscriber, std::vector<const DataDeltaBytesProto *>({delta_bytes})));
                }
            }
        }

        MaybeRequestorObjectUpdate requestor_object_update{};
        fbs::Builder inner_builder{};
        fbs::Builder outer_builder{};
        for (const auto[subscriber, delta_bytes_list]: subscribers_deltas) {
            inner_builder.Reset();
            outer_builder.Reset();

            std::vector<fbs::Offset<DataDeltaBytesProto>> inner_delta_bytes_vec{};
            for (const auto delta_bytes: delta_bytes_list) {
                inner_delta_bytes_vec.push_back(
                        CreateDataDeltaBytesProto(inner_builder,
                                                        inner_builder.CreateVector(delta_bytes->bytes()->Data(), delta_bytes->bytes()->size())));
            }

            auto inner_message = finish_and_copy_to_buffer(inner_builder,
                                                           buffer_pool, CreateUserMessageUnionWrapperProto(inner_builder,
                                                                                                           UserMessageUnionProto::DataUpdateMessageProto,
                                                                                                           CreateDataUpdateMessageProto(
                                                                                                                   inner_builder,
                                                                                                                   inner_builder.CreateVector(
                                                                                                                           inner_delta_bytes_vec)).Union()));

            if (subscriber == requestor) {
                assert(!requestor_object_update.has_value());
                requestor_object_update.emplace(std::move(inner_message));
            } else {
                const auto message = finish_and_copy_to_buffer(outer_builder,
                                                               buffer_pool, CreateRiverUserMessageProto(outer_builder,
                                                                                                        outer_builder.CreateString(
                                                                                                                log_context.get_context()),
                                                                                                        worker_id,
                                                                                                        worker_version,
                                                                                                        CreateUserMessageUnionWrapperBytesProto(
                                                                                                                outer_builder,
                                                                                                                outer_builder.CreateVector(
                                                                                                                        inner_message.as_u8(),
                                                                                                                        inner_message.size()))));
                request_context->async_send_to_session(log_context, subscriber, message.get_view());
            }
        }
        return std::move(requestor_object_update);
    }

    using UserInnerspaceResponseEnvelope = Innerspace<UserRequestProto, WorkerProcessUserResponseProto>::ResponseEnvelope;

    void handle_get_data(const RiverProcessorConfig &config, RiverServiceProviderS service_provider,
                                const LogContext &log_context, Buffer<UserRequestProto> request_buffer,
                                RequestContextS request_context) {

        auto client = service_provider->get_innerspace_client();

        const auto worker_id = request_buffer->worker_id();

        client->async_send(log_context, worker_id, std::move(request_buffer),
                           [log_context, request_context, service_provider](
                                   ResultCode<UserInnerspaceResponseEnvelope> envelope_r) mutable {
                               if (!envelope_r) {
                                   RESPOND_ERROR(envelope_r.get_error());
                               }
                               auto envelope = envelope_r.unwrap();
                               const auto &response = *envelope.get_payload();
                               fbs::Builder builder{};
                               auto buffer = create_river_user_response(
                                       service_provider,
                                       BufferView<UserResponseUnionWrapperProto>{*response.response()},
                                       std::nullopt,
                                       std::nullopt
                               );
                               request_context->async_respond(log_context, buffer.get_view());
                           });
    }

    void handle_subscribe_data_updates(const RiverProcessorConfig &config, RiverServiceProviderS service_provider,
                                               const LogContext &log_context, Buffer<UserRequestProto> request_buffer,
                                               RequestContextS request_context) {
        auto subscription_manager = service_provider->get_subscription_manager();
        const auto worker_id = request_context->authorized_worker_id();
        const auto session_handle = request_context->get_session_handle();
        const auto request = request_buffer->request_as_SubscribeDataUpdatesRequestProto();

        for (int i = 0; i < request->references()->size(); ++i) {
            const auto ref = request->references()->Get(i);
            subscription_manager->subscribe(
                    worker_id,
                    session_handle,
                    outerspace::DataUpdateSubscription{
                            ref->class_id(),
                            ref->primary_key()->str()
                    });
            log_debug(log_context, "Session {} subscribed to object updates from WorkerId {} ClassId {} PK {}",
                      session_handle, worker_id, ref->class_id(), ref->primary_key()->string_view());
        }

        fbs::Builder inner_builder{};
        inner_builder.Finish(CreateUserResponseUnionWrapperProto(inner_builder, UserResponseUnionProto::SubscribeDataUpdatesResponseProto,
                                                                 CreateSubscribeDataUpdatesResponseProto(inner_builder).Union()));
        const auto response = create_river_user_response(
                service_provider,
                BufferView<UserResponseUnionWrapperProto>{inner_builder},
                std::nullopt,
                std::nullopt
        );
        request_context->async_respond(log_context, response.get_view());
    }

    void handle_unsubscribe_data_updates(const RiverProcessorConfig &config, RiverServiceProviderS service_provider,
                                                 const LogContext &log_context, Buffer<UserRequestProto> request_buffer,
                                                 RequestContextS request_context) {
        auto subscription_manager = service_provider->get_subscription_manager();
        const auto worker_id = request_context->authorized_worker_id();
        const auto session_handle = request_context->get_session_handle();
        const auto request = request_buffer->request_as_UnsubscribeDataUpdatesRequestProto();

        for (int i = 0; i < request->references()->size(); ++i) {
            const auto ref = request->references()->Get(i);
            subscription_manager->unsubscribe(
                    worker_id,
                    session_handle,
                    outerspace::DataUpdateSubscription{
                            ref->class_id(),
                            ref->primary_key()->str()
                    });
            log_debug(log_context, "Session {} unsubscribed from object updates made to WorkerId {} ClassId {} PK {}",
                      session_handle, worker_id, ref->class_id(), ref->primary_key()->string_view());
        }

        fbs::Builder inner_builder{};
        inner_builder.Finish(CreateUserResponseUnionWrapperProto(inner_builder, UserResponseUnionProto::UnsubscribeDataUpdatesResponseProto,
                                                                 CreateUnsubscribeDataUpdatesResponseProto(inner_builder).Union()));
        const auto response = create_river_user_response(
                service_provider,
                BufferView<UserResponseUnionWrapperProto>{inner_builder},
                std::nullopt,
                std::nullopt
        );

        request_context->async_respond(log_context, response.get_view());
    }

    void handle_subscribe_message(const RiverProcessorConfig &config, RiverServiceProviderS service_provider,
                                     const LogContext &log_context, Buffer<UserRequestProto> request_buffer,
                                     RequestContextS request_context) {

        auto subscription_manager = service_provider->get_subscription_manager();
        const auto worker_id = request_context->authorized_worker_id();
        const auto session_handle = request_context->get_session_handle();
        const auto request = request_buffer->request_as_SubscribeMessageRequestProto();

        subscription_manager->subscribe(worker_id, session_handle, outerspace::MessageSubscription::CreateFrom(request));

        fbs::Builder inner_builder{};
        inner_builder.Finish(CreateUserResponseUnionWrapperProto(inner_builder, UserResponseUnionProto::SubscribeMessageResponseProto,
                                                                 CreateSubscribeMessageResponseProto(inner_builder).Union()));
        const auto response = create_river_user_response(
                service_provider,
                BufferView<UserResponseUnionWrapperProto>{inner_builder},
                std::nullopt,
                std::nullopt
        );

        request_context->async_respond(log_context, response.get_view());
    }

    void handle_unsubscribe_message(const RiverProcessorConfig &config, RiverServiceProviderS service_provider,
                                       const LogContext &log_context, Buffer<UserRequestProto> request_buffer,
                                       RequestContextS request_context) {
        auto subscription_manager = service_provider->get_subscription_manager();
        const auto worker_id = request_context->authorized_worker_id();
        const auto session_handle = request_context->get_session_handle();
        const auto request = request_buffer->request_as_UnsubscribeMessageRequestProto();

        subscription_manager->unsubscribe(worker_id, session_handle, outerspace::MessageSubscription::CreateFrom(request));

        fbs::Builder inner_builder{};
        inner_builder.Finish(CreateUserResponseUnionWrapperProto(inner_builder, UserResponseUnionProto::UnsubscribeMessageResponseProto,
                                                                 CreateUnsubscribeMessageResponseProto(inner_builder).Union()));
        const auto response = create_river_user_response(
                service_provider,
                BufferView<UserResponseUnionWrapperProto>{inner_builder},
                std::nullopt,
                std::nullopt
        );
        request_context->async_respond(log_context, response.get_view());
    }

    void handle_save_data(const RiverProcessorConfig &config, RiverServiceProviderS service_provider,
                                  const LogContext &log_context, Buffer<UserRequestProto> request_buffer,
                                  RequestContextS request_context) {
        auto client = service_provider->get_innerspace_client();

        const auto worker_id = request_buffer->worker_id();
        const auto worker_version = request_buffer->worker_version();

        client->async_send(log_context, worker_id, std::move(request_buffer),
                           [log_context, request_context, service_provider, worker_id, worker_version](
                                   ResultCode<UserInnerspaceResponseEnvelope> envelope_r) mutable {
                               if (!envelope_r) {
                                   RESPOND_ERROR(envelope_r.get_error());
                               }
                               auto envelope = envelope_r.unwrap();
                               const auto &response = *envelope.get_payload();
                               if (response.response_nested_root()->value_type() ==
                                   UserResponseUnionProto::SaveDataResponseProto &&
                                   response.deltas() && response.deltas()->size() > 0) {
                                   auto maybe_requestor_object_update_message = broadcast_object_updates(log_context,
                                                                                                         worker_id,
                                                                                                         worker_version,
                                                                                                         request_context,
                                                                                                         service_provider->get_subscription_manager(),
                                                                                                         service_provider->get_buffer_pool(),
                                                                                                         std::nullopt,
                                                                                                         *response.deltas());

                                   std::vector<Buffer<UserMessageUnionWrapperProto>> messages{};
                                   if (maybe_requestor_object_update_message.has_value())
                                       messages.push_back(std::move(maybe_requestor_object_update_message.value()));
                                   const auto buffer = create_river_user_response(
                                           service_provider,
                                           BufferView<UserResponseUnionWrapperProto>{*response.response()},
                                           std::move(messages),
                                           std::nullopt
                                   );
                                   request_context->async_respond(log_context, buffer.get_view());
                               } else {
                                   const auto buffer = create_river_user_response(
                                           service_provider,
                                           BufferView<UserResponseUnionWrapperProto>{*response.response()},
                                           std::nullopt,
                                           std::nullopt
                                   );
                                   request_context->async_respond(log_context, buffer.get_view());
                               }
                           });
    }

    void handle_call_service_method(const RiverProcessorConfig &config, RiverServiceProviderS service_provider,
                                         const LogContext &log_context, Buffer<UserRequestProto> request_buffer,
                                         RequestContextS request_context) {

        auto client = service_provider->get_innerspace_client();

        const auto worker_id = request_buffer->worker_id();
        const auto worker_version = request_buffer->worker_version();

        client->async_send(log_context, worker_id, std::move(request_buffer),
                           [
                                   log_context,
                                   request_context,
                                   service_provider,
                                   worker_id,
                                   worker_version
                           ](ResultCode<UserInnerspaceResponseEnvelope> envelope_r) mutable {
                               if (!envelope_r) {
                                   RESPOND_ERROR(envelope_r.get_error());
                               }
                               auto envelope = envelope_r.unwrap();
                               const auto &serenity_response = *envelope.get_payload();
                               std::optional<const BufferView<ConsoleLogProto>> maybe_conlog{};
                               if (serenity_response.console_log()) {
                                   maybe_conlog.emplace(BufferView<ConsoleLogProto>{*serenity_response.console_log()});
                               }

                               std::vector<Buffer<UserMessageUnionWrapperProto>> requestor_messages{};
                               if (serenity_response.response_nested_root()->value_type() ==
                                   UserResponseUnionProto::CallServiceMethodResponseProto) {
                                   std::optional<const fbs::Vector<fbs::Offset<MessageBytesProto>> *> maybe_events{};

                                   if (serenity_response.events() && serenity_response.events()->size() > 0) {
                                       maybe_events.emplace(serenity_response.events());
                                   }
                                   std::optional<const fbs::Vector<fbs::Offset<DataDeltaBytesProto>> *> maybe_deltas{};
                                   if (serenity_response.deltas() && serenity_response.deltas()->size() > 0) {
                                       maybe_deltas.emplace(serenity_response.deltas());
                                   }
                                   if (maybe_deltas.has_value() | maybe_events.has_value()) {
                                       auto subscription_manager = service_provider->get_subscription_manager();
                                       auto buffer_pool = service_provider->get_buffer_pool();
                                       if (maybe_events.has_value()) {
                                           auto[maybe_deltas_sent, maybe_requestor_event_messages] = broadcast_events(log_context,
                                                                                                                      worker_id,
                                                                                                                      worker_version,
                                                                                                                      request_context,
                                                                                                                      subscription_manager,
                                                                                                                      buffer_pool,
                                                                                                                      *maybe_events.value(),
                                                                                                                      maybe_deltas);
                                           if (maybe_requestor_event_messages.has_value()) {
                                               requestor_messages.reserve(maybe_requestor_event_messages->size());
                                               std::move(maybe_requestor_event_messages->begin(), maybe_requestor_event_messages->end(),
                                                         std::back_inserter(requestor_messages));
                                           }

                                           if (maybe_deltas.has_value()) {
                                               auto maybe_requestor_object_update_message = broadcast_object_updates(log_context,
                                                                                                                     worker_id,
                                                                                                                     worker_version,
                                                                                                                     request_context,
                                                                                                                     subscription_manager,
                                                                                                                     buffer_pool,
                                                                                                                     maybe_deltas_sent,
                                                                                                                     *maybe_deltas.value());
                                               if (maybe_requestor_object_update_message.has_value())
                                                   requestor_messages.push_back(
                                                           std::move(maybe_requestor_object_update_message.value()));
                                           }
                                       } else if (maybe_deltas.has_value()) {
                                           auto maybe_requestor_object_update_message = broadcast_object_updates(log_context,
                                                                                                                 worker_id,
                                                                                                                 worker_version,
                                                                                                                 request_context,
                                                                                                                 subscription_manager,
                                                                                                                 buffer_pool,
                                                                                                                 std::nullopt,
                                                                                                                 *maybe_deltas.value());
                                           if (maybe_requestor_object_update_message.has_value())
                                               requestor_messages.push_back(std::move(maybe_requestor_object_update_message.value()));
                                       }
                                   }
                               }
                               const auto response = create_river_user_response(service_provider,
                                                                                BufferView<UserResponseUnionWrapperProto>{
                                                                                        *serenity_response.response()},
                                                                                requestor_messages.empty() ? std::nullopt
                                                                                                           : std::make_optional(
                                                                                        std::move(requestor_messages)),
                                                                                maybe_conlog
                               );
                               request_context->async_respond(log_context, response.get_view());
                           });
    }

    void execute(const RiverProcessorConfig &config,
                 RiverServiceProviderS service_provider,
                 Buffer<UserRequestProto> &&request_buffer,
                 RequestContextS request_context) {

        if (!validate_flatbuffer<UserRequestProto>(request_buffer)) {
            RESPOND_SYS_ERROR(Code::Validator_InvalidFlatbuffer);
        }

        const auto request = request_buffer.get_flatbuffer();

        Validator validator{config, request_context->authorized_worker_id()};

        auto log_context_r = validator.validate_user_request(request);
        if (!log_context_r) {
            RESPOND_SYS_ERROR(log_context_r.get_error());
        }
        auto log_context{log_context_r.unwrap()};

        switch (request->request_type()) {
            case UserRequestUnionProto::GetDataRequestProto:
                handle_get_data(config, service_provider, log_context, std::move(request_buffer), request_context);
                break;
            case UserRequestUnionProto::SubscribeDataUpdatesRequestProto:
                handle_subscribe_data_updates(config, service_provider, log_context, std::move(request_buffer), request_context);
                break;
            case UserRequestUnionProto::UnsubscribeDataUpdatesRequestProto:
                handle_unsubscribe_data_updates(config, service_provider, log_context, std::move(request_buffer), request_context);
                break;
            case UserRequestUnionProto::SubscribeMessageRequestProto:
                handle_subscribe_message(config, service_provider, log_context, std::move(request_buffer), request_context);
                break;
            case UserRequestUnionProto::UnsubscribeMessageRequestProto:
                handle_unsubscribe_message(config, service_provider, log_context, std::move(request_buffer), request_context);
                break;
            case UserRequestUnionProto::SaveDataRequestProto:
                handle_save_data(config, service_provider, log_context, std::move(request_buffer), request_context);
                break;
            case UserRequestUnionProto::CallServiceMethodRequestProto:
                handle_call_service_method(config, service_provider, log_context, std::move(request_buffer), request_context);
                break;
            default:
                assert(false); //caught in validator
                break;
        }
    }
}
