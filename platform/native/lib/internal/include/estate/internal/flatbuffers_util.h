//
// Created by scott on 2/12/20.
//
#pragma once

#include "buffer_pool.h"
#include "logging.h"
#include <estate/runtime/deps/flatbuffers.h>

namespace estate {
    template<typename T>
    Buffer<T> finish_and_copy_to_buffer(fbs::Builder& builder, BufferPoolS buffer_pool, fbs::Offset<T> target) {
        //TODO: remove the extra copy
        builder.Finish(target);
        auto ptr = builder.GetBufferPointer();
        auto sz = builder.GetSize();
        Buffer<T> buffer = buffer_pool->get_buffer<T>();
        buffer.resize(sz);
        std::memmove(buffer.as_u8(), ptr, sz);
        return std::move(buffer);
    }
}
