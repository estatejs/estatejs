//
// Created by scott on 5/18/20.
//
#pragma once

#include <estate/internal/processor/processor.h>
#include <estate/runtime/numeric_types.h>
#include <estate/internal/innerspace/innerspace-client.h>
#include <estate/internal/outerspace/outerspace.h>
#include <estate/internal/buffer_pool.h>
#include <estate/internal/local_config.h>
#include <estate/runtime/protocol/worker_process_interface_generated.h>

namespace estate {
    struct RiverProcessorConfig {
        size_t min_primary_key_length;
        size_t max_primary_key_length;

        static RiverProcessorConfig FromRemote(const LocalConfigurationReader &reader) {
            return RiverProcessorConfig{
                    reader.get_u64("min_primary_key_length"),
                    reader.get_u64("max_primary_key_length"),
            };
        }
    };

    struct RiverServiceProvider {
        Service<BufferPool> _buffer_pool;
        Service<outerspace::SubscriptionManager> _object_update_manager;
        Service<innerspace::InnerspaceClient> _innerspace_client;
    public:
        RiverServiceProvider(BufferPoolS buffer_pool,
                             outerspace::SubscriptionManagerS subscription_manager,
                             innerspace::InnerspaceClientS innerspace_client
        );
        [[nodiscard]] BufferPoolS get_buffer_pool();
        [[nodiscard]] outerspace::SubscriptionManagerS get_subscription_manager();
        [[nodiscard]] innerspace::InnerspaceClientS get_innerspace_client();
    };
    using RiverServiceProviderS = std::shared_ptr<RiverServiceProvider>;

    using RequestContextS = std::shared_ptr<outerspace::RequestContext>;

    void execute(const RiverProcessorConfig &config,
                 RiverServiceProviderS service_provider,
                 Buffer<UserRequestProto> &&request_buffer,
                 RequestContextS request_context);

    using RiverProcessor = Processor<
            RiverProcessorConfig,
            UserRequestProto,
            RiverUserResponseProto,
            RiverServiceProvider,
            outerspace::RequestContext,
            Buffer<UserRequestProto>,
            execute>;
}
