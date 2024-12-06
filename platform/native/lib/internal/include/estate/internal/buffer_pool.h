//
// Created by scott on 5/2/20.
//

#pragma once

#include <estate/internal/deps/boost.h>
#include <estate/runtime/deps/flatbuffers.h>
#include <estate/runtime/numeric_types.h>
#include <estate/runtime/buffer_view.h>
#include <cassert>

#include <memory>
#include <vector>
#include <mutex>
#include <queue>
#include <atomic>
#include <functional>
#include "local_config.h"

namespace estate {
    using InternalBuffer = std::string;
    using InternalBufferU = std::unique_ptr<std::string>;

    template<typename T>
    class Buffer;

    template<typename THeader, typename TPayload, typename TPayloadSize>
    class Envelope;

    class BufferPool;
    using BufferPoolS = std::shared_ptr<BufferPool>;

    struct BufferPoolConfig {
        const bool clear_on_get{true};
        static BufferPoolConfig FromRemote(const LocalConfigurationReader &getter);
    };

    class BufferReference {
        BufferPoolS _origin;
        InternalBufferU _buffer;
    public:
        BufferReference(InternalBufferU buffer, BufferPoolS origin);
        ~BufferReference();
        InternalBuffer &get() {
            return *_buffer;
        }
        BufferPoolS origin() {
            return _origin;
        }
    };
    using BufferReferenceS = std::shared_ptr<BufferReference>;

    class BufferPool : public std::enable_shared_from_this<BufferPool> {
        const BufferPoolConfig config;
        std::atomic_int outstanding_leases{0};
        std::mutex buffers_mutex{};
        std::queue<InternalBufferU> buffers{};

        friend class BufferReference;

        void release(InternalBufferU buffer) {
            std::unique_lock<std::mutex> lck(buffers_mutex);
            outstanding_leases.fetch_sub(1);
            buffers.push(std::move(buffer));
        }

    public:
        BufferPool(BufferPoolConfig config);

        int outstanding_lease_count() const;

        [[nodiscard]] size_t buffer_queue_count();

        template<typename THeader, typename TPayload, typename TPayloadSize>
        Envelope<THeader, TPayload, TPayloadSize> get_envelope() {
            using Envelope_ = Envelope<THeader, TPayload, TPayloadSize>;
            auto buffer = get_buffer<typename Envelope_::_Envelope>();
            return Envelope_{std::move(buffer)};
        }

        template<typename T>
        Buffer<T> get_buffer() {
            bool was_reused{false};
            InternalBufferU internal_buffer = get_internal_buffer(was_reused);
            internal_buffer->clear();
            BufferReferenceS ref = std::make_shared<BufferReference>(std::move(internal_buffer), this->shared_from_this());
            Buffer<T> buffer{std::move(ref), was_reused};
            outstanding_leases.fetch_add(1);
            return std::move(buffer);
        }
    private:
        InternalBufferU get_internal_buffer(bool &was_reused);
    };

    template<typename T>
    class Buffer {
        BufferReferenceS _ref;
        bool _moved{false};
        const bool _reused;
    public:
        Buffer(BufferReferenceS ref, bool reused) :
                _ref{std::move(ref)}, _reused{reused}, _moved{false} {
            assert(_ref);
        }
        Buffer(const Buffer &other) : // copy OK (doesn't actually copy the internal buffer, just increments the reference)
                _ref{other._ref}, _reused{other._reused}, _moved{false} {
        }
        Buffer(Buffer &&move_from) noexcept: // move OK too
                _ref{std::move(move_from._ref)}, _reused{move_from._reused}, _moved{false} {
            move_from._moved = true;
        }
        Buffer<T> copy() const {
            assert(!_moved);
            const auto sz = _ref->get().size();
            assert(sz > 0);
            Buffer<T> buffer = _ref->origin()->template get_buffer<T>();
            buffer.resize(sz);
            std::memcpy(buffer.get(), get(), sz);
            return std::move(buffer);
        }
        BufferView<T> get_view() const {
            return BufferView<T>{
                    as_u8(),
                    _ref->get().size()
            };
        }
        void resize(size_t new_size) {
            assert(!_moved);
            if (_ref->get().size() != new_size) {
                _ref->get().resize(new_size);
                assert(_ref->get().size() == new_size);
            }
        }
        void clear() noexcept {
            assert(!_moved);
            _ref->get().clear();
        }
        const T *get_flatbuffer() const {
            assert(!_moved);
            assert(size() > 0);
            return flatbuffers::GetRoot<T>(_ref->get().data());
        }
        const T *get() const {
            assert(!_moved);
            assert(size() > 0);
            return reinterpret_cast<const T *>(_ref->get().data());
        }
        T *get() {
            assert(!_moved);
            assert(size() > 0);
            return reinterpret_cast<T *>(_ref->get().data());
        }
        const u8 *as_u8() const {
            assert(!_moved);
            assert(size() > 0);
            return reinterpret_cast<const u8 *>(_ref->get().data());
        }
        char *as_char() {
            assert(!_moved);
            assert(size() > 0);
            return reinterpret_cast<char *>(_ref->get().data());
        }
        const char *as_char() const {
            assert(!_moved);
            assert(size() > 0);
            return reinterpret_cast<const char *>(_ref->get().data());
        }
        u8 *as_u8() {
            assert(!_moved);
            assert(size() > 0);
            return reinterpret_cast<u8 *>(_ref->get().data());
        }
        size_t size() const noexcept {
            assert(!_moved);
            return _ref->get().size();
        }
        bool empty() const noexcept {
            assert(!_moved);
            return _ref->get().size() == 0;
        }
        const bool has_moved() const {
            return _moved;
        }
        const bool is_reused() const {
            return _reused;
        }
        void with_internal_buffer(std::function<void(InternalBuffer &)> action) {
            assert(!_moved);
            action(_ref->get());
        }
        template<typename R>
        R with_internal_buffer(std::function<R(InternalBuffer &)> action) {
            assert(!_moved);
            R ret = action(_ref->get());
            return std::forward<R>(ret);
        }
        constexpr const T *operator->() const {
            return get_flatbuffer();
        }
    };

    template<typename THeader, typename TPayload, typename TPayloadSize>
    class Envelope {
        struct _Envelope {
            THeader header;
            TPayload *payload;
        };
        Buffer<_Envelope> _buffer;
        friend class BufferPool;
    public:
        Buffer<_Envelope> &get_buffer() {
            return _buffer;
        }
        BufferView<TPayload> get_payload_view() {
            return BufferView<TPayload>{
                    get_payload_raw(),
                    get_payload_size()
            };
        }
        Envelope(Buffer<_Envelope> &&buffer) :
                _buffer(std::move(buffer)) {}
        Envelope(const Envelope &other) :
                _buffer{other._buffer} {}
        Envelope(Envelope &&other) :
                _buffer(std::move(other._buffer)) {}
        void resize_for_header() {
            _buffer.resize(sizeof(THeader));
            assert(_buffer.size() == sizeof(THeader));
        }
        THeader *get_header() {
            assert(_buffer.size() >= sizeof(THeader));
            return reinterpret_cast<THeader *>(_buffer.as_u8());
        }
        const THeader *get_header() const {
            assert(_buffer.size() >= sizeof(THeader));
            return reinterpret_cast<const THeader *>(_buffer.as_u8());
        }
        u8 *get_header_raw() {
            assert(_buffer.size() >= sizeof(THeader));
            return reinterpret_cast<u8 *>(_buffer.as_u8());
        }
        const u8 *get_header_raw() const {
            assert(_buffer.size() >= sizeof(THeader));
            return reinterpret_cast<const u8 *>(_buffer.as_u8());
        }
        u8 *get_payload_raw() {
            assert(_buffer.size() > sizeof(THeader));
            return reinterpret_cast<u8 *>(_buffer.as_u8() + sizeof(THeader));
        }
        const TPayload *get_payload() {
            assert(_buffer.size() > sizeof(THeader));
            return flatbuffers::GetRoot<TPayload>(_buffer.as_u8() + sizeof(THeader));
        }
        constexpr size_t get_header_size() const {
            return sizeof(THeader);
        }
        TPayloadSize get_payload_size() const {
            assert(_buffer.size() > sizeof(THeader));
            return _buffer.size() - sizeof(THeader);
        }
        [[nodiscard]] size_t get_total_size() const {
            return _buffer.size();
        }
        void resize_for_payload(TPayloadSize size) {
            assert(size > 0);
            _buffer.resize(sizeof(THeader) + size);
            assert(_buffer.size() == sizeof(THeader) + size);
        }
    };
}
