//
// Created by scott on 4/23/20.
//
#pragma once

#include "deps/flatbuffers.h"
#include "numeric_types.h"

#include <cstddef>

namespace estate {
    /* Non-threadsafe unowned data buffer of a fixed size.
     * */
    template<typename T>
    struct BufferView {
        BufferView(const fbs::Vector<u8>& fbs_vec) : data(fbs_vec.Data()), sz(fbs_vec.size()) {}
        BufferView(const fbs::Builder& builder) : data(builder.GetBufferPointer()), sz(builder.GetSize()) {}
        BufferView(const u8 *data, size_t sz) : data(data), sz(sz) {}
        BufferView(BufferView& other) = default;
        BufferView(const BufferView &other) = default;
        ~BufferView() = default;
        bool empty() const {
            return sz == 0;
        }
        size_t size() const {
            return sz;
        }
        const u8 *as_u8() const{
            return data;
        }
        u8 *as_u8() {
            return const_cast<u8*>(data);
        }
        const char *as_char() const{
            return reinterpret_cast<const char *>(data);
        }
        char *as_char() {
            return reinterpret_cast<char *>(const_cast<u8*>(data));
        }
        const T* get_payload() const {
            return flatbuffers::GetRoot<T>(data);
        }
        constexpr const T* operator->() const {
            return get_payload();
        }
    private:
        const size_t sz;
        const u8 *data;
    };
}
