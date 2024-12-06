//
// Created by scott on 5/15/20.
//

#include "estate/internal/river/river.h"
#include "estate/internal/net_util.h"
#include "estate/internal/local_config.h"
#include <estate/runtime/version.h>
#include <iostream>
#include <cassert>
#include <estate/internal/file_util.h>

namespace estate {
    River::Config River::LoadConfig() {
        LocalConfiguration local_configuration {
                LocalConfiguration::FromFileInEnvironmentVariable("ESTATE_RIVER_CONFIG_FILE")};
        return Config{
                BufferPoolConfig::FromRemote(local_configuration.create_reader("BufferPool")),
                ThreadPoolConfig::FromRemote(local_configuration.create_reader("ThreadPool")),
                WorkerAuthenticationConfig::FromRemote(local_configuration.create_reader("WorkerAuthentication")),
                LoggingConfig::FromRemote(local_configuration.create_reader("Logging")),
                RiverProcessorConfig::FromRemote(local_configuration.create_reader("Processor")),
                outerspace::Config::FromRemote(local_configuration.create_reader("Outerspace")),
                innerspace::InnerspaceClientConfig::AsWorkerUser::FromRemote(local_configuration.create_reader("InnerspaceClient")),
                innerspace::InnerspaceWorkerLoaderClientConfig::FromRemote(local_configuration.create_reader("InnerspaceWorkerLoader"))
        };
    }
    void River::shutdown() {
        sys_log_info("Shutting down");

        thread_pool->shutdown();
        thread_pool = nullptr;

        sys_log_info("Estate River shut down cleanly");
    }
    void River::init(const Config &config) {
        assert(!has_init);
        init_logging(config.logging_config, false);

        buffer_pool = std::make_shared<BufferPool>(config.buffer_pool_config);

        thread_pool = std::make_shared<ThreadPool>(config.thread_pool_config);
        thread_pool->start();

        worker_authentication = std::make_shared<WorkerAuthentication>(config.worker_authentication_config);

        subscription_manager = std::make_shared<outerspace::SubscriptionManager>();

        innerspace_client = std::make_shared<innerspace::InnerspaceClient>(buffer_pool,
                                                                           thread_pool->get_context(),
                                                                           config.innerspace_worker_loader_client_config,
                                                                           config.innerspace_client_config);

        river_system.init(config.processor_config,
                          config.outerspace_config,
                          buffer_pool,
                          thread_pool,
                          worker_authentication,
                          buffer_pool,
                          subscription_manager,
                          innerspace_client);
        has_init = true;
    }
    void River::run(bool daemon) {
        assert(has_init);

        river_system.start();

        sys_log_info("Estate River {0} started", ESTATE_VERSION);

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
}
