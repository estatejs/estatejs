//
// Created by scott on 5/15/20.
//
#pragma once

#include "handler.h"

#include <estate/internal/outerspace/outerspace.h>
#include <estate/internal/thread_pool.h>
#include <estate/internal/buffer_pool.h>

#include <estate/runtime/numeric_types.h>
#include <estate/runtime/version.h>
#include <estate/internal/deps/boost.h>

#include <iostream>

namespace estate {
    template<typename TProcessor>
    struct RiverSystem {
        using ProcessorConfig = typename TProcessor::Config;
        using ServiceProvider = typename TProcessor::ServiceProvider;

        template<typename... SPArgs>
        void init(const ProcessorConfig &processor_config,
                  const outerspace::Config &outerspace_config,
                  BufferPoolS buffer_pool,
                  ThreadPoolS thread_pool,
                  WorkerAuthenticationS worker_authentication,
                  SPArgs... spargs) {
            assert(!has_init);
            service_provider = std::make_shared<ServiceProvider>(std::forward<SPArgs>(spargs)...);
            processor = std::make_shared<TProcessor>(processor_config, service_provider);
            outerspace_server = outerspace::create_server(outerspace_config, worker_authentication, thread_pool->get_context(), buffer_pool,
                                               [p{processor}](outerspace::WebSocketSessionS websocket_session, WorkerId authorized_worker_id, RequestId request_id, Buffer<UserRequestProto> &&request_buffer) mutable {
                                                   auto request_context = std::make_shared<outerspace::RequestContext>(websocket_session, authorized_worker_id, request_id);
                                                   p->post(std::move(request_buffer), request_context);
                                               }, service_provider->get_subscription_manager());
            has_init = true;
        }
        void start() {
            outerspace_server->start();
        }
    private:
        std::shared_ptr<ServiceProvider> service_provider{};
        std::shared_ptr<TProcessor> processor{};
        outerspace::ServerS outerspace_server{};
        std::atomic_bool has_init{false};
    };
    struct River {
        struct Config {
            BufferPoolConfig buffer_pool_config{true};
            ThreadPoolConfig thread_pool_config{};
            WorkerAuthenticationConfig worker_authentication_config{};
            LoggingConfig logging_config{};
            RiverProcessorConfig processor_config{};
            outerspace::Config outerspace_config{};
            innerspace::InnerspaceClientConfig::AsWorkerUser innerspace_client_config{};
            innerspace::InnerspaceWorkerLoaderClientConfig innerspace_worker_loader_client_config{};
        };
        bool has_init{false};

        outerspace::SubscriptionManagerS subscription_manager{};
        ThreadPoolS thread_pool{};
        BufferPoolS buffer_pool{};
        WorkerAuthenticationS worker_authentication;
        innerspace::InnerspaceClientS innerspace_client;
        std::atomic_bool keep_running_daemon;

        RiverSystem<RiverProcessor> river_system;

        static Config LoadConfig();
        void shutdown();
        void init(const Config &config);
        void run(bool daemon);
    };
}
