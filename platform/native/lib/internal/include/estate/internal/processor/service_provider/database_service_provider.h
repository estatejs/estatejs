//
// Created by scott on 5/18/20.
//

#pragma once

#include "estate/internal/buffer_pool.h"
#include "estate/internal/server/server_fwd.h"
#include "estate/internal/processor/service.h"

#include <memory>

namespace estate {
    class DatabaseServiceProvider {
        Service<BufferPool> _buffer_pool;
        Service<storage::DatabaseManager> _database_manager;
    public:
        DatabaseServiceProvider(BufferPoolS buffer_pool, storage::DatabaseManagerS database_manager);
        [[nodiscard]] BufferPoolS get_buffer_pool();
        [[nodiscard]] storage::DatabaseManagerS get_database_manager();
    };
    using DatabaseServiceProviderS = std::shared_ptr<DatabaseServiceProvider>;
}
