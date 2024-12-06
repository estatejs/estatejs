//
// Created by scott on 5/15/20.
//

#pragma once

#include "estate/internal/outerspace/outerspace-fwd.h"
#include "estate/internal/outerspace/subscription.h"

#include "estate/internal/stopwatch.h"
#include "estate/internal/logging.h"
#include "estate/internal/buffer_pool.h"
#include "estate/internal/worker_authentication.h"

#include <estate/runtime/result.h>
#include <estate/internal/deps/boost.h>
#include <estate/runtime/version.h>
#include <estate/runtime/model_types.h>
#include <estate/runtime/limits.h>

#include <memory>
#include <unordered_set>
#include <queue>
#include <estate/internal/net_util.h>
#include <estate/runtime/protocol/interface_generated.h>

namespace estate::outerspace {
#define BOOST_SYSTEM_ERROR_END_OF_FILE (2)
#define OUTERSPACE_VERSION_STRING "Estate/" ESTATE_VERSION
    namespace beast = boost::beast;
    using WebSocketStream = beast::websocket::stream<beast::tcp_stream>;
    using Tcp = boost::asio::ip::tcp;
    using Acceptor = boost::asio::ip::tcp::acceptor;

    struct Config {
        u64 http_session_body_limit;
        std::chrono::seconds http_session_expiration;
        size_t websocket_max_request_size;
        size_t websocket_max_response_size;
        size_t websocket_max_broadcast_size;
        //stored as listen_ip and listen_port
        Tcp::endpoint listen_endpoint;

        static Config FromRemote(const LocalConfigurationReader &reader) {
            const std::string listen_ip = reader.get_string("listen_ip");
            const u16 listen_port = reader.get_u16("listen_port");

            return Config{
                    reader.get_u64("http_session_body_limit"),
                    std::chrono::seconds(reader.get_u64("http_session_expiration_sec")),
                    reader.get_u64("websocket_max_request_size"),
                    reader.get_u64("websocket_max_response_size"),
                    reader.get_u64("websocket_max_broadcast_size"),
                    make_endpoint(listen_ip, listen_port)
            };
        }
    };

    /*This is a wrapper around a beast::flat_buffers*/
    template<typename THeader, typename TPayload, typename TPayloadSize>
    class EnvelopeBuffer {
        struct _Envelope {
            THeader header;
            TPayload *payload;
        };
        beast::flat_buffer _buffer;
    public:
        EnvelopeBuffer() = default;
        EnvelopeBuffer(const EnvelopeBuffer &other) = delete;
        EnvelopeBuffer(EnvelopeBuffer &&other) : _buffer(std::move(other._buffer)) {}
        BufferView<TPayload> get_payload_view() {
            return BufferView<TPayload>{
                    get_payload_raw(),
                    get_payload_size()
            };
        }
        beast::flat_buffer &get_buffer() {
            return _buffer;
        }
        void consume_all() {
            _buffer.consume(_buffer.size());
        }
        THeader *get_header() {
            assert(_buffer.size() >= sizeof(THeader));
            return reinterpret_cast<THeader *>(_buffer.data().data());
        }
        const THeader *get_header() const {
            assert(_buffer.size() >= sizeof(THeader));
            return reinterpret_cast<const THeader *>(_buffer.cdata().data());
        }
        u8 *get_header_raw() {
            assert(_buffer.size() >= sizeof(THeader));
            return reinterpret_cast<u8 *>(_buffer.data().data());
        }
        const u8 *get_header_raw() const {
            assert(_buffer.size() >= sizeof(THeader));
            return reinterpret_cast<const u8 *>(_buffer.cdata().data());
        }
        u8 *get_payload_raw() {
            assert(_buffer.size() > sizeof(THeader));
            return reinterpret_cast<u8 *>((std::size_t) _buffer.data().data() + sizeof(THeader));
        }
        const TPayload *get_payload() {
            assert(_buffer.size() > sizeof(THeader));
            return flatbuffers::GetRoot<TPayload>(get_payload_raw());
        }
        constexpr size_t get_header_size() const {
            return sizeof(THeader);
        }
        TPayloadSize get_payload_size() const {
            assert(_buffer.size() > sizeof(THeader));
            return _buffer.size() - sizeof(THeader);
        }
        [[nodiscard]] size_t get_total_size() const {
            return _buffer.size();
        }
    };

    enum class SendKind : u8 {
        Invalid = 0,
        Response = 1,
        Message = 2,
    };

#pragma pack(push, 1)
    struct RequestHeader {
        RequestId request_id;
        u32 payload_size;
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct SendHeader {
        RequestId request_id; //0 if no request_id
        SendKind kind;
    };
#pragma pack(pop)

    class Sender : public std::enable_shared_from_this<Sender> {
    public:
        using Envelope_ = Envelope<SendHeader, void, u32>;
    private:
        struct Bundle {
            const LogContext log_context;
            Envelope_ envelope;
        };
        const u32 _broadcast_max_size;
        const u32 _response_max_size;
        BufferPoolS _buffer_pool;
        WebSocketStream &_websocket_stream;
        std::queue<Bundle> _queue;
        Result<Bundle> create_bundle(const LogContext &log_context, RequestId request_id, const BufferView<void> &send_buffer, SendKind kind);
        void on_write(beast::error_code ec, std::size_t bytes_transferred);
    public:
        Sender(BufferPoolS buffer_pool, u32 broadcast_max_size, u32 response_max_size, WebSocketStream &websocket_stream);
        void async_send_response(const LogContext &log_context, RequestId request_id, const BufferView<void> &send_buffer, SendKind kind);
    };
    using SenderS = std::shared_ptr<Sender>;

    class WebSocketSession;
    using WebSocketSessionS = std::shared_ptr<WebSocketSession>;

    struct SharedState {
        void add_session(WorkerId authorized_worker_id, WebSocketSession *session);
        void remove_session(WorkerId authorized_worker_id, WebSocketSession *session);
        std::optional<std::weak_ptr<WebSocketSession>> get_weak_session(WorkerId authorized_worker_id, const SessionHandle handle);
        std::vector<std::weak_ptr<WebSocketSession>> get_weak_sessions_except(WorkerId authorized_worker_id, WebSocketSession *except);
    private:
        std::mutex _mutex;
        std::unordered_map<WorkerId, std::unordered_set<void *>> _session_map{};
    };
    using SharedStateS = std::shared_ptr<SharedState>;

    struct RequestContext {
        RequestContext(WebSocketSessionS websocket_session, WorkerId authorized_worker_id, RequestId request_id);
        RequestContext(RequestContext &&other);
        RequestContext(const RequestContext &other) = delete;
        void async_respond(const LogContext &log_context, const BufferView<RiverUserResponseProto> &buffer);
        void async_send_to_session(const LogContext &log_context, const SessionHandle session_handle, const BufferView<RiverUserMessageProto> &buffer);
        void on_forbidden();
        WorkerId authorized_worker_id() const;
        SessionHandle get_session_handle() const;
    private:
        WebSocketSessionS _websocket_session;
        SharedStateS _shared_state;
        RequestId _request_id;
        WorkerId _authorized_worker_id;
        bool _moved{false};
    };


    using RequestDispatcher = std::function<void(WebSocketSessionS websocket_session,
                                                 WorkerId authorized_worker_id,
                                                 RequestId request_id,
                                                 Buffer<UserRequestProto> &&payload)>;

    bool is_error(const beast::error_code& ec);

    struct WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
        WebSocketSession(const Config &config, const WorkerId authorized_worker_id, Tcp::socket &&socket, SharedStateS const &shared_state,
                         BufferPoolS buffer_pool, RequestDispatcher request_dispatcher, SubscriptionManagerS subscription_manager);
        ~WebSocketSession();
        SharedStateS get_shared_state();
        SenderS get_sender();
        template<class Body, class Allocator>
        void start(beast::http::request<Body, beast::http::basic_fields<Allocator>> request) {
            _websocket_stream.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::server));

            //Change the server of the handshake
            _websocket_stream.set_option(beast::websocket::stream_base::decorator([](beast::websocket::response_type &res) {
                res.set(beast::http::field::server, std::string(OUTERSPACE_VERSION_STRING) + " river");
            }));

            //Accept the websocket handshake
            _websocket_stream.async_accept(request, beast::bind_front_handler(&WebSocketSession::on_accept, this->shared_from_this()));
        }
        void on_forbidden();
    private:
        void async_read_request();
        void on_accept(beast::error_code ec);
        WorkerId authorized_worker_id() const;
    private:
        EnvelopeBuffer<RequestHeader, UserRequestProto, u32> _read_buffer;
        WebSocketStream _websocket_stream;
        SenderS _sender;
        SharedStateS _shared_state;
        BufferPoolS _buffer_pool;
        const Config &_config;
        RequestDispatcher _request_dispatcher;
        WorkerId _authorized_worker_id;
        SubscriptionManagerS _subscription_manager;
    };

    struct HttpSession : public std::enable_shared_from_this<HttpSession> {
        HttpSession(const Config &config, WorkerAuthenticationS worker_authentication,
                    Tcp::socket &&socket, SharedStateS const &shared_state, BufferPoolS buffer_pool, RequestDispatcher websocket_request_dispatcher,
                    SubscriptionManagerS subscription_manager);
        void start();
    private:
        template<class Body, class Allocator, class Send>
        static void HandleUnauthorized(beast::http::request<Body, beast::http::basic_fields<Allocator>> &&req, Send &&send) {
            beast::http::response<beast::http::string_body> res{beast::http::status::unauthorized, req.version()};
            res.set(beast::http::field::server, OUTERSPACE_VERSION_STRING);
            res.set(beast::http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.prepare_payload();
            return send(std::move(res));
        }

        template<class Body, class Allocator, class Send>
        static void HandleOk(beast::http::request<Body, beast::http::basic_fields<Allocator>> &&req, Send &&send) {
            beast::http::response<beast::http::string_body> res{beast::http::status::ok, req.version()};
            res.set(beast::http::field::server, OUTERSPACE_VERSION_STRING);
            res.set(beast::http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.prepare_payload();
            return send(std::move(res));
        }
    private:
        void report_error(beast::error_code ec, const char *what);
        void do_read();
        void on_read(beast::error_code ec, std::size_t sz);
        void on_write(beast::error_code ec, std::size_t sz, bool close);
    private:
        struct SendLambda {
            HttpSession &_http_session;

            explicit SendLambda(HttpSession &self) : _http_session(self) {
            }

            template<bool isRequest, class Body, class Fields>
            void
            operator()(beast::http::message<isRequest, Body, Fields> &&msg) const {
                // The lifetime of the message has to extend
                // for the duration of the async operation so
                // we use a shared_ptr to manage it.
                auto sp = std::make_shared<beast::http::message<isRequest, Body, Fields>>(std::move(msg));

                // Write the response
                auto http_session = _http_session.shared_from_this();
                beast::http::async_write(
                        _http_session._stream,
                        *sp,
                        [http_session, sp](beast::error_code ec, std::size_t bytes) {
                            http_session->on_write(ec, bytes, sp->need_eof());
                        });
            }
        };
    private:
        const Config &_config;
        WorkerAuthenticationS _worker_authentication;
        beast::tcp_stream _stream;
        beast::flat_buffer _buffer;
        SharedStateS _shared_state;
        std::optional<beast::http::request_parser<beast::http::string_body>> _parser;
        BufferPoolS _buffer_pool;
        RequestDispatcher _websocket_request_dispatcher;
        SubscriptionManagerS _subscription_manager;
    };

    struct Server : public std::enable_shared_from_this<Server> {
    public:
        Server(const Config config, WorkerAuthenticationS worker_authentication, IoContextS io_context, SharedStateS shared_state,
               BufferPoolS buffer_pool, RequestDispatcher request_dispatcher, SubscriptionManagerS subscription_manager);
        void report_error(beast::error_code ec, const char *what);
        void on_accept(beast::error_code ec, Tcp::socket socket);
        void start();
    private:
        const Config _config;
        WorkerAuthenticationS _worker_authentication;
        IoContextS _io_context;
        Acceptor _acceptor;
        SharedStateS _shared_state;
        BufferPoolS _buffer_pool;
        RequestDispatcher _request_dispatcher;
        SubscriptionManagerS _subscription_manager;
    };
    using ServerS = std::shared_ptr<Server>;

    ServerS create_server(const Config &config, WorkerAuthenticationS worker_authentication, IoContextS io_context, BufferPoolS buffer_pool, RequestDispatcher request_dispatcher, SubscriptionManagerS subscription_manager);
}
