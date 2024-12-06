//
// Created by sjone on 1/14/2022.
//

#pragma once

#include <estate/runtime/model_types.h>
#include <estate/internal/local_config.h>
#include <estate/internal/logging.h>
#include "estate/internal/serenity/worker-process-table.h"
#include <estate/internal/thread_pool.h>

namespace estate {
    struct Launcher {
        struct Config {
            LoggingConfig logging_config{};
            WorkerProcessTableConfig table_config{};
        };
        static Config load_config();
        void init(Config config);
        void run();
        void shutdown();
        ThreadPoolS thread_pool;
        WorkerProcessTableS worker_process_table;
        std::atomic<bool> keep_running_daemon;
    };
}
