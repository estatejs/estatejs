//
// Created by scott on 5/18/20.
//

#pragma once

#include <estate/internal/deps/fmt.h>

#include <memory>
#include <mutex>
#include <type_traits>

namespace estate {

    template<class T, typename TS = std::shared_ptr<T>>
    class Service {
        std::mutex _mutex{};
        TS _service;
    public:
        explicit Service(TS service) : _service(std::move(service)) {
            assert(_service);
        }

        ~Service() = default;
        Service(const Service &other) = delete;
        Service(Service &&other) = delete;

        TS get_service() {
            std::lock_guard<std::mutex> lck(_mutex);
            return _service;
        }

        /* Atomically gets the service if it passes a check, if it does not, its replaced and the replacement is returned. */
        TS get_or_replace(std::function<bool(const T&)> replace_if, std::function<TS()> factory) {
            std::lock_guard<std::mutex> lck(_mutex);
            if(replace_if(*_service)) {
                _service = factory();
                assert(_service);
            }
            return _service;
        }
    };
}

#undef lock