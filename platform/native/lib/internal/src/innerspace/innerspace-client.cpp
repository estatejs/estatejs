//
// Created by scott on 1/21/2022.
//

#include "estate/internal/innerspace/innerspace-client.h"
#include "estate/internal/flatbuffers_util.h"

namespace estate::innerspace {
    struct WorkerProcessEndpoint {
        BreakerS worker_process_breaker;
        u16 setup_worker_port;
        u16 delete_worker_port;
        u16 user_port;
        template<typename TReq, typename TResp>
        [[nodiscard]] u16 get_port() const;
    };

    template<>
    u16 WorkerProcessEndpoint::get_port<SetupWorkerRequestProto, SetupWorkerResponseProto>() const {
        return this->setup_worker_port;
    }
    template<>
    u16 WorkerProcessEndpoint::get_port<DeleteWorkerRequestProto, DeleteWorkerResponseProto>() const {
        return this->delete_worker_port;
    }
    template<>
    u16 WorkerProcessEndpoint::get_port<UserRequestProto, WorkerProcessUserResponseProto>() const {
        return this->user_port;
    }

    class WorkerLoaderClient : public std::enable_shared_from_this<WorkerLoaderClient> {
        BufferPoolS _buffer_pool;
        using InnerspaceT = Innerspace<GetWorkerProcessEndpointRequestProto, GetWorkerProcessEndpointResponseProto>;
        InnerspaceT::ClientS _client;
        std::mutex _endpoints_mutex;
        std::unordered_map<WorkerId, WorkerProcessEndpoint> _endpoints{};
    public:
        WorkerLoaderClient(InnerspaceT::ClientS client, BufferPoolS buffer_pool) :
                _client{std::move(client)}, _buffer_pool{std::move(buffer_pool)} {
        }

        [[nodiscard]] bool is_faulted() const {
            return _client->is_faulted();
        }

        /* Calls Serenity's Worker-Loader process to get and/or load the Worker-Process process and return the ports it's listening on. */
        void async_with_worker_process_endpoint(const LogContext &log_context, WorkerId worker_id,
                                            std::function<void(ResultCode<WorkerProcessEndpoint>)> response_handler) {
            log_trace(log_context, "Getting worker process endpoint from cache");

            {
                std::lock_guard<std::mutex> lck{_endpoints_mutex};
                const auto it = _endpoints.find(worker_id);
                if (it != _endpoints.end()) {
                    if (it->second.worker_process_breaker->is_faulted()) {
                        log_warn(log_context, "Replacing previously faulted endpoint for worker {}", worker_id);
                        _endpoints.erase(it);
                    } else {
                        log_trace(log_context, "Retrieved worker process endpoint from cache");
                        response_handler(ResultCode<WorkerProcessEndpoint>::Ok(it->second));
                        return;
                    }
                }
            }

            fbs::Builder builder{};

            auto target = CreateGetWorkerProcessEndpointRequestProto(builder,
                                                                 builder.CreateString(
                                                                         log_context.get_context()),
                                                                 worker_id);

            auto buffer = finish_and_copy_to_buffer(builder, _buffer_pool, target);

            log_trace(log_context, "Getting connection from worker loader client");

            auto conn_r = _client->get_connection(log_context);
            if (!conn_r) {
                response_handler(ResultCode<WorkerProcessEndpoint>::Error(conn_r.get_error()));
                return;
            }

            log_trace(log_context, "Getting worker process endpoint from loader");

            auto conn = conn_r.unwrap();
            std::shared_ptr<WorkerLoaderClient> self = this->shared_from_this();
            conn->async_send(log_context, buffer.get_view(),
                             [self, log_context, worker_id, response_handler{std::move(response_handler)}]
                                     (ResultCode<InnerspaceT::ResponseEnvelope> envelope_r) mutable {
                                 using Result = ResultCode<WorkerProcessEndpoint>;

                                 if (!envelope_r) {
                                     response_handler(Result::Error(envelope_r.get_error()));
                                     return;
                                 }

                                 auto envelope = envelope_r.unwrap();
                                 const auto &response = *envelope.get_payload();

                                 switch (response.value_type()) {
                                     case GetWorkerProcessEndpointErrorUnionProto::ErrorCodeResponseProto: {
                                         const auto code = (Code) response.value_as_ErrorCodeResponseProto()->error_code();
                                         response_handler(Result::Error(code));
                                         break;
                                     }
                                     case GetWorkerProcessEndpointErrorUnionProto::WorkerProcessEndpointProto: {
                                         const auto proto = response.value_as_WorkerProcessEndpointProto();
                                         WorkerProcessEndpoint endpoint{
                                                 std::make_shared<Breaker>(),
                                                 proto->setup_worker_port(),
                                                 proto->delete_worker_port(),
                                                 proto->user_port()
                                         };

                                         {
                                             std::lock_guard<std::mutex> lck{self->_endpoints_mutex};
                                             const auto it = self->_endpoints.find(worker_id);
                                             if (it != self->_endpoints.end()) {
                                                 log_trace(log_context, "Got worker process endpoint from cache after trying to re-create it");
                                                 response_handler(ResultCode<WorkerProcessEndpoint>::Ok(it->second));
                                                 //silently discard the newly returned endpoint because it's probably a duplicate.
                                                 return;
                                             }

                                             self->_endpoints[worker_id] = endpoint;
                                         }

                                         log_trace(log_context, "Retrieved worker process endpoint from worker loader");
                                         response_handler(Result::Ok(std::move(endpoint)));
                                         break;
                                     }
                                     default:
                                         assert(false);
                                 }
                             });
        }
    };
    using WorkerLoaderClientS = std::shared_ptr<WorkerLoaderClient>;

    WorkerLoaderClientS create_worker_loader_client(InnerspaceWorkerLoaderClientConfig config, BufferPoolS buffer_pool, IoContextS io_context) {
        return std::make_shared<WorkerLoaderClient>(
                Innerspace<GetWorkerProcessEndpointRequestProto, GetWorkerProcessEndpointResponseProto>::CreateClient(
                        std::move(config), buffer_pool,
                        io_context, std::make_shared<Breaker>()), buffer_pool);
    }

    class WorkerLoaderClientFactory : public std::enable_shared_from_this<WorkerLoaderClientFactory> {
        InnerspaceWorkerLoaderClientConfig _config;
        Service<BufferPool> _buffer_pool;
        Service<IoContext> _io_context;
        Service<WorkerLoaderClient> _worker_loader_client;
    public:
        WorkerLoaderClientFactory(InnerspaceWorkerLoaderClientConfig config, BufferPoolS buffer_pool, IoContextS io_context) :
                _config{config},
                _buffer_pool{buffer_pool},
                _io_context{io_context},
                _worker_loader_client{create_worker_loader_client(std::move(config), buffer_pool, io_context)} {
        }
        WorkerLoaderClientS get(const LogContext &log_context) {
            std::shared_ptr<WorkerLoaderClientFactory> self = this->shared_from_this();
            return _worker_loader_client.get_or_replace(
                    [](const WorkerLoaderClient &c) { return c.is_faulted(); },
                    [self, log_context]() {
                        log_warn(log_context, "Replaced worker-loader client to {}:{} host due to a fault.", self->_config.host, self->_config.port);
                        return create_worker_loader_client(
                                self->_config,
                                self->_buffer_pool.get_service(),
                                self->_io_context.get_service());
                    }
            );
        }
    };
    using WorkerLoaderClientFactoryS = std::shared_ptr<WorkerLoaderClientFactory>;

    /* Used to send SetupWorker, DeleteWorker, or User commands */
    template<typename TReq, typename TResp>
    class WorkerProcessClient {
        using InnerspaceT = Innerspace<TReq, TResp>;
        typename InnerspaceT::ClientS _client;
    public:
        explicit WorkerProcessClient(typename InnerspaceT::ClientS client) :
                _client{std::move(client)} {}
        [[nodiscard]] bool is_faulted() const {
            return _client->is_faulted();
        }
        void async_send(const LogContext &log_context, Buffer<TReq> request_buffer,
                        typename Innerspace<TReq, TResp>::Client::Connection::ResponseHandler response_handler) {
            using ResponseHandlerResult = typename Innerspace<TReq, TResp>::Client::Connection::ResponseHandlerResult;
            auto conn_r = _client->get_connection(log_context);
            if (conn_r) {
                auto conn = conn_r.unwrap();
                /*NOTE: The request_buffer is copied synchronously as a result of this call so it's safe to pass a view here so long as we're
                 * holding onto the reference until after the call is over. */
                conn->async_send(log_context, request_buffer.get_view(), std::move(response_handler));
            } else {
                log_trace(log_context, "When trying to get the connection to worker process received an error instead: {}",
                          get_code_name(conn_r.get_error()));
                response_handler(ResponseHandlerResult::Error(conn_r.get_error()));
            }
        }
    };

    template<typename TReq, typename TResp>
    using WorkerProcessClientS = std::shared_ptr<WorkerProcessClient<TReq, TResp>>;

    template<typename TReq, typename TResp>
    class WorkerProcessClientFactory : public std::enable_shared_from_this<WorkerProcessClientFactory<TReq, TResp>> {
        Service<WorkerLoaderClientFactory> _worker_loader_client_factory;
        Service<BufferPool> _buffer_pool;
        Service<IoContext> _io_context;
        const std::string _host;
        const u8 _connection_count;
        const u32 _max_request_size;
        const u32 _max_response_size;
        std::mutex _worker_process_clients_mutex;
        std::unordered_map<WorkerId, Service<WorkerProcessClient<TReq, TResp>>> _worker_process_clients{};
    public:
        WorkerProcessClientFactory(WorkerLoaderClientFactoryS worker_loader_client_factory,
                               BufferPoolS buffer_pool,
                               IoContextS io_context,
                               std::string host,
                               u8 connection_count,
                               u32 max_request_size,
                               u32 max_response_size) :
                _worker_loader_client_factory{std::move(worker_loader_client_factory)},
                _buffer_pool{std::move(buffer_pool)},
                _io_context{std::move(io_context)},
                _host{std::move(host)},
                _connection_count{connection_count},
                _max_request_size{max_request_size},
                _max_response_size{max_response_size} {
        }
        void async_with_worker_process_client(const LogContext &log_context, WorkerId worker_id,
                                          std::function<void(ResultCode<WorkerProcessClientS<TReq, TResp>>)> handler) {

            { //if I've already got a WorkerProcess client, handle with that
                std::lock_guard<std::mutex> lck{_worker_process_clients_mutex};
                const auto it = _worker_process_clients.find(worker_id);
                if (it != _worker_process_clients.end()) {
                    WorkerProcessClientS<TReq, TResp> service = it->second.get_service();
                    if (service->is_faulted()) {
                        log_warn(log_context, "(in pre check) Removing previously faulted worker process client for worker {}", worker_id);
                        _worker_process_clients.erase(it);
                    } else {
                        log_trace(log_context, "Retrieved worker process client from cache");
                        handler(ResultCode<WorkerProcessClientS<TReq, TResp>>::Ok(it->second.get_service()));
                        return;
                    }
                }
            }

            auto worker_loader = _worker_loader_client_factory.get_service()->get(log_context);

            std::shared_ptr<WorkerProcessClientFactory> self = this->shared_from_this();

            worker_loader->async_with_worker_process_endpoint(log_context, worker_id,
                                                        [self, /*[sic] cant std::move this lest CLion not see it.*/
                                                                log_context,
                                                                worker_id,
                                                                handler{std::move(handler)},
                                                                buffer_pool{_buffer_pool.get_service()},
                                                                io_context{_io_context.get_service()}]
                                                                (ResultCode<WorkerProcessEndpoint> endpoint_r) {
                                                            if (endpoint_r) {
                                                                auto endpoint = endpoint_r.unwrap();

                                                                WorkerProcessClientS<TReq, TResp> client{};

                                                                {
                                                                    std::lock_guard<std::mutex> lck{self->_worker_process_clients_mutex};

                                                                    const auto it = self->_worker_process_clients.find(worker_id);
                                                                    if (it != self->_worker_process_clients.end()) {
                                                                        WorkerProcessClientS<TReq, TResp> service = it->second.get_service();
                                                                        if (!service->is_faulted()) {
                                                                            client = it->second.get_service(); //discard the newly created enpoint because it's already been replaced.
                                                                        } else {
                                                                            log_warn(log_context,
                                                                                     "(after creation) Replacing previously faulted WorkerProcessClient to worker {}",
                                                                                     worker_id);
                                                                            self->_worker_process_clients.erase(it); //remove it if it's faulted
                                                                        }
                                                                    }

                                                                    if (!client) {
                                                                        typename Innerspace<TReq, TResp>::Client::Config client_config{
                                                                                self->_host,
                                                                                endpoint.get_port<TReq, TResp>(),
                                                                                self->_connection_count,
                                                                                self->_max_request_size,
                                                                                self->_max_response_size
                                                                        };

                                                                        auto innerspace_client = Innerspace<TReq, TResp>::CreateClient(client_config,
                                                                                                                                       buffer_pool,
                                                                                                                                       io_context,
                                                                                                                                       endpoint.worker_process_breaker);

                                                                        auto[at, _] = self->_worker_process_clients.emplace(worker_id,
                                                                                                                        std::make_shared<WorkerProcessClient<TReq, TResp>>(
                                                                                                                                std::move(
                                                                                                                                        innerspace_client)));

                                                                        client = at->second.get_service();
                                                                    }
                                                                }

                                                                handler(ResultCode<WorkerProcessClientS<TReq, TResp>>::Ok(std::move(client)));
                                                            } else {
                                                                handler(ResultCode<WorkerProcessClientS<TReq, TResp>>::Error(endpoint_r.get_error()));
                                                            }
                                                        });
        }
    };
    template<typename TReq, typename TResp>
    using WorkerProcessClientFactoryS = std::shared_ptr<WorkerProcessClientFactory<TReq, TResp>>;

    class InnerspaceClient::Impl {
        std::optional<Service<WorkerProcessClientFactory<SetupWorkerRequestProto, SetupWorkerResponseProto>>>
                _maybe_setup_factory;
        std::optional<Service<WorkerProcessClientFactory<DeleteWorkerRequestProto, DeleteWorkerResponseProto>>> _maybe_delete_factory{};
        std::optional<Service<WorkerProcessClientFactory<UserRequestProto, WorkerProcessUserResponseProto>>> _maybe_user_factory{};
    public:
        /* Admin commands (Jayne) */
        Impl(BufferPoolS buffer_pool, IoContextS io_context, InnerspaceWorkerLoaderClientConfig worker_loader_config,
             InnerspaceClientConfig::AsWorkerAdmin admin_config) {

            auto worker_loader_client_factory = std::make_shared<WorkerLoaderClientFactory>(worker_loader_config, buffer_pool, io_context);

            _maybe_setup_factory.emplace(std::make_shared<WorkerProcessClientFactory<SetupWorkerRequestProto, SetupWorkerResponseProto>>(
                    worker_loader_client_factory,
                    buffer_pool,
                    io_context,
                    admin_config.host,
                    admin_config.setup_worker_connection_count,
                    admin_config.setup_worker_max_request_size,
                    admin_config.setup_worker_max_response_size));


            _maybe_delete_factory.emplace(std::make_shared<WorkerProcessClientFactory<DeleteWorkerRequestProto, DeleteWorkerResponseProto>>(
                    std::move(worker_loader_client_factory),
                    buffer_pool,
                    io_context,
                    admin_config.host,
                    admin_config.delete_worker_connection_count,
                    admin_config.delete_worker_max_request_size,
                    admin_config.delete_worker_max_response_size));
        }
        /* User commands (River) */
        Impl(BufferPoolS buffer_pool, IoContextS io_context, InnerspaceWorkerLoaderClientConfig worker_loader_config,
             InnerspaceClientConfig::AsWorkerUser user_config) {

            _maybe_user_factory.emplace(std::make_shared<WorkerProcessClientFactory<UserRequestProto, WorkerProcessUserResponseProto>>(
                    std::make_shared<WorkerLoaderClientFactory>(worker_loader_config, buffer_pool, io_context),
                    buffer_pool,
                    io_context,
                    user_config.host,
                    user_config.user_connection_count,
                    user_config.user_max_request_size,
                    user_config.user_max_response_size));
        }

        template<typename TReq, typename TResp>
        WorkerProcessClientFactoryS<TReq, TResp> get_factory();

        template<typename TReq, typename TResp>
        using ConnectionT = typename Innerspace<TReq, TResp>::Client::Connection;

        template<typename TReq, typename TResp>
        void async_send(const LogContext &log_context, WorkerId worker_id, Buffer<TReq> request_buffer,
                        typename ConnectionT<TReq, TResp>::ResponseHandler response_handler) {
            WorkerProcessClientFactoryS<TReq, TResp> factory = get_factory<TReq, TResp>();
            log_trace(log_context, "Retreived worker process client factory");
            factory->async_with_worker_process_client(log_context, worker_id,
                                                  [log_context, request_buffer{std::move(request_buffer)}, response_handler{
                                                          std::move(response_handler)}]
                                                          (ResultCode<WorkerProcessClientS<TReq, TResp>> worker_process_client_r) {
                                                      if (worker_process_client_r) {
                                                          //[sic] extra variable because I need CLion to find the function.
                                                          WorkerProcessClientS<TReq, TResp> worker_process_client = worker_process_client_r.unwrap();
                                                          WorkerProcessClient<TReq, TResp> &worker_process_client_ = *worker_process_client;
                                                          worker_process_client_.async_send(log_context, request_buffer,
                                                                                        std::move(response_handler));
                                                      } else {
                                                          log_trace(log_context, "Received error {} instead of worker process client",
                                                                    get_code_name(worker_process_client_r.get_error()));
                                                          response_handler(ConnectionT<TReq, TResp>::ResponseHandlerResult::Error(
                                                                  worker_process_client_r.get_error()));
                                                      }
                                                  });
        }
    };

    template<>
    WorkerProcessClientFactoryS<UserRequestProto, WorkerProcessUserResponseProto> InnerspaceClient::Impl::get_factory() {
        assert(this->_maybe_user_factory.has_value());
        return this->_maybe_user_factory->get_service();
    }

    template<>
    WorkerProcessClientFactoryS<SetupWorkerRequestProto, SetupWorkerResponseProto> InnerspaceClient::Impl::get_factory() {
        assert(this->_maybe_setup_factory.has_value());
        return this->_maybe_setup_factory->get_service();
    }

    template<>
    WorkerProcessClientFactoryS<DeleteWorkerRequestProto, DeleteWorkerResponseProto> InnerspaceClient::Impl::get_factory() {
        assert(this->_maybe_delete_factory.has_value());
        return this->_maybe_delete_factory->get_service();
    }

    InnerspaceClient::~InnerspaceClient() {
        if (_impl)
            delete _impl;
    }

    InnerspaceClient::InnerspaceClient(BufferPoolS buffer_pool, IoContextS io_context, InnerspaceWorkerLoaderClientConfig worker_loader_config,
                                       InnerspaceClientConfig::AsWorkerAdmin admin_config) {
        _impl = new Impl(buffer_pool, io_context, worker_loader_config, std::move(admin_config));
    }
    InnerspaceClient::InnerspaceClient(BufferPoolS buffer_pool, IoContextS io_context, InnerspaceWorkerLoaderClientConfig worker_loader_config,
                                       InnerspaceClientConfig::AsWorkerUser user_config) {
        _impl = new Impl(buffer_pool, io_context, worker_loader_config, std::move(user_config));
    }

    void InnerspaceClient::async_send(const LogContext &log_context, WorkerId worker_id, Buffer<UserRequestProto> request_buffer,
                                      Innerspace<UserRequestProto, WorkerProcessUserResponseProto>::Client::Connection::ResponseHandler response_handler) {
        assert(_impl);
        _impl->async_send<UserRequestProto, WorkerProcessUserResponseProto>(log_context, worker_id, std::move(request_buffer), std::move(response_handler));
    }
    void InnerspaceClient::async_send(const LogContext &log_context, WorkerId worker_id, Buffer<SetupWorkerRequestProto> request_buffer,
                                      Innerspace<SetupWorkerRequestProto, SetupWorkerResponseProto>::Client::Connection::ResponseHandler response_handler) {
        assert(_impl);
        _impl->async_send<SetupWorkerRequestProto, SetupWorkerResponseProto>(log_context, worker_id, std::move(request_buffer),
                                                                         std::move(response_handler));
    }
    void InnerspaceClient::async_send(const LogContext &log_context, WorkerId worker_id, Buffer<DeleteWorkerRequestProto> request_buffer,
                                      Innerspace<DeleteWorkerRequestProto, DeleteWorkerResponseProto>::Client::Connection::ResponseHandler response_handler) {
        assert(_impl);
        _impl->async_send<DeleteWorkerRequestProto, DeleteWorkerResponseProto>(log_context, worker_id, std::move(request_buffer),
                                                                           std::move(response_handler));
    }
}