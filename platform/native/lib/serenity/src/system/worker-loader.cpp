//
// Created by scott on 4/11/20.
//

#include "estate/internal/serenity/system/worker-loader.h"
#include "estate/internal/net_util.h"

#include <estate/runtime/enum_op.h>
#include <estate/runtime/version.h>
#include <iostream>
#include <cassert>

namespace estate {
    WorkerLoader::Config WorkerLoader::LoadConfig(SupportedCommand supported_commands) {
        LocalConfiguration local_configuration {
                LocalConfiguration::FromFileInEnvironmentVariable("ESTATE_SERENITY_WORKER_LOADER_CONFIG_FILE")};
        return Config{
                BufferPoolConfig::FromRemote(local_configuration.create_reader("BufferPool")),
                ThreadPoolConfig::FromRemote(local_configuration.create_reader("ThreadPool")),
                LoggingConfig::FromRemote(local_configuration.create_reader("Logging")),
                supported_commands,
                GetWorkerProcessEndpointProcessorConfig::FromRemote(local_configuration.create_reader("GetWorkerProcessEndpointProcessor")),
                GetWorkerProcessEndpointInnerspace::Server::Config::FromRemote(local_configuration.create_reader("GetWorkerProcessEndpointInnerspaceServer"))
        };
    }
    void WorkerLoader::shutdown() {
        sys_log_trace("Shutting down");

        sys_log_trace("Shutting down thread pool");
        thread_pool->shutdown();
        thread_pool = nullptr;
        sys_log_trace("Thread pool shut down");

        if(get_worker_process_endpoint_system) {
            sys_log_trace("Shutting down GetWorkerProcessEndpoint system");
            get_worker_process_endpoint_system.value().shutdown();
            get_worker_process_endpoint_system.reset();
            sys_log_trace("GetWorkerProcessEndpoint system shut down");
        }

        sys_log_info("Estate WorkerLoader shut down cleanly");
    }
    void WorkerLoader::init(WorkerProcessTableS worker_process_table, const Config &config) {
        assert(!has_init);
        init_logging(config.logging_config, true);

        if (config.supported_commands == SupportedCommand::None)
            throw std::domain_error("can't init without specifying any commands");

        buffer_pool = std::make_shared<BufferPool>(config.buffer_pool_config);

        thread_pool = std::make_shared<ThreadPool>(config.thread_pool_config);
        thread_pool->start();

        if (config.has_command(SupportedCommand::GetWorkerProcessEndpoint)) {
            get_worker_process_endpoint_system.emplace();
            get_worker_process_endpoint_system.value().init(config.get_worker_process_endpoint_processor_config,
                                     config.get_worker_process_endpoint_server_config,
                                                        buffer_pool,
                                                        thread_pool,
                                                        buffer_pool,
                                                        worker_process_table);
        }

        has_init = true;
    }
    void WorkerLoader::run(bool daemon) {
        assert(has_init);

        if (get_worker_process_endpoint_system) {
            get_worker_process_endpoint_system.value().start();
        }

        sys_log_info("Estate WorkerLoader {0} started", ESTATE_VERSION);

        if (daemon) {
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
    }
    bool WorkerLoader::Config::has_command(WorkerLoader::SupportedCommand command) const {
        return (supported_commands & command) == command;
    }
}
