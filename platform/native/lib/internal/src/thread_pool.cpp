//
// Created by scott on 2/10/20.
//

#include "estate/internal/thread_pool.h"

namespace estate {

    void ThreadPool::start() {
        assert(!keep_running_work);

        auto context_ = std::make_shared<boost::asio::io_context>();
        auto keep_running_work_ = std::make_unique<boost::asio::io_context::work>(*context_);

        for (int i = 0; i < config.num_threads; ++i) {
            thread_group.create_thread([context_] { return thread_pool_worker_thread(context_); });
        }

        context = context_;
        keep_running_work = std::move(keep_running_work_);
    }

    void ThreadPool::shutdown() {
        keep_running_work.reset(); //clear the fake work keeping the context from exiting
        context->stop();
    }

    bool ThreadPool::is_started() const {
        return keep_running_work != nullptr;
    }

    ThreadPool::ThreadPool(ThreadPoolConfig config) :
        config(config), context(nullptr), thread_group(), keep_running_work(nullptr) {
        assert(config.num_threads > 0);
    }

    std::shared_ptr<boost::asio::io_context> ThreadPool::get_context() {
        return context;
    }

    void thread_pool_worker_thread(std::shared_ptr<boost::asio::io_context> context) { // NOLINT(performance-unnecessary-value-param)
        context->run();
    }
}