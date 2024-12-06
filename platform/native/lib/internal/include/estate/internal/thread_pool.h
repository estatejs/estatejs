//
// Created by scott on 2/10/20.
//
#pragma once

#include "stopwatch.h"

#include "estate/internal/local_config.h"
#include "estate/internal/deps/boost.h"

#include <estate/runtime/numeric_types.h>

#include <memory>
#include <thread>

namespace estate {
    void thread_pool_worker_thread(std::shared_ptr<boost::asio::io_context> context);

    struct ThreadPoolConfig {
        size_t num_threads = boost::thread::hardware_concurrency();
        static ThreadPoolConfig Half() {
            return ThreadPoolConfig{
                    boost::thread::hardware_concurrency() / 2
            };
        }
        static ThreadPoolConfig FromRemote(const LocalConfigurationReader &getter) {
            static_assert(sizeof(u64) == sizeof(size_t));
            return ThreadPoolConfig {
                getter.get_u64("num_threads", boost::thread::hardware_concurrency())
            };
        }
    };

    class ThreadPool {
        const ThreadPoolConfig config;
        std::unique_ptr<boost::asio::io_context::work> keep_running_work{};
        boost::thread_group thread_group;
        std::shared_ptr<boost::asio::io_context> context;
    public:
        explicit ThreadPool(const ThreadPoolConfig config);
        [[nodiscard]] bool is_started() const;
        [[nodiscard]] std::shared_ptr<boost::asio::io_context> get_context();
        void start();
        void shutdown();
        template<class F>
        void post(F f) {
            assert(is_started());
            context->post(f);
        }
        [[nodiscard]] boost::asio::io_context::strand create_strand() {
            boost::asio::io_context::strand s(*context);
            return s;
        }
    };

    using ThreadPoolS = std::shared_ptr<ThreadPool>;
}
