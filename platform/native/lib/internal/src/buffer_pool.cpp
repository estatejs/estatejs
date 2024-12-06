//
// Originally written by Scott R. Jones.
// Copyright (c) 2020 Warpdrive Technologies, Inc. All rights reserved.
//

#include <estate/internal/buffer_pool.h>

namespace estate {
    BufferPool::BufferPool(BufferPoolConfig config) : config(std::move(config)) {
    }
    int BufferPool::outstanding_lease_count() const {
        return outstanding_leases;
    }
    size_t BufferPool::buffer_queue_count() {
        std::unique_lock<std::mutex> lck(buffers_mutex);
        return buffers.size();
    }
    InternalBufferU BufferPool::get_internal_buffer(bool &was_reused) {
        std::unique_lock<std::mutex> lck(buffers_mutex);
        if (buffers.empty()) {
            auto internal_buffer = std::make_unique<InternalBuffer>();
            was_reused = false;
            return std::move(internal_buffer);
        } else {
            InternalBufferU internal_buffer = std::move(buffers.front());
            buffers.pop();
            was_reused = true;
            return std::move(internal_buffer);
        }
    }
    BufferPoolConfig BufferPoolConfig::FromRemote(const LocalConfigurationReader &getter) {
        return BufferPoolConfig{
                getter.get_bool("clear_on_get", true)
        };
    }
    BufferReference::BufferReference(InternalBufferU buffer, BufferPoolS origin) :
            _origin{origin}, _buffer{std::move(buffer)} {
    }
    BufferReference::~BufferReference() {
        if (_buffer) {
            _origin->release(std::move(_buffer));
            _buffer = nullptr;
        }
    }
}
