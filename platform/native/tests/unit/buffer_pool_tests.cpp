#include "../estate_test.h"
#include "gtest/gtest.h"

#include <estate/internal/buffer_pool.h>
#include <estate/internal/innerspace/innerspace.h>
#include <estate/internal/flatbuffers_util.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

using namespace estate;

TEST(unit_buffer_pool_tests, CanUseMessageBuffer) {
    using Innerspace = Innerspace<DeleteWorkerRequestProto, DeleteWorkerResponseProto>;

    auto log_context = make_test_log_context;

    BufferPoolConfig buffer_pool_config{
            true
    };
    auto buffer_pool = std::make_shared<BufferPool>(buffer_pool_config);
    const WorkerId worker_id = 100;
    const WorkerVersion worker_version = 22;
    const ClassId class_id = 2;
    const std::string primary_key_str{std::string("default")};
    int blob_sz = 0;
    u8 *raw_ptr = 0;

    {
        ASSERT_EQ(buffer_pool->outstanding_lease_count(), 0);
        ASSERT_EQ(buffer_pool->buffer_queue_count(), 0);
        {
            auto message = buffer_pool->get_envelope<Innerspace::RequestHeader, DeleteWorkerRequestProto, u32>();
            message.resize_for_header();

            auto &buffer = message.get_buffer();
            ASSERT_FALSE(buffer.is_reused());
            ASSERT_EQ(buffer_pool->outstanding_lease_count(), 1);

            ASSERT_FALSE(buffer.has_moved());
            message.get_header()->request_id = 55;

            fbs::Builder builder{1024};
            auto proto = CreateDeleteWorkerRequestProtoDirect(builder, log_context.get_context().c_str(), worker_id, worker_version);
            builder.Finish(proto);
            const auto *buff = builder.GetBufferPointer();
            blob_sz = builder.GetSize();
            message.resize_for_payload(blob_sz);
            std::memcpy(message.get_payload_raw(), buff, blob_sz);

            message.get_header()->payload_size = blob_sz;

            ASSERT_EQ(message.get_payload_size(), blob_sz);
            ASSERT_EQ(message.get_payload()->worker_id(), worker_id);
            ASSERT_EQ(message.get_payload()->worker_version(), worker_version);

            raw_ptr = message.get_header_raw();
            auto fun = [raw_ptr, blob_sz, worker_id, worker_version, class_id, primary_key_str, buffer_pool{buffer_pool}, old_message{&message}, message{std::move(message)}]() mutable {
                ASSERT_EQ(raw_ptr, message.get_header_raw()); //after move, real buffer ptr should be the same
                ASSERT_EQ(buffer_pool->outstanding_lease_count(), 1);
                ASSERT_EQ(buffer_pool->buffer_queue_count(), 0);
                ASSERT_NE(old_message, &message);
                ASSERT_TRUE(old_message->get_buffer().has_moved());
                ASSERT_FALSE(message.get_buffer().has_moved());
                ASSERT_EQ(message.get_header()->request_id, 55);

                ASSERT_EQ(message.get_payload_size(), blob_sz);
                ASSERT_EQ(message.get_payload()->worker_id(), worker_id);
                ASSERT_EQ(message.get_payload()->worker_version(), worker_version);
            };
            fun();
        }
        ASSERT_EQ(buffer_pool->outstanding_lease_count(), 0);
        ASSERT_EQ(buffer_pool->buffer_queue_count(), 1);
    }
    {
        auto message = buffer_pool->get_envelope<Innerspace::RequestHeader, DeleteWorkerRequestProto, u32>();
        ASSERT_TRUE(message.get_buffer().is_reused());
        ASSERT_EQ(buffer_pool->outstanding_lease_count(), 1);
        ASSERT_EQ(buffer_pool->buffer_queue_count(), 0);
        ASSERT_EQ(message.get_total_size(), 0);
        message.resize_for_payload(blob_sz);
        ASSERT_EQ(message.get_total_size(), blob_sz + sizeof(Innerspace::RequestHeader));
        ASSERT_EQ(message.get_header()->payload_size, 0);
        ASSERT_EQ(message.get_header()->request_id, 0);
        ASSERT_EQ(message.get_payload()->worker_id(), 0);
        ASSERT_EQ(message.get_payload()->worker_version(), 0);
    }
    ASSERT_EQ(buffer_pool->outstanding_lease_count(), 0);
    ASSERT_EQ(buffer_pool->buffer_queue_count(), 1);
}

#pragma clang diagnostic pop
