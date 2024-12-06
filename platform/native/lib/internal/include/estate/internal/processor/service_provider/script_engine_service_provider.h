//
// Created by scott on 5/18/20.
//

#pragma once

#include "estate/internal/buffer_pool.h"
#include "estate/internal/server/server_fwd.h"
#include "estate/internal/processor/service.h"

#include <memory>

namespace estate {
    template<typename TObjectRuntime>
    class ScriptEngineServiceProvider {
        Service<BufferPool> _buffer_pool;
        Service<storage::DatabaseManager> _database_manager;
        Service<TObjectRuntime> _object_runtime;
    public:
        using ObjectRuntimeS = std::shared_ptr<TObjectRuntime>;

        ScriptEngineServiceProvider(BufferPoolS buffer_pool, storage::DatabaseManagerS database_manager, ObjectRuntimeS object_runtime) :
                _buffer_pool(std::move(buffer_pool)),
                _database_manager(std::move(database_manager)),
                _object_runtime(std::move(object_runtime)) {
        }
        [[nodiscard]] BufferPoolS get_buffer_pool() {
            return _buffer_pool.get_service();
        }
        [[nodiscard]] storage::DatabaseManagerS get_database_manager() {
            return _database_manager.get_service();
        }
        [[nodiscard]] ObjectRuntimeS get_object_runtime() {
            return _object_runtime.get_service();
        }
    };
    template<typename TObjectRuntime>
    using ScriptEngineServiceProviderS = std::shared_ptr<ScriptEngineServiceProvider<TObjectRuntime>>;
}
