//
// Originally written by Scott R. Jones.
// Copyright (c) 2020 Warpdrive Technologies, Inc. All rights reserved.
//

#include "estate/runtime/model_types.h"

namespace estate {
    PrimaryKey::~PrimaryKey() {
        if (!_moved && _owned) {
            free((void *) _bytes);
        }
    }
    size_t PrimaryKey::get_size() const {
        return _size;
    }
    const u8 *PrimaryKey::get_bytes() const {
        return _bytes;
    }
    const std::string_view PrimaryKey::view() const {
        return _view;
    }
    PrimaryKey::PrimaryKey(const std::string &source) :
            _size(source.size() * sizeof(char)),
            _owned(true),
            _moved(false) {
        _bytes = (u8 *) malloc(_size);
        std::memcpy((void *) _bytes, source.data(), _size);
        _view = std::string_view{reinterpret_cast<const char *>(_bytes), _size};
    }
    PrimaryKey::PrimaryKey(const std::string_view &source) :
            _size(source.size() * sizeof(char)),
            _owned(false),
            _moved(false),
            _bytes{reinterpret_cast<const u8 *>(source.data())},
            _view{reinterpret_cast<const char *>(_bytes), _size} {
    }
    PrimaryKey::PrimaryKey(const u8 *bytes, size_t size) :
            _size(size),
            _bytes(bytes),
            _owned(false),
            _moved(false),
            _view{reinterpret_cast<const char *>(bytes), _size} {
    }
    PrimaryKey::PrimaryKey(PrimaryKey &&other) noexcept: _size(other._size), _bytes(other._bytes), _owned(other._owned), _view(other._view), _moved(false) {
        other._moved = true;
    }
    bool PrimaryKey::operator==(const PrimaryKey &other) const {
        return other._view == this->_view;
    }
}
