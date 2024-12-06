#include "../estate_test.h"
#include "gtest/gtest.h"

#include <estate/internal/innerspace/innerspace.h>
#include <estate/internal/net_util.h>
#include <estate/internal/flatbuffers_util.h>
#include "../logging.h"

using namespace estate;

TEST(contract_innerspace_tests, SingleRequest) {
    using Innerspace = Innerspace<DeleteWorkerRequestProto, DeleteWorkerResponseProto>;

    BufferPoolConfig buffer_pool_config{
            false
    };
    auto buffer_pool = std::make_shared<BufferPool>(buffer_pool_config);

    //Create the server and start it
    ThreadPoolConfig server_thread_pool_config = ThreadPoolConfig::Half();
    auto server_thread_pool = std::make_shared<ThreadPool>(server_thread_pool_config);
    server_thread_pool->start();

    Innerspace::Server::Config server_config{
            make_endpoint("0.0.0.0", 50000),
            1024,
            100
    };
    auto server = Innerspace::CreateServer(
            server_config, buffer_pool, server_thread_pool->get_context(),
            [buffer_pool](Innerspace::Server::ServerConnectionS connection, Innerspace::RequestEnvelope &&request_envelope) {
                const DeleteWorkerRequestProto *request = request_envelope.get_payload();

                LogContext lc{
                        std::move(request->log_context()->str())
                };

                const auto &lc_str = lc.get_context();
                ASSERT_EQ(lc_str, std::string("REQ"));

                auto request_id = request_envelope.get_header()->request_id;

                log_info(lc, "Request id {} handled with worker_id {} worker_version {}",
                         request_id, request->worker_id(), request->worker_version());

                auto response_proto_buffer = buffer_pool->get_buffer<DeleteWorkerResponseProto>();

                fbs::Builder builder{};

                auto response_buffer = finish_and_copy_to_buffer<DeleteWorkerResponseProto>(builder,
                                                                                          buffer_pool,
                                                                                          CreateDeleteWorkerResponseProto(builder));

                connection->async_send_response(std::move(lc), request_id, response_buffer.get_view(), std::nullopt);
            });
    server->start();

    auto test_log_context = make_test_log_context;

    //Create the client and send a request
    ThreadPoolConfig thread_pool_config = ThreadPoolConfig::Half();
    auto client_thread_pool = std::make_shared<ThreadPool>(thread_pool_config);
    client_thread_pool->start();

    Innerspace::Client::Config config{
            "localhost",
            50000,
            4,
            1024,
            100
    };
    auto client = Innerspace::CreateClient(config, buffer_pool, client_thread_pool->get_context(), std::make_shared<Breaker>());

    auto conn = client->get_connection(test_log_context).unwrap();

    fbs::Builder builder{};
    auto proto = CreateDeleteWorkerRequestProtoDirect(builder, "REQ", 1005, 27);
    builder.Finish(proto);

    auto sz = builder.GetSize();

    auto proto_buffer = buffer_pool->get_buffer<DeleteWorkerRequestProto>();
    proto_buffer.resize(sz);
    std::memcpy(proto_buffer.as_u8(), builder.GetBufferPointer(), builder.GetSize());

    Event received_response{};

    conn->async_send(test_log_context, proto_buffer.get_view(),
                     [&received_response, &test_log_context](ResultCode<Innerspace::ResponseEnvelope> envelope_r) {
                         //executed after response is received
                         ASSERT_TRUE(envelope_r);
                         auto envelope = envelope_r.unwrap();
                         const auto *response = envelope.get_payload();
                         ASSERT_FALSE(response->error());
                         log_info(test_log_context, "Received DeleteWorker response");
                         received_response.notify_one();
                     });

    log_info(test_log_context, "Waiting for response");
    received_response.wait_one();

    log_info(test_log_context, "Waiting for client to shutdown");
    client->shutdown();
    log_info(test_log_context, "Waiting for server to shutdown");
    server->shutdown();

    server_thread_pool->shutdown();
    client_thread_pool->shutdown();
}

TEST(contract_innerspace_tests, TenThousandRequests) {
    init_performance_logging_level();

    using Innerspace = Innerspace<DeleteWorkerRequestProto, DeleteWorkerResponseProto>;

    const int count = 10000;
    const int connections = 10;

    //Create the server and start it
    ThreadPoolConfig server_thread_pool_config = ThreadPoolConfig::Half();
    auto server_thread_pool = std::make_shared<ThreadPool>(server_thread_pool_config);
    server_thread_pool->start();

    Innerspace::Server::Config server_config{
            make_endpoint("0.0.0.0", 50000),
            1024,
            100
    };

    std::atomic_int responses_sent{0};

    BufferPoolConfig buffer_pool_config{false};

    auto buffer_pool = std::make_shared<BufferPool>(buffer_pool_config);

    auto server = Innerspace::CreateServer(server_config, buffer_pool, server_thread_pool->get_context(),
                                           [buffer_pool, &responses_sent](Innerspace::Server::ServerConnectionS connection,
                                                                          Innerspace::RequestEnvelope request_envelope) {
                                               // Handle each request
                                               const DeleteWorkerRequestProto *request = request_envelope.get_payload();
                                               const auto lc_str = request->log_context()->str();
                                               ASSERT_EQ(lc_str, "REQ");
                                               LogContext lc{std::move(lc_str)};

                                               fbs::Builder builder{};
                                               auto response_buffer = finish_and_copy_to_buffer(
                                                       builder, buffer_pool,
                                                       CreateDeleteWorkerResponseProto(builder,
                                                                                     CreateErrorCodeResponseProto(builder,
                                                                                                                  request->worker_id() +
                                                                                                                  request->worker_version())));

                                               connection->async_send_response(std::move(lc), request_envelope.get_header()->request_id,
                                                                               response_buffer.get_view(), std::nullopt);
                                               responses_sent.fetch_add(1);
                                           });
    server->start();

    //Create the client and send a request
    ThreadPoolConfig thread_pool_config = ThreadPoolConfig::Half();
    auto client_thread_pool = std::make_shared<ThreadPool>(thread_pool_config);
    client_thread_pool->start();

    Innerspace::Client::Config config{
            "localhost",
            50000,
            connections,
            1024,
            100
    };
    auto client = Innerspace::CreateClient(config, buffer_pool, client_thread_pool->get_context(), std::make_shared<Breaker>());
    auto test_log_context = make_test_log_context;

    fbs::Builder builder{1024};

    log_trace(test_log_context, "Sending requests");
    Event received_all{};
    std::atomic_int errors_received{0};
    std::atomic_int responses_received{0};
    Innerspace::Client::ConnectionS prev_conn{};
    Innerspace::Client::ConnectionS conn{};

    Stopwatch sw{test_log_context};
    for (int i = 0; i < count; ++i) {
        builder.Clear();
        auto v = i % 10;

        auto log_context_str = fmt::format("REQ", i);

        auto proto_buffer = finish_and_copy_to_buffer(builder, buffer_pool,
                                                      CreateDeleteWorkerRequestProtoDirect(builder,
                                                                                         log_context_str.c_str(), v, v * 2));

        if (conn) {
            prev_conn = conn;
        }

        auto conn_r = client->get_connection(test_log_context);
        ASSERT_TRUE(conn_r);
        conn = conn_r.unwrap();

        if (prev_conn) {
            ASSERT_NE(prev_conn.get(), conn.get());
        }

        conn->async_send(test_log_context, proto_buffer.get_view(), [&test_log_context, &errors_received, &received_all, &responses_received, v]
                (ResultCode<Innerspace::ResponseEnvelope> envelope_r) {
            if (envelope_r) {
                auto envelope = envelope_r.unwrap();
                const auto *response = envelope.get_payload();
                ASSERT_TRUE(response->error());
                ASSERT_EQ(response->error()->error_code(), v + (v * 2));
                if (responses_received.fetch_add(1) == count - 1)
                    received_all.notify_one();
            } else {
                log_error(test_log_context, "Received an error: {}", get_code_name(envelope_r.get_error()));
                ++errors_received;
            }
        });
    }

    received_all.wait_one();
    sw.log_elapsed("Requests", count);

    ASSERT_EQ(0, errors_received);

    if (responses_sent.load() != count || responses_received.load() != responses_sent.load()) {
        log_error(test_log_context, "Sent {} and received {} responses ", responses_sent.load(), responses_received.load());
        ASSERT_TRUE(false);
    }

    log_info(test_log_context, "Sent {} and received {} responses ", responses_sent.load(), responses_received.load());

    ASSERT_NO_FATAL_FAILURE(client->shutdown());
    ASSERT_NO_FATAL_FAILURE(server->shutdown());
    ASSERT_NO_FATAL_FAILURE(server_thread_pool->shutdown());
    ASSERT_NO_FATAL_FAILURE(client_thread_pool->shutdown());

    init_default_logging_level();
}
