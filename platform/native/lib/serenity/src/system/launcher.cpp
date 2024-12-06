//
// Created by sjone on 1/14/2022.
//

#include "estate/internal/serenity/system/launcher.h"
#include "estate/internal/serenity/worker-process-table.h"

#include "estate/internal/serenity/system/worker-process.h"
#include "estate/internal/serenity/system/worker-loader.h"

#include <estate/runtime/enum_op.h>

#include <sys/wait.h>

namespace estate {
    Launcher::Config Launcher::load_config() {
        LocalConfiguration local_configuration {
                LocalConfiguration::FromFileInEnvironmentVariable("ESTATE_SERENITY_LAUNCHER_CONFIG_FILE")};
        return Config {
                LoggingConfig::FromRemote(local_configuration.create_reader("Logging")),
                WorkerProcessTableConfig::FromRemote(local_configuration.create_reader("WorkerProcessTable"))
        };
    }

    pid_t fork_worker_loader_process(WorkerProcessTableS worker_process_table) {
        sys_log_trace("Starting WorkerLoader process");
        const auto pid = fork();
        switch (pid) {
            case -1: {
                sys_log_critical("Failed to fork when trying to start WorkerLoader");
                return -1;
            }
            case 0: {
                //Worker-Loader process
                auto config = WorkerLoader::LoadConfig(WorkerLoader::SupportedCommand::GetWorkerProcessEndpoint);
                WorkerLoader worker_loader{};
                worker_loader.init(worker_process_table, config);
                worker_loader.run(true);
                return 0;
            }
            default: {
                return pid;
            }
        }
    }

    void Launcher::shutdown() {
        sys_log_info("Shutting down...");
        thread_pool->shutdown();
    }
    void Launcher::init(Config config) {
        init_logging(config.logging_config, false);

        thread_pool = std::make_shared<ThreadPool>(ThreadPoolConfig{});
        thread_pool->start();

        worker_process_table = std::make_shared<WorkerProcessTable>(config.table_config);
    }
    void Launcher::run() {
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

        // Start the WorkerLoader
        auto worker_loader_pid = fork_worker_loader_process(worker_process_table);
        switch(worker_loader_pid) {
            case -1: //fork failed, already logged
            case 0: //on WorkerLoader and it exited
                return;
            default: //everything else: started OK
                break;
        }

        sys_log_info("Estate Launcher {0} started", ESTATE_VERSION);

        while(keep_running_daemon) {
            switch(worker_process_table->launcher_update()) {
                case LauncherUpdateResult::IDLE:
                    sleep(1);
                    break;
                case LauncherUpdateResult::CONTINUE:
                    break;
                case LauncherUpdateResult::FAILURE:
                    return; //already logged
            }
        }
    }
}

