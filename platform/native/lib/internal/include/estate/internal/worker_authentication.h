//
// Originally written by Scott Jones.
// Copyright (c) 2020 Warpdrive Technologies, Inc. All rights reserved.
//

#pragma once

#include "estate/internal/deps/redis++.h"
#include "estate/internal/local_config.h"

#include <estate/runtime/model_types.h>
#include <estate/runtime/result.h>

namespace estate {
    struct WorkerAuthenticationConfig {
        std::string redis_endpoint;
        std::size_t connection_count;
        std::chrono::milliseconds connection_wait_timeout;
        std::chrono::milliseconds connection_lifetime;
        static WorkerAuthenticationConfig FromRemote(const LocalConfigurationReader &getter) {
            return WorkerAuthenticationConfig{
                    getter.get_string("redis_endpoint"),
                    getter.get_u64("connection_count"),
                    std::chrono::milliseconds(getter.get_i64("connection_wait_timeout_ms")),
                    std::chrono::milliseconds(getter.get_i64("connection_lifetime_ms"))
            };
        }
    };
    struct WorkerAuthentication {
        WorkerAuthentication(const WorkerAuthenticationConfig &config);
        ResultCode <WorkerId> try_get_workerid_by_user_key(std::string_view user_key);
        WorkerAuthentication(const WorkerAuthentication &other) = delete;
    private:
        RedisU _redis;
    };
    using WorkerAuthenticationS = std::shared_ptr<WorkerAuthentication>;
}
