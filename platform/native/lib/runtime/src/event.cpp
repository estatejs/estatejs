//
// Created by scott on 4/28/20.
//

#include "estate/runtime/event.h"

namespace estate {
    void Event::wait_one() {
        std::unique_lock<std::mutex> lck(mutex);
        while(count.load() == 0)
            cv.wait(lck);
        count.fetch_sub(1);
    }
    void Event::notify_one() {
        std::unique_lock<std::mutex> lck(mutex);
        count.fetch_add(1);
        cv.notify_one();
    }
}