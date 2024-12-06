//
// Created by scott on 4/23/20.
//
#pragma once

#include "numeric_types.h"

#include <cstddef>
#include <cassert>
#include <cstdlib>

namespace estate {
    /* Owned data buffer of a fixed size.
     * Use this for long-lived data. Use Buffer for transient data.
     * */
    template<typename T>
    struct Blob {
        Blob() : data(nullptr), sz(0), moved(false) {}
        Blob(const u8 *data, size_t sz) :
                data(data), sz(sz) {
        }
        Blob(Blob &&other) noexcept: sz(other.sz), data(other.data) {
            other.data = nullptr;
            other.moved = true;
        }
        Blob(Blob &other) = delete;
        Blob(const Blob &other) = delete;
        size_t size() const {
            assert(!moved);
            return sz;
        }
        const T* get_flatbuffer() {
            return flatbuffers::GetRoot<T>(data);
        }
        bool empty() const {
            assert(!moved);
            return sz == 0;
        }
        const char *get_char() const {
            assert(!moved);
            return reinterpret_cast<const char *>(data);
        }
        const u8* get_raw() {
            assert(!moved);
            return data;
        }
        char *get_char() {
            assert(!moved);
            return (char *) data;
        }
        ~Blob() {
            if (!moved) {
                free((void *) data);
                data = nullptr;
            }
        }
    private:
        const size_t sz;
        const u8 *data;
        bool moved{false};
    };
}
