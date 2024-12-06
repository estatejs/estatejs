//
// Created by scott on 4/14/20.
//
#pragma once

#include "numeric_types.h"
#include <cstring>

#include <string>

namespace estate {
    using WorkerId = u64;
    using ClassId = u16;
    using MethodId = u16;
    using FileId = u16;
    using ObjectVersion = u64;
    using WorkerVersion = u64;
    using RequestId = u32;
    class PrimaryKey {
        bool _moved;
        const bool _owned;
        const size_t _size;
        const u8 *_bytes;
        std::string_view _view;
    public:
        ~PrimaryKey();
        //The size of the PrimaryKey in bytes
        [[nodiscard]] size_t get_size() const;
        [[nodiscard]] const u8 *get_bytes() const;
        [[nodiscard]] const std::string_view view() const;
        explicit PrimaryKey(const std::string &source);
        explicit PrimaryKey(const std::string_view &source);
        explicit PrimaryKey(const u8 *bytes, size_t size);
        PrimaryKey(const PrimaryKey &other) = delete; //no copy
        PrimaryKey(PrimaryKey &&other) noexcept;
        bool operator==(const PrimaryKey& other) const;
    };
}
