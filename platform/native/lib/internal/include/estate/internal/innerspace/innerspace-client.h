//
// Created by sjone on 1/21/2022.
//

#pragma once

#include <unordered_map>

#include "estate/internal/innerspace/innerspace.h"
#include "estate/runtime/protocol/worker_process_interface_generated.h"

#include <estate/runtime/result.h>
#include <estate/runtime/model_types.h>

namespace estate::innerspace {
    using InnerspaceWorkerLoaderClientConfig = Innerspace<GetWorkerProcessEndpointRequestProto, GetWorkerProcessEndpointResponseProto>::Client::Config;

    struct InnerspaceClientConfig {
        struct AsWorkerUser {
            std::string host;
            u8 user_connection_count;
            u32 user_max_request_size;
            u32 user_max_response_size;
            static AsWorkerUser FromRemote(const LocalConfigurationReader &reader) {
                return AsWorkerUser{
                        reader.get_string("host"),
                        reader.get_u8("user_connection_count"),
                        reader.get_u32("user_max_request_size"),
                        reader.get_u32("user_max_response_size")
                };
            }
        };
        struct AsWorkerAdmin {
            std::string host;
            u8 setup_worker_connection_count;
            u32 setup_worker_max_request_size;
            u32 setup_worker_max_response_size;
            u8 delete_worker_connection_count;
            u32 delete_worker_max_request_size;
            u32 delete_worker_max_response_size;
            static AsWorkerAdmin FromRemote(const LocalConfigurationReader &reader) {
                return AsWorkerAdmin{
                        reader.get_string("host"),
                        reader.get_u8("setup_worker_connection_count"),
                        reader.get_u32("setup_worker_max_request_size"),
                        reader.get_u32("setup_worker_max_response_size"),
                        reader.get_u8("delete_worker_connection_count"),
                        reader.get_u32("delete_worker_max_request_size"),
                        reader.get_u32("delete_worker_max_response_size")
                };
            }
        };
    };

    class InnerspaceClient {
        class Impl;
        Impl* _impl {nullptr};
    public:
        ~InnerspaceClient();
        InnerspaceClient(BufferPoolS buffer_pool, IoContextS io_context, InnerspaceWorkerLoaderClientConfig worker_loader_config,
                         InnerspaceClientConfig::AsWorkerAdmin admin_config);
        InnerspaceClient(BufferPoolS buffer_pool, IoContextS io_context, InnerspaceWorkerLoaderClientConfig worker_loader_config,
                         InnerspaceClientConfig::AsWorkerUser user_config);
        void async_send(const LogContext &log_context, WorkerId worker_id,
                        Buffer<UserRequestProto> request_buffer,
                        Innerspace<UserRequestProto, WorkerProcessUserResponseProto>::Client::Connection::ResponseHandler response_handler);
        void async_send(const LogContext &log_context, WorkerId worker_id,
                        Buffer<SetupWorkerRequestProto> request_buffer,
                        Innerspace<SetupWorkerRequestProto, SetupWorkerResponseProto>::Client::Connection::ResponseHandler response_handler);
        void async_send(const LogContext &log_context, WorkerId worker_id,
                        Buffer<DeleteWorkerRequestProto> request_buffer,
                        Innerspace<DeleteWorkerRequestProto, DeleteWorkerResponseProto>::Client::Connection::ResponseHandler response_handler);
    };
    using InnerspaceClientS = std::shared_ptr<InnerspaceClient>;
}

