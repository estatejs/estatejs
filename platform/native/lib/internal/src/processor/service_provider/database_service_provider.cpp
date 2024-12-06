//
// Created by scott on 4/5/20.
//

#include "estate/internal/processor/service_provider/database_service_provider.h"

namespace estate {
    storage::DatabaseManagerS DatabaseServiceProvider::get_database_manager() {
        return _database_manager.get_service();
    }
    DatabaseServiceProvider::DatabaseServiceProvider(BufferPoolS buffer_pool, storage::DatabaseManagerS database_manager) :
            _buffer_pool(std::move(buffer_pool)), _database_manager(std::move(database_manager)) {
    }
    BufferPoolS DatabaseServiceProvider::get_buffer_pool() {
        return _buffer_pool.get_service();
    }
}
