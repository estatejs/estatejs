//
// Created by scott on 4/26/20.
//

#pragma once

#include "estate/internal/buffer_pool.h"
#include "estate/internal/thread_pool.h"
#include "estate/internal/logging.h"
#include "estate/internal/deps/boost.h"
#include "estate/internal/processor/service.h"
#include "estate/internal/net_util.h"
#include "estate/runtime/protocol/serenity_interface_generated.h"

#include <estate/runtime/deps/flatbuffers.h>
#include <estate/runtime/event.h>
#include <estate/runtime/model_types.h>
#include <estate/runtime/result.h>

#include <utility>
#include <vector>
#include <queue>
#include <unordered_set>
//#include <opencl-c-base.h>

namespace estate {
    struct InnerspaceClientEndpoint {
        bool operator==(const InnerspaceClientEndpoint &rhs) const;
        const std::string host;
        const u16 port;
        struct Hasher {
            std::size_t operator()(const InnerspaceClientEndpoint &client_endpoint) const;
        };
    };

    class Breaker {
        std::atomic_bool _fault{false};
    public:
        Breaker() = default;
        Breaker(const Breaker &) = delete;
        Breaker(Breaker &&) = delete;
        bool fault();
        [[nodiscard]] bool is_faulted() const;
    };
    using BreakerS = std::shared_ptr<Breaker>;

    ResultCode<ResolvedEndpoint, Code> resolve_endpoint(const std::string &host, u16 port, IoContextS io_context);

    template<typename TRequestProto,
            typename TResponseProto>
    struct Innerspace {
#pragma pack(push, 1)
        struct RequestHeader {
            RequestId request_id;
            u32 payload_size;
        };
#pragma pack(pop)

        using RequestEnvelope = Envelope<RequestHeader, TRequestProto, u32>;

#pragma pack(push, 1)
        struct ResponseHeader {
            RequestId request_id;
            u32 payload_size;
        };
#pragma pack(pop)

        using ResponseEnvelope = Envelope<ResponseHeader, TResponseProto, u32>;

        class Client;
        using ClientS = std::shared_ptr<Client>;

        class Client {
        public:
            struct Config {
                std::string host;
                u16 port;
                u8 connection_count;
                u32 max_request_size;
                u32 max_response_size;
                static Config FromRemote(const LocalConfigurationReader &reader) {
                    return Config{
                            reader.get_string("host"),
                            reader.get_u16("port"),
                            reader.get_u8("connection_count"),
                            reader.get_u32("max_request_size"),
                            reader.get_u32("max_response_size")
                    };
                }
            };
            class ConnectionPool;
            class Connection;
            using ConnectionS = std::shared_ptr<Connection>;
            class Connection : public std::enable_shared_from_this<Connection> {
            public:
                using ResponseHandlerResult = ResultCode<ResponseEnvelope>;
                using ResponseHandler = std::function<void(ResponseHandlerResult)>;
                struct RequestBundle {
                    const LogContext log_context;
                    RequestId request_id;
                    ResponseHandler response_handler;
                    RequestEnvelope request_message;
                };
                Connection(const Config &config, BufferPoolS buffer_pool, Socket socket, ExecutorStrand strand, IoContextS io_context,
                           BreakerS breaker) :
                        _buffer_pool{std::move(buffer_pool)},
                        _config{config},
                        _socket{std::move(socket)},
                        _strand{std::move(strand)},
                        _io_context{io_context},
                        _keep_running{false},
                        _breaker{breaker} {}
                friend class ConnectionPool;
            private:
                const Config _config;
                std::optional<ResolvedEndpoint> _endpoint{};
                BufferPoolS _buffer_pool;
                std::atomic<RequestId> _next_request_id{1};
                std::mutex _connection_mutex;
                Socket _socket;
                IoContextS _io_context;
                ExecutorStrand _strand;
                std::mutex _request_bundle_queue_mutex;
                std::queue<RequestBundle> _request_bundle_queue;
                std::mutex _response_map_mutex;
                std::unordered_map<RequestId, ResponseHandler> _response_map;
                std::unique_ptr<std::thread> _write_thread{};
                std::unique_ptr<std::thread> _read_thread{};
                BreakerS _breaker;
                std::atomic_bool _keep_running;
                Event _start_write;
                Event _finish_write;
                Event _start_read;
                Event _finish_read;
            public:
                UnitResultCode ensure_ready(const LogContext &log_context) {
                    using Result = UnitResultCode;

                    if (_breaker->is_faulted())
                        return Result::Error(Code::Innerspace_ConnectionFault);

                    std::lock_guard<std::mutex> lck(_connection_mutex);

                    if (!_endpoint.has_value()) {
                        auto resolved_r = resolve_endpoint(_config.host, _config.port, _io_context);
                        if (!resolved_r)
                            return Result::Error(resolved_r.get_error());
                        _endpoint = resolved_r.unwrap();
                    }

                    bool threads_running = _write_thread != nullptr && _read_thread != nullptr;
                    bool is_open = _socket.is_open();

                    if (!threads_running || !is_open) {
                        stop_threads();

                        if (is_open) {
                            disconnect();
                        }

                        auto c_r = connect(log_context);
                        if (!c_r)
                            return Result::Error(c_r.get_error());

                        auto s_r = start_threads();
                        if (!s_r)
                            return Result::Error(s_r.get_error());
                    }

                    return Result::Ok();
                }
                void fault(const char *cause, const boost::system::error_code &error_code) {
                    if (_breaker->fault()) {
                        /*First fault*/
                        sys_log_error("{} caused a fault from {}:{} because the code {} message {}",
                                      cause, _endpoint.value()->host_name(), _endpoint.value()->service_name(),
                                      error_code.value(), error_code.message());
                    } else {
                        sys_log_trace("[[Previously faulted]] {} caused a fault from {}:{} because the code {} message {}",
                                      cause, _endpoint.value()->host_name(), _endpoint.value()->service_name(),
                                      error_code.value(), error_code.message());
                    }

                    std::lock_guard<std::mutex> response_map_lock(_response_map_mutex);
                    std::lock_guard<std::mutex> request_bundle_queue_lock(_request_bundle_queue_mutex);

                    //stop threads and disconnect
                    stop_threads();
                    if (_socket.is_open()) {
                        disconnect();
                        assert(!_socket.is_open());
                    }

                    //signal all the waiting handlers they should perform a client-side retry
                    for (const auto[_, response_handler]: _response_map) {
                        response_handler(ResultCode<ResponseEnvelope>::Error(Code::Innerspace_ConnectionFault));
                    }
                }
            public:
                Connection(const Connection &other) = delete;
                void async_send(const LogContext &log_context, const BufferView<TRequestProto> request_buffer, ResponseHandler response_handler) {
                    if (_breaker->is_faulted()) {
                        response_handler(ResponseHandlerResult::Error(Code::Innerspace_ConnectionFault));
                        return;
                    }

                    auto bundle_r = bundle_request(log_context, request_buffer, response_handler);
                    if (!bundle_r) {
                        response_handler(ResponseHandlerResult::Error(bundle_r.get_error()));
                        return;
                    }
                    auto bundle = bundle_r.unwrap();

                    {
                        std::lock_guard<std::mutex> lck(_request_bundle_queue_mutex);
                        _request_bundle_queue.push(std::move(bundle));
                        _start_write.notify_one();
                    }
                }
            private:
                ResultCode<RequestBundle, Code>
                bundle_request(const LogContext &log_context, const BufferView<TRequestProto> request_buffer, ResponseHandler response_handler) {
                    using Result = ResultCode<RequestBundle, Code>;

                    const auto request_size = request_buffer.size();

                    if (request_size > _config.max_request_size) {
                        log_error(log_context, "Error sending request because its size {} was larger than the maximum request size {}",
                                  request_size, _config.max_request_size);
                        return Result::Error(Code::Innerspace_RequestTooLarge);
                    }

                    RequestBundle bundle{
                            log_context,
                            std::move(_next_request_id.fetch_add(1)),
                            std::move(response_handler),
                            std::move(_buffer_pool->get_envelope<RequestHeader, TRequestProto, u32>())
                    };

                    bundle.request_message.resize_for_payload(request_size);

                    RequestHeader *request_header = bundle.request_message.get_header();
                    request_header->request_id = bundle.request_id;
                    request_header->payload_size = request_size;

                    std::memcpy(bundle.request_message.get_payload_raw(), request_buffer.as_char(), request_size);

                    assert(bundle.request_message.get_payload_size() == request_size);
                    assert(bundle.request_message.get_header_size() == sizeof(RequestHeader));
                    assert(bundle.request_message.get_total_size() == request_size + sizeof(RequestHeader));

                    return Result::Ok(std::move(bundle));
                }
                void async_write_request(RequestBundle bundle) {
                    log_trace(bundle.log_context, "Sending request {} of size {} bytes", bundle.request_id, bundle.request_message.get_total_size());

                    auto buffer = boost::asio::buffer(bundle.request_message.get_header_raw(), bundle.request_message.get_total_size());

                    std::shared_ptr<Connection> self{this->shared_from_this()};

                    boost::asio::async_write(
                            _socket,
                            buffer,
                            [self, bundle{std::move(bundle)}]
                                    (const boost::system::error_code &error_code, const std::size_t bytes_transferred) mutable {
                                if (error_code) {
                                    self->fault("Write request buffer", error_code);
                                    //since this one hasn't been added to response_map, fire with empty to make it retry client-side
                                    bundle.response_handler(ResultCode<ResponseEnvelope>::Error(Code::Innerspace_ConnectionFault));
                                    return;
                                }

                                log_trace(bundle.log_context, "Request sent of {} bytes", bundle.request_message.get_total_size());

                                {
                                    std::lock_guard<std::mutex> lck(self->_response_map_mutex);
                                    self->_response_map[bundle.request_id] = bundle.response_handler;
                                }

                                self->_finish_write.notify_one();
                                self->_start_read.notify_one();
                            });
                }
                void async_read_response() {
                    sys_log_trace("Starting to read response header of {} bytes.", sizeof(ResponseHeader));

                    auto response_buffer = _buffer_pool->get_envelope<ResponseHeader, TResponseProto, u32>();
                    response_buffer.resize_for_header();

                    auto outer_buffer = boost::asio::buffer(response_buffer.get_header_raw(), response_buffer.get_header_size());

                    std::shared_ptr<Connection> self{this->shared_from_this()};

                    boost::asio::async_read(_socket,
                                            outer_buffer,
                                            [self, response_buffer{std::move(response_buffer)}](const boost::system::error_code &error_code,
                                                                                                const std::size_t bytes_transferred) mutable {
                                                if (error_code) {
                                                    self->fault("Reading response header", error_code);
                                                    return;
                                                }

                                                sys_log_trace("Response received of {} bytes", bytes_transferred);

                                                if (response_buffer.get_header()->payload_size > self->_config.max_response_size) {
                                                    sys_log_error(
                                                            "Error while getting response header. Payload size {} was bigger than max allowed {}",
                                                            response_buffer.get_header()->payload_size,
                                                            self->_config.max_response_size);
                                                    return;
                                                }

                                                response_buffer.resize_for_payload(response_buffer.get_header()->payload_size);

                                                sys_log_trace("Starting to read response payload of {} bytes",
                                                              response_buffer.get_header()->payload_size);

                                                auto inner_buffer = boost::asio::buffer(response_buffer.get_payload_raw(),
                                                                                        response_buffer.get_payload_size());

                                                auto &sock = self->_socket;

                                                boost::asio::async_read(sock,
                                                                        inner_buffer,
                                                                        [self, response_buffer{std::move(response_buffer)}]
                                                                                (const boost::system::error_code &error_code,
                                                                                 const std::size_t bytes_transferred) mutable {
                                                                            //since all the socket work is done, the remainder can overlap.
                                                                            self->_finish_read.notify_one();

                                                                            if (error_code) {
                                                                                self->fault("Reading response payload", error_code);
                                                                                return;
                                                                            }

                                                                            sys_log_trace("Response payload of {} bytes received", bytes_transferred);

                                                                            auto response_handler_o = self->get_response_handler(
                                                                                    response_buffer.get_header()->request_id);
                                                                            if (!response_handler_o) {
                                                                                sys_log_warn(
                                                                                        "Response handler wasn't found for request id {}. This may have been caused by a reset.",
                                                                                        response_buffer.get_header()->request_id);
                                                                                return;
                                                                            }

                                                                            ResponseHandler response_handler = std::move(response_handler_o.value());

                                                                            sys_log_trace("Calling response handler for request id {}",
                                                                                          response_buffer.get_header()->request_id);

                                                                            response_handler(
                                                                                    ResultCode<ResponseEnvelope>::Ok(std::move(response_buffer)));

                                                                            sys_log_trace("Handled response successfully");
                                                                        });
                                            });
                }
                std::optional<ResponseHandler> get_response_handler(RequestId request_id) {
                    std::lock_guard<std::mutex> lck(_response_map_mutex);
                    auto it = _response_map.find(request_id);
                    if (it == _response_map.end())
                        return std::nullopt;
                    ResponseHandler response_handler = std::move(it->second);
                    _response_map.erase(it);
                    return response_handler;
                }
                std::optional<RequestBundle> get_request() {
                    std::lock_guard<std::mutex> lck(_request_bundle_queue_mutex);
                    if (_request_bundle_queue.empty())
                        return std::nullopt;
                    auto request = std::move(_request_bundle_queue.front());
                    _request_bundle_queue.pop();
                    return std::move(request);
                }
                void stop_threads() {
                    _keep_running = false;
                    if (_write_thread) {
                        sys_log_trace("Stopping write thread");
                        _start_write.notify_one();
                        _finish_write.notify_one();
                        _write_thread->join();
                        _write_thread = nullptr;
                        sys_log_trace("Write thread stopped");
                    }
                    if (_read_thread) {
                        sys_log_trace("Stopping read thread");
                        _start_read.notify_one();
                        _finish_read.notify_one();
                        _read_thread->join();
                        _read_thread = nullptr;
                        sys_log_trace("Stopped read thread");
                    }
                }
                UnitResultCode start_threads() {
                    assert(!_breaker->is_faulted());

                    if (_write_thread) {
                        sys_log_error("Send thread was already started when trying to start it.");
                        return UnitResultCode::Error(Code::InnerspaceClientConnection_UnexpectedSendThreadAlreadyRunning);
                    }

                    if (_read_thread) {
                        sys_log_error("Receive thread was already running when trying to start it");
                        return UnitResultCode::Error(Code::InnerspaceClientConnection_UnexpectedRecvThreadAlreadyRunning);
                    }

                    _keep_running = true;

                    std::shared_ptr<Connection> self{this->shared_from_this()};

                    _write_thread = std::make_unique<std::thread>([self]() {
                        sys_log_trace("Send thread started");
                        while (self->_keep_running) {
                            self->_start_write.wait_one();
                            if (!self->_keep_running)
                                break;
                            while (auto request_o = self->get_request()) {
                                auto request = std::move(request_o.value());
                                self->async_write_request(std::move(request));
                                self->_finish_write.wait_one();
                                if (!self->_keep_running)
                                    break;
                            }
                        }
                        sys_log_trace("Send thread ended");
                    });

                    _read_thread = std::make_unique<std::thread>([self]() {
                        sys_log_trace("Recv thread started");
                        while (self->_keep_running) {
                            self->_start_read.wait_one();
                            if (!self->_keep_running)
                                break;
                            self->async_read_response();
                            self->_finish_read.wait_one();
                            if (!self->_keep_running)
                                break;
                        }
                        sys_log_trace("Recv thread ended");
                    });

                    return UnitResultCode::Ok();
                }
                void disconnect() {
                    boost::system::error_code ec;
                    _socket.close(ec);
                    if (ec) {
                        sys_log_error("Unable to close socket from {}:{}. Error: {}", _endpoint.value()->host_name(),
                                      _endpoint.value()->service_name(), ec.message());
                    }
                }
                UnitResultCode connect(const LogContext& log_context) {
                    using Result = UnitResultCode;

                    assert(_endpoint.has_value());

                    boost::system::error_code ec;
                    boost::asio::connect(_socket, _endpoint.value(), ec);

                    if (ec) {
                        this->fault("Trying to connect", ec);
                        return Result::Error(Code::Innerspace_ConnectionFault);
                    }

                    log_info(log_context, "Connection to {}:{} opened", _endpoint.value()->host_name(), _endpoint.value()->service_name());

                    return Result::Ok();
                }
            };
            class ConnectionPool {
                const u8 _count;
                const Config _config;
                BreakerS _breaker;
                std::vector<ConnectionS> _connections;
                std::atomic<u8> _next_connection{0};
                std::atomic_bool _keep_running{false};
                friend class Client;
            public:
                explicit ConnectionPool(const Config &config,
                                        BufferPoolS buffer_pool,
                                        IoContextS io_context,
                                        u8 count,
                                        BreakerS breaker) :
                        _config{config},
                        _count{count},
                        _breaker{breaker} {

                    for (int i = 0; i < count; ++i) {
                        ExecutorStrand strand = boost::asio::make_strand(*io_context);
                        Socket socket{strand};
                        _connections.emplace_back(std::make_shared<Connection>(config,
                                                                               buffer_pool,
                                                                               std::move(socket),
                                                                               std::move(strand),
                                                                               io_context,
                                                                               breaker)
                        );
                    }
                    _keep_running = true;
                }
                ResultCode<ConnectionS, Code> get_connection(const LogContext &log_context) {
                    using Result = ResultCode<ConnectionS, Code>;

                    if (_breaker->is_faulted())
                        return Result::Error(Code::Innerspace_ConnectionFault);

                    if (!_keep_running)
                        return Result::Error(Code::Innerspace_Shutdown);

                    auto conn_index = _next_connection.fetch_add(1) % _count;
                    ConnectionS connection = _connections[conn_index];

                    auto ec_r = connection->ensure_ready(log_context);
                    if (!ec_r) {
                        log_error(log_context, "Failed to read connection because it couldn't ensure it was ready");
                        return Result::Error(ec_r.get_error());
                    }

                    return Result::Ok(std::move(connection));
                }
                void shutdown() {
                    _keep_running = false;
                    for (ConnectionS connection: _connections) {
                        connection->disconnect();
                        connection->stop_threads();
                    }
                }
            };
            using ConnectionPoolU = std::unique_ptr<ConnectionPool>;
        private:
            friend class Innerspace;
            const Config _config;
            IoContextS _io_context;
            BufferPoolS _buffer_pool;
            ConnectionPoolU _connection_pool;
            BreakerS _breaker;
            std::atomic_bool _keep_running;
        public:
            explicit Client(const Config &config, BufferPoolS buffer_pool, IoContextS io_context, BreakerS breaker) :
                    _config(config),
                    _buffer_pool(std::move(buffer_pool)),
                    _io_context(std::move(io_context)),
                    _breaker{std::move(breaker)},
                    _keep_running{true} {

                if (_config.connection_count == 0)
                    throw std::domain_error("invalid connection count");

                _connection_pool = std::make_unique<ConnectionPool>(config, _buffer_pool, _io_context, config.connection_count, _breaker);
            }

            [[nodiscard]] bool is_faulted() const {
                return _breaker->is_faulted();
            }

            void shutdown() {
                _keep_running = false;
                _connection_pool->shutdown();
            }

            ResultCode<ConnectionS, Code> get_connection(const LogContext &log_context) {
                using Result = ResultCode<ConnectionS, Code>;

                if (_breaker->is_faulted())
                    return Result::Error(Code::Innerspace_ConnectionFault);

                if (!_keep_running)
                    return Result::Error(Code::Innerspace_Shutdown);

                auto conn_r = _connection_pool->get_connection(log_context);
                if (!conn_r) {
                    return Result::Error(conn_r.get_error());
                }
                return Result::Ok(conn_r.unwrap());
            }
        };

        static ClientS CreateClient(const typename Client::Config &config, BufferPoolS buffer_pool, IoContextS io_context, BreakerS breaker) {
            return std::make_shared<Innerspace::Client>(config, std::move(buffer_pool), std::move(io_context), breaker);
        }

        class Server;
        using ServerS = std::shared_ptr<Server>;
        class Server : public std::enable_shared_from_this<Server> {
        public:
            struct Config {
                Endpoint listen_endpoint;
                u32 max_request_size;
                u32 max_response_size;
                static Config FromRemote(const LocalConfigurationReader &reader) {
                    std::string listen_ip = reader.get_string("listen_ip");
                    u16 listen_port = reader.get_u16("listen_port");
                    return Config{
                            make_endpoint(listen_ip, listen_port),
                            reader.get_u32("max_request_size"),
                            reader.get_u32("max_response_size")
                    };
                }
                static Config FromRemoteWithoutPort(const LocalConfigurationReader &reader, u16 listen_port) {
                    std::string listen_ip = reader.get_string("listen_ip");
                    return Config{
                            make_endpoint(listen_ip, listen_port),
                            reader.get_u32("max_request_size"),
                            reader.get_u32("max_response_size")
                    };
                }
            };
            class ServerConnection;
            using ServerConnectionS = std::shared_ptr<ServerConnection>;
            using ServerRequestDispatcher = std::function<void(ServerConnectionS, RequestEnvelope &&)>;

            struct ServerRequestContext {
                ServerRequestContext(ServerConnectionS connection, RequestId request_id) :
                        connection(std::move(connection)), request_id(request_id) {}
                ServerRequestContext(const ServerRequestContext &other) = delete;
                ServerRequestContext(ServerRequestContext &&other) = delete;
                void async_respond(const LogContext &log_context, const BufferView<TResponseProto> &response, std::optional<std::function<void()>> and_then) {
                    assert(connection);
                    assert(request_id > 0);
                    connection->async_send_response(log_context, request_id, std::move(response), std::move(and_then));
                }
            private:
                ServerConnectionS connection;
                const RequestId request_id;
            };
            class ServerConnection : public std::enable_shared_from_this<ServerConnection> {
                struct ResponseBundle {
                    const LogContext log_context;
                    ResponseEnvelope response_envelope;
                    std::optional<std::function<void()>> and_then;
                };

                ServerS server;
                const Config config;
                BufferPoolS buffer_pool;
                ServerRequestDispatcher request_dispatcher;
                IoContextS io_context;
                Socket socket;
                ExecutorStrand strand;
                Event start_write;
                Event finish_write;
                std::mutex response_queue_mutex;
                std::queue<ResponseBundle> response_queue;
                std::unique_ptr<std::thread> write_thread;
                std::atomic_bool keep_running{false};

                explicit ServerConnection(const Config &config, ServerS server, BufferPoolS buffer_pool, Socket &&socket, ExecutorStrand &&strand,
                                          IoContextS io_context, ServerRequestDispatcher request_dispatcher) :
                        config(config),
                        server(std::move(server)),
                        buffer_pool(std::move(buffer_pool)),
                        socket(std::move(socket)),
                        strand(std::move(strand)),
                        io_context(io_context),
                        request_dispatcher(request_dispatcher) {}
                static ServerConnectionS
                Create(const Config &config, ServerS server, BufferPoolS buffer_pool, IoContextS io_context,
                       ServerRequestDispatcher request_dispatcher) {
                    ExecutorStrand strand = boost::asio::make_strand(*io_context);
                    Socket socket{strand};
                    return std::shared_ptr<ServerConnection>(
                            new ServerConnection(config, std::move(server), std::move(buffer_pool), std::move(socket),
                                                 std::move(strand), io_context, request_dispatcher));
                }
                void start() {
                    keep_running = true;
                    start_thread();
                    async_read_request();
                }
                void stop() {
                    keep_running = false;
                    boost::system::error_code ec;
                    socket.close(ec);
                    if (ec) {
                        sys_log_warn("Error while closing socket: {}", ec.message());
                    }
                    start_write.notify_one();
                    finish_write.notify_one();
                    if (write_thread) {
                        sys_log_trace("Stopping write thread");
                        write_thread->join();
                        write_thread = nullptr;
                        sys_log_trace("Write thread stopped");
                    }
                }
            public:
                ~ServerConnection() {
                    stop();
                    if (server->keep_running) //don't bother removing it if we're already shutting down
                        server->remove_connection(this->shared_from_this());
                }
                void async_send_response(const LogContext &log_context, RequestId request_id, const BufferView<TResponseProto> &response_buffer, std::optional<std::function<void()>> and_then) {
                    if (!keep_running) {
                        log_error(log_context, "Unable to send response because the connection was shutdown");
                        return;
                    }

                    auto response_bundle_r = create_response_bundle(log_context, request_id, response_buffer, std::move(and_then));
                    if (!response_bundle_r) {
                        return;
                    }

                    auto response_bundle = response_bundle_r.unwrap();

                    std::lock_guard<std::mutex> lck(response_queue_mutex);
                    response_queue.push(std::move(response_bundle));
                    start_write.notify_one();
                }
            private:
                friend class Server;
                Socket &get_socket() {
                    return socket;
                }
                Result<ResponseBundle>
                create_response_bundle(const LogContext &log_context, RequestId request_id, const BufferView<TResponseProto> &response_buffer,
                                       std::optional<std::function<void()>> and_then) {
                    using Result = Result<ResponseBundle>;

                    if (response_buffer.size() > config.max_response_size) {
                        log_error(log_context, "For request id {}, the response size {} was bigger than the max response size {}", request_id,
                                  response_buffer.size(), config.max_response_size);
                        return Result::Error();
                    }

                    ResponseEnvelope response_envelope = buffer_pool->get_envelope<ResponseHeader, TResponseProto, u32>();

                    response_envelope.resize_for_payload(response_buffer.size());

                    ResponseHeader *response_header = response_envelope.get_header();
                    response_header->payload_size = response_buffer.size();
                    response_header->request_id = request_id;

                    std::memcpy(response_envelope.get_payload_raw(), response_buffer.as_u8(), response_buffer.size());

                    assert(response_envelope.get_payload_size() == response_buffer.size());
                    assert(response_envelope.get_header_size() == sizeof(RequestHeader));
                    assert(response_envelope.get_total_size() == response_buffer.size() + sizeof(RequestHeader));

                    ResponseBundle response_bundle{
                            log_context,
                            std::move(response_envelope),
                            std::move(and_then) //https://youtu.be/oqwzuiSy9y0?t=18
                    };

                    return Result::Ok(std::move(response_bundle));
                }
                void async_read_request() {
                    sys_log_trace("Started reading requests");
                    RequestEnvelope request_envelope = buffer_pool->get_envelope<RequestHeader, TRequestProto, u32>();
                    request_envelope.resize_for_header();

                    auto outer_buffer = boost::asio::buffer((void *) request_envelope.get_header_raw(), request_envelope.get_header_size());

                    std::shared_ptr<ServerConnection> self{this->shared_from_this()};

                    boost::asio::async_read(socket,
                                            outer_buffer,
                                            [self, request_envelope{std::move(request_envelope)}]
                                                    (const boost::system::error_code &error_code, const std::size_t bytes_transferred) mutable {
                                                if (error_code) {
                                                    if (error_code.value() == 2) {
                                                        /* End of file = normal disconnection */
                                                        sys_log_trace("Connection disconnected");
                                                    } else if (self->keep_running /*Ignore errors if we're shutting down*/) {
                                                        sys_log_error("Error while reading request header. message: {} code: {}",
                                                                      error_code.message(), error_code.value());
                                                    }

                                                    return;
                                                }

                                                sys_log_trace("Read request header of {} bytes", bytes_transferred);

                                                const RequestHeader *request_header = request_envelope.get_header();
                                                const u32 payload_size = request_header->payload_size;

                                                if (payload_size > self->config.max_request_size) {
                                                    sys_log_error("Request size {} was larger than max request size {}",
                                                                  payload_size, self->config.max_request_size);
                                                    return;
                                                }

                                                sys_log_trace("Reading request payload of {} bytes", payload_size);

                                                request_envelope.resize_for_payload(payload_size);

                                                auto inner_buffer = boost::asio::buffer(request_envelope.get_payload_raw(), payload_size);

                                                auto &sock = self->socket;

                                                boost::asio::async_read(sock,
                                                                        inner_buffer,
                                                                        [self, request_envelope{std::move(request_envelope)}]
                                                                                (const boost::system::error_code &error_code,
                                                                                 const std::size_t bytes_transferred) mutable {
                                                                            if (error_code) {
                                                                                if (self->keep_running) {
                                                                                    sys_log_error("Error while reading request payload: {}",
                                                                                                  error_code.message());
                                                                                }
                                                                                return;
                                                                            }

                                                                            sys_log_trace("Read request payload of {} bytes", bytes_transferred);

                                                                            RequestId request_id = request_envelope.get_header()->request_id;
                                                                            self->request_dispatcher(self, std::move(request_envelope));
                                                                            sys_log_trace("Request {} dispatched successfully", request_id);

                                                                            if (self->keep_running)
                                                                                self->async_read_request();
                                                                            else
                                                                                sys_log_info("Stopped reading requests");
                                                                        });
                                            });
                }
                std::optional<ResponseBundle> get_queued_response() {
                    std::lock_guard<std::mutex> lck(response_queue_mutex);
                    if (response_queue.empty())
                        return std::nullopt;
                    ResponseBundle response_bundle = std::move(response_queue.front());
                    response_queue.pop();
                    return std::move(response_bundle);
                }
                void async_write_response(ResponseBundle response_bundle) {
                    auto buffer = boost::asio::buffer(response_bundle.response_envelope.get_header_raw(),
                                                      response_bundle.response_envelope.get_total_size());
                    boost::asio::async_write(socket, buffer,
                                             [self{this->shared_from_this()}, response_bundle{std::move(response_bundle)}]
                                                     (const boost::system::error_code &error_code, const std::size_t bytes_transferred) mutable {
                                                 self->finish_write.notify_one();

                                                 RequestId request_id = response_bundle.response_envelope.get_header()->request_id;

                                                 if (error_code) {
                                                     log_error(response_bundle.log_context,
                                                               "Error while sending response for request id {}. Message: {}", request_id,
                                                               error_code.message());
                                                     return;
                                                 }

                                                 log_trace(response_bundle.log_context, "Response for request id {} sent successfully", request_id);

                                                 if(response_bundle.and_then.has_value()) {
                                                     response_bundle.and_then.value()();
                                                 }
                                             });
                }
                void start_thread() {
                    assert(!write_thread);

                    write_thread = std::make_unique<std::thread>([self{this->shared_from_this()}]() {
                        sys_log_trace("Server write thread started");
                        while (self->keep_running) {
                            self->start_write.wait_one();
                            if (!self->keep_running)
                                break;
                            while (auto response_bundle_o = self->get_queued_response()) {
                                self->async_write_response(std::move(response_bundle_o.value()));
                                self->finish_write.wait_one();
                                if (!self->keep_running)
                                    break;
                            }
                        }
                        sys_log_trace("Server write thread stopped");
                    });
                }
            };
        private:
            friend class Innerspace;
            friend class Connection;
            const Config config;
            std::atomic_bool keep_running{false};
            BufferPoolS buffer_pool;
            IoContextS io_context;
            Acceptor acceptor;
            ServerRequestDispatcher request_dispatcher;
            std::mutex connections_mutex;
            std::unordered_set<ServerConnectionS> connections;
        public:
            Server(const Config &config, BufferPoolS buffer_pool,
                   IoContextS io_context, ServerRequestDispatcher &&request_dispatcher) :
                    buffer_pool(std::move(buffer_pool)),
                    request_dispatcher(std::move(request_dispatcher)),
                    acceptor(*io_context, config.listen_endpoint),
                    io_context(io_context),
                    config(config) {}
            void start() {
                keep_running = true;
                start_accept();
            }
            void shutdown() {
                boost::system::error_code ec;
                acceptor.close(ec);
                if (ec) {
                    sys_log_error( "Error while shutting down acceptor: {}", ec.message());
                }
                for (ServerConnectionS conn: connections) {
                    conn->stop();
                }
            }
        private:
            void add_connection(const ServerConnectionS connection) {
                std::lock_guard<std::mutex> lck(connections_mutex);
                connections.insert(connection);
            }
            void remove_connection(const ServerConnectionS &connection) {
                std::lock_guard<std::mutex> lck(connections_mutex);
                auto it = connections.find(connection);
                if (it != connections.end())
                    connections.erase(it);
            }
            void handle_accept(ServerConnectionS connection, const boost::system::error_code &error_code) {
                if (!keep_running)
                    return;

                if (!error_code) {
                    connection->start();
                }
                start_accept();
            }
            void start_accept() {
                auto connection = ServerConnection::Create(config, this->shared_from_this(), buffer_pool, io_context, request_dispatcher);
                add_connection(connection);
                acceptor.async_accept(connection->get_socket(),
                                      boost::bind(&Server::handle_accept, this,
                                                  connection, boost::asio::placeholders::error));
            }
        };

        static ServerS CreateServer(const typename Server::Config &config, BufferPoolS buffer_pool, IoContextS io_context,
                                    typename Server::ServerRequestDispatcher &&request_dispatcher) {
            return std::make_shared<Innerspace::Server>(config, buffer_pool, io_context, std::move(request_dispatcher));
        }
    };
}
