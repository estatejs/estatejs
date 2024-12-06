//
// Created by scott on 5/18/20.
//

#pragma once

#include "estate/internal/buffer_pool.h"
#include "estate/internal/processor/service.h"

#include <memory>

namespace estate {
    class BufferPoolServiceProvider {
        Service<BufferPool> _buffer_pool;
    public:
        BufferPoolServiceProvider(BufferPoolS buffer_pool);
        [[nodiscard]] BufferPoolS get_buffer_pool();
    };
    using BufferPoolServiceProviderS = std::shared_ptr<BufferPoolServiceProvider>;
}