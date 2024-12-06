//
// Created by scott on 5/2/20.
//

#pragma once

#include <estate/runtime/result.h>

#include <memory>
#include <mutex>
#include <queue>
#include <atomic>
#include <functional>

namespace estate {
    template<typename TResource, typename C>
    class Pool;

    template<typename TResource, typename C>
    using PoolS = std::shared_ptr<Pool<TResource, C>>;

    template<typename TResource, typename C>
    class Pool : public std::enable_shared_from_this<Pool<TResource, C>> {
    public:
        using Resource = TResource;
        using ResourceU = std::unique_ptr<TResource>;
        using ResourceFactory = std::function<ResultCode<ResourceU, C>()>;
        using ResourceValidator = std::function<bool(ResourceU &)>;

        class Handle {
            PoolS<TResource, C> pool;
        public:
            Handle(ResourceU &&resource, PoolS<TResource, C> pool, bool reused) : _resource(std::move(resource)), pool(pool), _reused(reused) {}
            Handle(const Handle &other) = delete;
            Handle(Handle &&other) : _resource(std::move(other._resource)), pool(other.pool), _reused(other._reused) {
                other._moved = true;
                other.pool = nullptr;
            }
            ~Handle() {
                if (!_moved) {
                    pool->release(std::move(_resource));
                }
            }
        public:
            ResourceU &get() {
                return _resource;
            }
        private:
            friend class Pool<TResource, C>;
            ResourceU _resource;
            bool _moved{false};
            const bool _reused;
        };
    private:
        std::atomic_int _outstanding_leases{0};
        std::mutex _resources_mutex{};
        std::queue<ResourceU> _resources{};

        friend class ResourceHandle;
        void release(ResourceU &&buffer) {
            std::unique_lock<std::mutex> lck(_resources_mutex);
            _outstanding_leases.fetch_sub(1);
            _resources.push(std::move(buffer));
        }
    public:
        Pool() = default;

        int outstanding_lease_count() const {
            return _outstanding_leases;
        }

        [[nodiscard]] size_t resource_queue_count() {
            std::unique_lock<std::mutex> lck(_resources_mutex);
            return _resources.size();
        }
        ResultCode <Handle, C> get_resource(ResourceFactory factory, ResourceValidator validate) {
            using Result = ResultCode<Handle, C>;

            bool was_reused{false};

            auto resource_r = fetch_resource(factory, validate, was_reused);
            if (!resource_r) {
                return Result::Error(resource_r.get_error());
            }
            ResourceU resource = resource_r.unwrap();

            Handle handle{std::move(resource), this->shared_from_this(), was_reused};
            _outstanding_leases.fetch_add(1);
            return Result::Ok(std::move(handle));
        }
    private:
        Result <ResourceU> try_fetch() {
            using Result = Result<ResourceU>;
            std::unique_lock<std::mutex> lck(_resources_mutex);
            if (!_resources.empty()) {
                ResourceU internal_buffer = std::move(_resources.front());
                _resources.pop();
                return Result::Ok(std::move(internal_buffer));
            }
            return Result::Error();
        }
        ResultCode <ResourceU, C> fetch_resource(ResourceFactory &factory, ResourceValidator &validate, bool &was_reused) {
            using Result = ResultCode<ResourceU, C>;
            auto try_resource_r = try_fetch();
            if (try_resource_r) {
                ResourceU resource = try_resource_r.unwrap();
                if (!validate(resource)) {
                    auto resource_r = factory();
                    if (!resource_r)
                        return Result::Error(resource_r.get_error());
                    was_reused = false;
                    return Result::Ok(std::move(resource_r.unwrap()));
                } else {
                    was_reused = true;
                    return Result::Ok(std::move(resource));
                }
            } else {
                auto resource_r = factory();
                if (!resource_r)
                    return Result::Error(resource_r.get_error());
                was_reused = false;
                return Result::Ok(std::move(resource_r.unwrap()));
            }
        }
    };
}
