//
// Created by scott on 4/5/20.
//

#include "estate/internal/processor/service_provider/buffer_pool_service_provider.h"

namespace estate {
    BufferPoolServiceProvider::BufferPoolServiceProvider(BufferPoolS buffer_pool) :
            _buffer_pool(std::move(buffer_pool)) {
    }
    BufferPoolS BufferPoolServiceProvider::get_buffer_pool() {
        return _buffer_pool.get_service();
    }
}