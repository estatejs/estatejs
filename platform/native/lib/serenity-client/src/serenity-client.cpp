//
// Created by scott on 5/6/20.
//

#include "serenity-client.h"

#include <iostream>

#include <estate/internal/innerspace/innerspace-client.h>
#include <estate/internal/local_config.h>
#include <estate/internal/thread_pool.h>
#include <estate/internal/buffer_pool.h>
#include <estate/internal/logging.h>

using namespace estate;

struct Setup {
    Setup(ThreadPoolS thread_pool,
          BufferPoolS buffer_pool,
          innerspace::InnerspaceClientConfig::AsWorkerAdmin innerspace_client_config,
          Innerspace<GetWorkerProcessEndpointRequestProto, GetWorkerProcessEndpointResponseProto>::Client::Config worker_loader_config) :
            thread_pool{std::move(thread_pool)},
            buffer_pool{std::move(buffer_pool)},
            innerspace_client_config(std::move(innerspace_client_config)),
            worker_loader_config{std::move(worker_loader_config)} {}
    ThreadPoolS thread_pool;
    BufferPoolS buffer_pool;
    const Innerspace<GetWorkerProcessEndpointRequestProto, GetWorkerProcessEndpointResponseProto>::Client::Config worker_loader_config;
    const innerspace::InnerspaceClientConfig::AsWorkerAdmin innerspace_client_config;
};

static std::shared_ptr<Setup> _setup{};

innerspace::InnerspaceClientS create_client() {
    assert(_setup);
    return std::make_shared<innerspace::InnerspaceClient>(_setup->buffer_pool,
                                                          _setup->thread_pool->get_context(),
                                                          _setup->worker_loader_config,
                                                          _setup->innerspace_client_config);
}

void init(const char* config_file) {
    auto config = LocalConfiguration::FromFile(config_file);
    auto logging_config = LoggingConfig::FromRemote(config.create_reader("Logging"));
    init_logging(logging_config, true);

    if(_setup) {
        sys_log_error("Attempted to re-init");
        return;
    }

    auto thread_pool_config = ThreadPoolConfig::FromRemote(config.create_reader("ThreadPool"));
    auto thread_pool = std::make_shared<ThreadPool>(thread_pool_config);
    thread_pool->start();

    auto buffer_pool_config = BufferPoolConfig::FromRemote(config.create_reader("BufferPool"));
    auto buffer_pool = std::make_shared<BufferPool>(buffer_pool_config);

    auto worker_loader_config = innerspace::InnerspaceWorkerLoaderClientConfig::FromRemote(config.create_reader("InnerspaceWorkerLoader"));
    auto innerspace_client_config = innerspace::InnerspaceClientConfig::AsWorkerAdmin::FromRemote(config.create_reader("InnerspaceClient"));

    _setup = std::make_shared<Setup>(std::move(thread_pool), std::move(buffer_pool), std::move(innerspace_client_config), std::move(worker_loader_config));

    sys_log_info("Serenity-Client initialization complete");
}

void send_setup_worker_request(const char *log_context_str, const WorkerId worker_id, const u8 *buffer, size_t buffer_size, OnResponseCallback callback) {
    using Innerspace = Innerspace<SetupWorkerRequestProto, SetupWorkerResponseProto>;

    assert(_setup);

    LogContext log_context{log_context_str};

    auto buffer_ = _setup->buffer_pool->get_buffer<SetupWorkerRequestProto>();
    buffer_.resize(buffer_size);
    std::memcpy(buffer_.as_char(), buffer, buffer_size);

    log_trace(log_context, "sending request");

    auto client = create_client();

    client->async_send(log_context, worker_id, std::move(buffer_), [callback, log_context](ResultCode<Innerspace::ResponseEnvelope> response_r) {
        log_trace(log_context, "response received");
        if(!response_r) {
            callback(response_r.get_error(), 0, 0);
            return;
        }
        auto response = response_r.unwrap();
        const auto *response_bytes = response.get_payload_raw();
        const auto response_size = response.get_payload_size();
        callback(Code::Ok, response_bytes, response_size);
    });
}

void send_delete_worker_request(const char *log_context_str, const WorkerId worker_id, const u8 *buffer, size_t buffer_size, OnResponseCallback callback) {
    using Innerspace = Innerspace<DeleteWorkerRequestProto, DeleteWorkerResponseProto>;

    assert(_setup);

    LogContext log_context{log_context_str};

    auto buffer_ = _setup->buffer_pool->get_buffer<DeleteWorkerRequestProto>();
    buffer_.resize(buffer_size);
    std::memcpy(buffer_.as_char(), buffer, buffer_size);

    log_trace(log_context, "sending request");

    auto client = create_client();

    client->async_send(log_context, worker_id, std::move(buffer_), [callback, log_context](ResultCode<Innerspace::ResponseEnvelope> response_r) {
        log_trace(log_context, "response received");
        if (!response_r) {
            callback(response_r.get_error(), 0, 0);
            return;
        }
        auto response = response_r.unwrap();
        const auto *response_bytes = response.get_payload_raw();
        const auto response_size = response.get_payload_size();
        callback(Code::Ok, response_bytes, response_size);
    });
}
