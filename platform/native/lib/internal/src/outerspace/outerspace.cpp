//
// Originally written by Scott R. Jones.
// Copyright (c) 2020 Warpdrive Technologies, Inc. All rights reserved.
//

#include <estate/internal/outerspace/outerspace.h>

namespace estate::outerspace {

    ServerS create_server(const Config &config, WorkerAuthenticationS worker_authentication, IoContextS io_context, BufferPoolS buffer_pool, RequestDispatcher request_dispatcher, SubscriptionManagerS subscription_manager) {
        auto shared_state = std::make_shared<SharedState>();
        return std::make_shared<Server>(config, worker_authentication, io_context, shared_state, buffer_pool, request_dispatcher, subscription_manager);
    }
    bool is_error(const beast::error_code &ec) {
        if(ec == boost::asio::error::operation_aborted || ec == beast::websocket::error::closed || ec == boost::asio::error::eof)
            return false;
        return true;
    }

    RequestContext::RequestContext(WebSocketSessionS websocket_session, WorkerId authorized_worker_id, RequestId request_id) :
            _shared_state(websocket_session->get_shared_state()),
            _websocket_session(websocket_session),
            _authorized_worker_id(authorized_worker_id),
            _request_id(request_id) {
        assert(_request_id >= ESTATE_MIN_REQUEST_ID);
    }
    RequestContext::RequestContext(RequestContext &&other) :
            _shared_state(std::move(other._shared_state)),
            _websocket_session(std::move(other._websocket_session)),
            _request_id(std::move(other._request_id)),
            _authorized_worker_id(std::move(other._authorized_worker_id)),
            _moved(false) {
        other._moved = true;
    }
    void RequestContext::async_respond(const LogContext &log_context, const BufferView<RiverUserResponseProto> &buffer) {
        assert(!_moved);
        assert(_websocket_session);
        _websocket_session->get_sender()->async_send_response(log_context, _request_id, *reinterpret_cast<const BufferView<void> *>(&buffer), SendKind::Response);
    }
    void RequestContext::async_send_to_session(const LogContext &log_context, const SessionHandle session_handle, const BufferView<RiverUserMessageProto> &buffer) {
        assert(!_moved);
        assert(_websocket_session);
        assert(_shared_state);
        const auto wp = _shared_state->get_weak_session(_authorized_worker_id, session_handle);
        if (wp.has_value()) {
            if (auto session = wp.value().lock()) {
                session->get_sender()->async_send_response(log_context, _request_id, *reinterpret_cast<const BufferView<void> *>(&buffer), SendKind::Message);
            }
        }
    }
    void RequestContext::on_forbidden() {
        assert(!_moved);
        _websocket_session->on_forbidden();
    }
    WorkerId RequestContext::authorized_worker_id() const {
        assert(!_moved);
        return _authorized_worker_id;
    }
    SessionHandle RequestContext::get_session_handle() const {
        assert(!_moved);
        assert(_websocket_session);
        return reinterpret_cast<SessionHandle>(_websocket_session.get());
    }
    Result<Sender::Bundle> Sender::create_bundle(const LogContext &log_context, RequestId request_id, const BufferView<void> &send_buffer, SendKind kind) {
        using Result = Result<Sender::Bundle>;

        const auto max_size = kind == SendKind::Response ? _response_max_size : _broadcast_max_size;

        if (send_buffer.size() > max_size) {
            log_error(log_context, "For request id {}, the message size {} was bigger than the max message size {}", request_id, send_buffer.size(), max_size);
            return Result::Error();
        }

        Envelope_ envelope = _buffer_pool->get_envelope<SendHeader, void, u32>();

        envelope.resize_for_payload(send_buffer.size());

        SendHeader *header = envelope.get_header();
        header->request_id = request_id;
        header->kind = kind;

        std::memcpy(envelope.get_payload_raw(), send_buffer.as_u8(), send_buffer.size());

        assert(envelope.get_payload_size() == send_buffer.size());
        assert(envelope.get_header_size() == sizeof(SendHeader));
        assert(envelope.get_total_size() == send_buffer.size() + sizeof(SendHeader));

        Bundle response_bundle{
                log_context,
                std::move(envelope)
        };

        return Result::Ok(std::move(response_bundle));
    }
    void Sender::on_write(beast::error_code ec, std::size_t bytes_transferred) {
        auto &log_context = _queue.front().log_context;

        if (ec) {
            if(is_error(ec))
                log_error(log_context, "Error while writing. Code {} Category {} Message {}", ec.value(), ec.category().name(), ec.message());
            _queue.pop();
            return;
        }

        _queue.pop();

        if (!_queue.empty()) {
            auto &envelope = _queue.front().envelope;
            //Send the next message
            _websocket_stream.async_write(boost::asio::buffer(envelope.get_header_raw(), envelope.get_total_size()),
                                          beast::bind_front_handler(&Sender::on_write, this->shared_from_this()));
        }
    }
    Sender::Sender(BufferPoolS buffer_pool, u32 broadcast_max_size, u32 response_max_size, WebSocketStream &websocket_stream) :
            _buffer_pool(buffer_pool),
            _broadcast_max_size(broadcast_max_size),
            _response_max_size(response_max_size),
            _websocket_stream(websocket_stream) {
    }
    void Sender::async_send_response(const LogContext &log_context, RequestId request_id, const BufferView<void> &send_buffer, SendKind kind) {
        auto bundle_r = create_bundle(log_context, request_id, send_buffer, kind);
        if (!bundle_r)
            return;
        Bundle bundle = bundle_r.unwrap();

        assert(_websocket_stream.binary());

        //posts the send to the strand so this isn't accessed concurrently.
        boost::asio::post(_websocket_stream.get_executor(),
                          beast::bind_front_handler([self{this->shared_from_this()}, r{std::move(bundle)}]() mutable {
                              self->_queue.push(std::move(r));

                              // are we already writing?
                              if (self->_queue.size() > 1)
                                  return;

                              auto &envelope = self->_queue.front().envelope;

                              //since we're not currently writing, send this immediately
                              self->_websocket_stream.async_write(boost::asio::buffer(envelope.get_header_raw(), envelope.get_total_size()),
                                                                  beast::bind_front_handler(&Sender::on_write, self));
                          }));
    }
    void SharedState::add_session(WorkerId authorized_worker_id, WebSocketSession *session) {
        std::lock_guard<std::mutex> lock(_mutex);
        _session_map[authorized_worker_id].insert(static_cast<void *>(session));
    }
    void SharedState::remove_session(WorkerId authorized_worker_id, WebSocketSession *session) {
        std::lock_guard<std::mutex> lock(_mutex);
        _session_map[authorized_worker_id].erase(static_cast<void *>(session));
        if (_session_map.count(authorized_worker_id) == 0)
            _session_map.erase(authorized_worker_id);
    }
    std::optional<std::weak_ptr<WebSocketSession>> SharedState::get_weak_session(WorkerId authorized_worker_id, const SessionHandle handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        const auto &sessions = _session_map[authorized_worker_id];
        const auto it = sessions.find(handle);
        if (it != sessions.end()) {
            auto *ptr = static_cast<WebSocketSession *>(handle);
            return ptr->weak_from_this();
        }
        return std::nullopt;
    }
    std::vector<std::weak_ptr<WebSocketSession>> SharedState::get_weak_sessions_except(WorkerId authorized_worker_id, WebSocketSession *except) {
        std::vector<std::weak_ptr<WebSocketSession>> v;
        std::lock_guard<std::mutex> lock(_mutex);
        const auto &sessions = _session_map[authorized_worker_id];
        v.reserve(sessions.size());
        for (void *p: sessions) {
            if (p != except) {
                auto *ptr = static_cast<WebSocketSession *>(p);
                v.emplace_back(ptr->weak_from_this());
            }
        }
        return v;
    }
    WebSocketSession::WebSocketSession(const Config &config, const WorkerId authorized_worker_id, boost::asio::ip::tcp::socket &&socket,
                                       const SharedStateS &shared_state, BufferPoolS buffer_pool, RequestDispatcher request_dispatcher,
                                       SubscriptionManagerS subscription_manager) :
            _config(config),
            _authorized_worker_id(authorized_worker_id),
            _websocket_stream(std::move(socket)),
            _shared_state(shared_state),
            _buffer_pool(buffer_pool),
            _request_dispatcher(request_dispatcher),
            _sender(std::make_shared<Sender>(buffer_pool, config.websocket_max_broadcast_size, config.websocket_max_response_size, _websocket_stream)),
            _subscription_manager(subscription_manager) {
        _websocket_stream.binary(true);
    }
    WebSocketSession::~WebSocketSession() {
        _subscription_manager->unsubscribe_all(_authorized_worker_id, static_cast<SessionHandle>(this));
        _shared_state->remove_session(_authorized_worker_id, this);
    }
    SharedStateS WebSocketSession::get_shared_state() {
        return _shared_state;
    }
    SenderS WebSocketSession::get_sender() {
        return _sender;
    }
    void WebSocketSession::on_forbidden() {
        _websocket_stream.close("forbidden");
    }
    void WebSocketSession::async_read_request() {
        _websocket_stream.async_read(_read_buffer.get_buffer(), beast::bind_front_handler(
                [self{this->shared_from_this()}](const boost::system::error_code error_code, const std::size_t bytes_transferred) {
                    if (error_code) {
                        if (is_error(error_code))
                            sys_log_error("Error while reading request. Code: {} Category: {} Message: {}", error_code.value(), error_code.category().name(), error_code.message());
                        return;
                    }

                    sys_log_trace("Read request of {} bytes", bytes_transferred);

                    auto header = self->_read_buffer.get_header();
                    const u32 payload_size = header->payload_size;

                    if (payload_size > self->_config.websocket_max_request_size) {
                        sys_log_error("Request payload {} was larger than max request size {}",
                                      payload_size, self->_config.websocket_max_request_size);
                        return;
                    }

                    if (!self->_read_buffer.get_payload_raw()) {
                        sys_log_error("Request payload was supposed to have {} bytes but was empty.", payload_size);
                        return;
                    }

                    RequestId request_id = header->request_id;

                    //Copy the buffer out of the temporary buffer
                    auto buffer = self->_buffer_pool->template get_buffer<UserRequestProto>();
                    buffer.resize(self->_read_buffer.get_payload_size());
                    std::memcpy(buffer.as_u8(), self->_read_buffer.get_payload_raw(), self->_read_buffer.get_payload_size());

                    //Consume the bytes in the temporary buffer
                    self->_read_buffer.consume_all();

                    //Dispatch the request
                    self->_request_dispatcher(self, self->_authorized_worker_id, request_id, std::move(buffer));

                    sys_log_trace("Request {} dispatched successfully", request_id);

                    //Read another one
                    self->async_read_request();
                }));
    }
    void WebSocketSession::on_accept(beast::error_code ec) {
        if (ec) {
            if (is_error(ec))
                sys_log_error("Error while accepting connection: {}", ec.message());
            return;
        }

        _shared_state->add_session(_authorized_worker_id, this);
        async_read_request();
    }
    WorkerId WebSocketSession::authorized_worker_id() const {
        return _authorized_worker_id;
    }
    HttpSession::HttpSession(const Config &config, WorkerAuthenticationS worker_authentication, boost::asio::ip::tcp::socket &&socket, const SharedStateS &shared_state, BufferPoolS buffer_pool, RequestDispatcher websocket_request_dispatcher,
                             SubscriptionManagerS subscription_manager) :
            _config(config), _worker_authentication(worker_authentication), _stream(std::move(socket)),
            _shared_state(shared_state), _buffer_pool(buffer_pool),
            _websocket_request_dispatcher(websocket_request_dispatcher),
            _subscription_manager{subscription_manager} {
    }
    void HttpSession::start() {
        do_read();
    }
    void HttpSession::report_error(beast::error_code ec, const char *what) {
        if (ec == boost::asio::error::operation_aborted)
            return;
        sys_log_error("Failed to {} because {}", what, ec.message());
    }
    void HttpSession::do_read() {
        _parser.emplace();
        _parser->body_limit(_config.http_session_body_limit);
        _stream.expires_after(_config.http_session_expiration);
        beast::http::async_read(_stream, _buffer, _parser->get(),
                                beast::bind_front_handler(&HttpSession::on_read,
                                                          this->shared_from_this()));
    }
    void HttpSession::on_read(beast::error_code ec, std::size_t sz) {
        //connection closed
        if (ec == beast::http::error::end_of_stream) {
            _stream.socket().shutdown(Tcp::socket::shutdown_send, ec);
            return;
        }

        if (ec) {
            report_error(ec, "read");
            return;
        }

        if (beast::websocket::is_upgrade(_parser->get())) {
            const auto req = _parser->get();
            const auto target = req.target();

            static const char *USER_KEY_QUERY = "/?user_key=";
            static size_t USER_KEY_QUERY_LEN = strlen(USER_KEY_QUERY);

            //NOTE: No logging when unauthorized because I don't want it to overburden this server.
            //However, I'm going to have to have some kind of DDoS protection to prevent many invalid calls from the same IP.

            if (target.length() != ESTATE_USER_KEY_LENGTH + USER_KEY_QUERY_LEN) {
                HandleUnauthorized(_parser->release(), SendLambda(*this));
                return;
            }

            if (target.rfind(USER_KEY_QUERY, 0) != 0) {
                HandleUnauthorized(_parser->release(), SendLambda(*this));
                return;
            }

            const auto user_key = target.substr(USER_KEY_QUERY_LEN, ESTATE_USER_KEY_LENGTH);
            auto worker_id_r = _worker_authentication->try_get_workerid_by_user_key(user_key);
            if (!worker_id_r) {
                HandleUnauthorized(_parser->release(), SendLambda(*this));
                return;
            }

            const auto authorized_worker_id = worker_id_r.unwrap();

            //good
            std::make_shared<WebSocketSession>(_config, authorized_worker_id, _stream.release_socket(),
                                               _shared_state, _buffer_pool, _websocket_request_dispatcher,
                                               _subscription_manager)
                    ->start(_parser->release());
            return;
        } else {
            HandleOk(_parser->release(), SendLambda(*this));
            return;
        }
    }
    void HttpSession::on_write(beast::error_code ec, std::size_t sz, bool close) {
        if (ec) {
            report_error(ec, "write");
            return;
        }

        if (close) {
            _stream.socket().shutdown(Tcp::socket::shutdown_send, ec);
            return;
        }

        do_read();
    }
    Server::Server(const Config config, WorkerAuthenticationS worker_authentication, IoContextS io_context, SharedStateS shared_state, BufferPoolS buffer_pool, RequestDispatcher request_dispatcher, SubscriptionManagerS subscription_manager) :
            _config(config), _worker_authentication(worker_authentication), _io_context(io_context), _acceptor(*io_context),
            _shared_state(std::move(shared_state)), _buffer_pool(buffer_pool), _request_dispatcher(request_dispatcher),
            _subscription_manager(subscription_manager) {
        beast::error_code ec;

        // Open the acceptor
        _acceptor.open(config.listen_endpoint.protocol(), ec);
        if (ec) {
            report_error(ec, "open");
            return;
        }
        _acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec) {
            report_error(ec, "set_option");
            return;
        }

        //Bind to the server's address
        _acceptor.bind(config.listen_endpoint, ec);
        if (ec) {
            report_error(ec, "bind");
            return;
        }

        //Start listening for connections
        _acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            report_error(ec, "listen");
            return;
        }
    }
    void Server::report_error(beast::error_code ec, const char *what) {
        if (ec == boost::asio::error::operation_aborted)
            return;
        sys_log_error("Failed to {} because {}", what, ec.message());
    }
    void Server::on_accept(beast::error_code ec, Tcp::socket socket) {
        if (ec) {
            report_error(ec, "accept");
            return;
        } else {
            std::make_shared<HttpSession>(_config, _worker_authentication, std::move(socket), _shared_state,
                                          _buffer_pool, _request_dispatcher, _subscription_manager)->start();
        }

        _acceptor.async_accept(boost::asio::make_strand(*_io_context),
                               beast::bind_front_handler(&Server::on_accept, this->shared_from_this()));
    }
    void Server::start() {
        _acceptor.async_accept(boost::asio::make_strand(*_io_context),
                               beast::bind_front_handler(&Server::on_accept, this->shared_from_this()));
    }
}
