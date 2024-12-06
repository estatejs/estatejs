//
// Created by scott on 4/11/20.
//

#include "estate/internal/serenity/system/worker-process.h"

#include <estate/internal/net_util.h>
#include <estate/runtime/enum_op.h>
#include <estate/runtime/version.h>
#include <iostream>
#include <cassert>

namespace estate {
    WorkerProcess::Config WorkerProcess::LoadConfig(SupportedCommand supported_commands, WorkerId worker_id, u16 setup_worker_port, u16 delete_worker_port, u16 user_port) {
        LocalConfiguration local_configuration {
                LocalConfiguration::FromFileInEnvironmentVariable("ESTATE_SERENITY_WORKER_PROCESS_CONFIG_FILE")};
        return Config{
                BufferPoolConfig::FromRemote(local_configuration.create_reader("BufferPool")),
                ThreadPoolConfig::FromRemote(local_configuration.create_reader("ThreadPool")),
                engine::javascript::JavascriptConfig::FromRemote(local_configuration.create_reader("Javascript")),
                storage::DatabaseManagerConfiguration::FromRemote(local_configuration.create_reader("DatabaseManager")),
                LoggingConfig::FromRemoteWithWorkerId(local_configuration.create_reader("Logging"), worker_id),
                supported_commands,
                UserProcessorConfig::Create(worker_id),
                UserInnerspace::Server::Config::FromRemoteWithoutPort(local_configuration.create_reader("UserInnerspaceServer"), user_port),
                SetupWorkerProcessorConfig::Create(worker_id),
                SetupWorkerInnerspace::Server::Config::FromRemoteWithoutPort(local_configuration.create_reader("SetupWorkerInnerspaceServer"), setup_worker_port),
                DeleteWorkerProcessorConfig::FromRemoteWithWorkerId(local_configuration.create_reader("DeleteWorkerProcessor"), worker_id),
                DeleteWorkerInnerspace::Server::Config::FromRemoteWithoutPort(local_configuration.create_reader("DeleteWorkerInnerspaceServer"), delete_worker_port)
        };
    }
    void WorkerProcess::shutdown() {
        sys_log_info("Shutting down");

        sys_log_trace("Shutting down thread pool");
        thread_pool->shutdown();
        thread_pool = nullptr;
        sys_log_trace("Thread pool shut down");

        if (user_system) {
            sys_log_trace("Shutting down CallMethod system");
            user_system.value().shutdown();
            user_system.reset();
            sys_log_trace("CallMethod system shut down");
        }

        if (delete_worker_system) {
            sys_log_trace("Shutting down DeleteWorker system");
            delete_worker_system.value().shutdown();
            delete_worker_system.reset();
            sys_log_trace("DeleteWorker system shut down");
        }

        if (setup_worker_system) {
            sys_log_trace("Shutting down SetupWorker system");
            setup_worker_system.value().shutdown();
            setup_worker_system.reset();
            sys_log_trace("SetupWorker system shut down");
        }

        sys_log_trace("Shutting down database");
        database_manager = nullptr;
        sys_log_trace("Database shut down");

        if (engine::javascript::is_initialized()) {
            sys_log_trace("Shutting down JavaScript platform");
            engine::javascript::shutdown();
            sys_log_trace("JavaScript platform shutdown");
        }

        sys_log_info("Estate WorkerProcess shut down cleanly");
    }
    void WorkerProcess::init(WorkerProcessTableS worker_process_table, const Config &config) {
        assert(!has_init);

        init_logging(config.logging_config, true);

        if (config.supported_commands == SupportedCommand::None)
            throw std::domain_error("can't init without specifying any commands");

        buffer_pool = std::make_shared<BufferPool>(config.buffer_pool_config);

        database_manager = std::make_shared<storage::DatabaseManager>(config.db_config, buffer_pool);

        if (config.has_command(SupportedCommand::User) || config.has_command(SupportedCommand::SetupWorker)) {
            if (!engine::javascript::is_initialized())
                engine::javascript::initialize(config.javascript_config);
        }

        thread_pool = std::make_shared<ThreadPool>(config.thread_pool_config);
        thread_pool->start();

        engine::IObjectRuntimeS js_object_runtime{};

        if (config.has_command(SupportedCommand::User)) {
            js_object_runtime = engine::javascript::create_object_runtime(config.javascript_config.max_heap_size);
            user_system.emplace();
            user_system.value().init(config.user_processor_config,
                                     config.user_server_config,
                                     buffer_pool,
                                     thread_pool,
                                     buffer_pool,
                                     database_manager,
                                     js_object_runtime);
        }

        if (config.has_command(SupportedCommand::DeleteWorker)) {
            delete_worker_system.emplace();
            WorkerProcessS self = this->shared_from_this();
            auto shutdown_requestor = std::make_shared<ShutdownRequestor>([self](){
                sys_log_trace("Requesting WorkerProcess shutdown");
                self->keep_running_daemon = false;
                self->shutdown();
            });
            delete_worker_system.value().init(config.delete_worker_processor_config,
                                            config.delete_worker_server_config,
                                            buffer_pool,
                                            thread_pool,
                                            buffer_pool,
                                            database_manager,
                                            shutdown_requestor,
                                            worker_process_table);
        }

        if (config.has_command(SupportedCommand::SetupWorker)) {
            auto js_setup_runtime = engine::javascript::create_setup_runtime();
            setup_worker_system.emplace();
            setup_worker_system.value().init(config.setup_worker_processor_config,
                                           config.setup_worker_server_config,
                                           buffer_pool,
                                           thread_pool,
                                           buffer_pool,
                                           database_manager,
                                           js_setup_runtime);
        }

        has_init = true;
    }
    void WorkerProcess::run_daemon() {
        keep_running_daemon = true;

        boost::asio::signal_set shutdown_signals{*thread_pool->get_context()};

        shutdown_signals.add(SIGINT);
        shutdown_signals.add(SIGTERM);
        shutdown_signals.add(SIGQUIT);

        shutdown_signals.async_wait([self{this}](boost::system::error_code error_code, int signal_number) {
            switch (signal_number) {
                case SIGINT:
                    sys_log_warn("SIGINT caught, shutting down");
                    break;
                case SIGTERM:
                    sys_log_warn("SIGTERM caught, shutting down");
                    break;
                case SIGQUIT:
                    sys_log_warn("SIGQUIT caught, shutting down");
                    break;
                default:
                    sys_log_error("Unknown signal caught, ignoring");
                    return;
            }
            self->keep_running_daemon = false;
            self->shutdown();
        });

        std::cout << "Press Ctrl+C to shutdown" << std::endl;

        while (keep_running_daemon) {
            sleep(1);
        }
    }
    void WorkerProcess::start() {
        assert(has_init);

        if (user_system) {
            user_system.value().start();
        }

        if (delete_worker_system) {
            delete_worker_system.value().start();
        }

        if (setup_worker_system) {
            setup_worker_system.value().start();
        }

        sys_log_info("Estate WorkerProcess {0} started", ESTATE_VERSION);
    }
    bool WorkerProcess::Config::has_command(WorkerProcess::SupportedCommand command) const {
        return (supported_commands & command) == command;
    }
}
