//
// Created by scott on 1/11/2022.
//
#pragma once

#include "estate/internal/serenity/handler/user.h"
#include "estate/internal/serenity/handler/admin.h"

#include "estate/runtime/numeric_types.h"
#include "estate/internal/thread_pool.h"
#include "estate/internal/server/server.h"
#include "estate/runtime/version.h"
#include <iostream>
#include "estate/internal/deps/boost.h"

namespace estate {
    template<typename TProcessor, typename TInnerspace>
    struct WorkerProcessSystem {
        using ServerConfig = typename TInnerspace::Server::Config;
        using Server = typename TInnerspace::Server;
        using ProcessorConfig = typename TProcessor::Config;
        using ServiceProvider = typename TProcessor::ServiceProvider;

        template<typename... SPArgs>
        void init(const ProcessorConfig &processor_config,
                  const ServerConfig &server_config,
                  BufferPoolS buffer_pool,
                  ThreadPoolS thread_pool,
                  SPArgs... spargs) {
            using ConnectionS = typename TInnerspace::Server::ServerConnectionS;
            using RequestEnvelope = typename TInnerspace::RequestEnvelope;
            using RequestContext = typename TProcessor::RequestContext;

            assert(!has_init);
            service_provider = std::make_shared<ServiceProvider>(std::forward<SPArgs>(spargs)...);
            processor = std::make_shared<TProcessor>(processor_config, service_provider);
            server = TInnerspace::CreateServer(server_config, buffer_pool, thread_pool->get_context(),
                                               [m = processor](ConnectionS connection, RequestEnvelope &&request_envelope) mutable {
                                                   auto request_context = std::make_shared<RequestContext>(connection,
                                                                                                           request_envelope.get_header()->request_id);
                                                   m->post(std::move(request_envelope), std::move(request_context));
                                               });
            has_init = true;
        }
        void start() {
            server->start();
        }
        void shutdown() {
            server->shutdown();
        }

    private:
        std::shared_ptr<ServiceProvider> service_provider{};
        std::shared_ptr<TProcessor> processor{};
        std::unique_ptr<TInnerspace> innerspace{};
        std::shared_ptr<Server> server{};
        std::atomic_bool has_init{false};
    };
    struct WorkerProcess : public std::enable_shared_from_this<WorkerProcess> {
        enum class SupportedCommand : u8 {
            None = 0,
            User = 1,
            DeleteWorker = 2,
            SetupWorker = 4
        };
        struct Config {
            BufferPoolConfig buffer_pool_config{true};
            ThreadPoolConfig thread_pool_config{};
            engine::javascript::JavascriptConfig javascript_config{};
            storage::DatabaseManagerConfiguration db_config{};
            LoggingConfig logging_config{};
            SupportedCommand supported_commands{};
            UserProcessorConfig user_processor_config{};
            UserInnerspace::Server::Config user_server_config{};
            SetupWorkerProcessorConfig setup_worker_processor_config{};
            SetupWorkerInnerspace::Server::Config setup_worker_server_config{};
            DeleteWorkerProcessorConfig delete_worker_processor_config{};
            DeleteWorkerInnerspace::Server::Config delete_worker_server_config{};
            bool has_command(SupportedCommand command) const;
        };
        bool has_init{false};

        storage::DatabaseManagerS database_manager{};
        ThreadPoolS thread_pool{};
        BufferPoolS buffer_pool{};
        std::atomic_bool keep_running_daemon;

        std::optional<WorkerProcessSystem<UserProcessor, UserInnerspace>> user_system;
        std::optional<WorkerProcessSystem<SetupWorkerProcessor, SetupWorkerInnerspace>> setup_worker_system;
        std::optional<WorkerProcessSystem<DeleteWorkerProcessor, DeleteWorkerInnerspace>> delete_worker_system;

        static Config LoadConfig(SupportedCommand supported_commands, WorkerId worker_id, u16 setup_worker_port, u16 delete_worker_port, u16 user_port);
        void shutdown();
        void init(WorkerProcessTableS worker_process_table, const Config &config);
        void start();
        void run_daemon();
    };
    using WorkerProcessS = std::shared_ptr<WorkerProcess>;
}
