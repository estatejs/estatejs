#include <estate/internal/thread_pool.h>

#include <memory>
#include <gtest/gtest.h>
#include <condition_variable>

using namespace estate;

TEST(unit_thread_pool_tests, CanPostToThreadPool) {
    //arrange
    auto thread_pool = std::make_unique<ThreadPool>(ThreadPoolConfig{});
    thread_pool->start();
    ASSERT_TRUE(thread_pool->is_started());

    auto strand = thread_pool->create_strand();
    int step = 0;
    std::condition_variable cv;
    bool ready = false;
    std::mutex mutex;

    //act
    strand.post([&step](){
        if(step == 0)
            step = 1;
    });
    strand.post([&step](){
        if(step == 1)
            step = 2;
    });
    strand.post([&step, &ready, &cv](){
        if(step == 2) {
            step = 3;
            ready = true;
            cv.notify_one();
        }
    });

    std::unique_lock<std::mutex> lck(mutex);
    while(!ready)
        cv.wait(lck);

    //assert
    thread_pool->shutdown();
    ASSERT_TRUE(step == 3);
}
