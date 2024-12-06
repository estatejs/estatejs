//
// Created by scott on 4/28/20.
//

#pragma once

#include <condition_variable>
#include <mutex>
#include <atomic>
#include "numeric_types.h"

namespace estate {
    struct Event {
        void wait_one();
        void notify_one();
    private:
        std::condition_variable cv{};
        std::mutex mutex{};
        std::atomic<u64> count{0};
    };
}