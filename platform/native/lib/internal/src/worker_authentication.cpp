//
// Originally written by Scott R. Jones.
// Copyright (c) 2020 Warpdrive Technologies, Inc. All rights reserved.
//

#include "estate/internal/worker_authentication.h"
#include "estate/internal/logging.h"

namespace estate {
    WorkerAuthentication::WorkerAuthentication(const WorkerAuthenticationConfig &config) {
        sw::redis::ConnectionPoolOptions pool_options{
                config.connection_count,
                config.connection_wait_timeout,
                config.connection_lifetime
        };
        sw::redis::ConnectionOptions options{
                config.redis_endpoint
        };
        _redis = std::make_unique<sw::redis::Redis>(options, pool_options);
    }
    ResultCode<WorkerId> WorkerAuthentication::try_get_workerid_by_user_key(std::string_view user_key) {
        using Result = ResultCode<WorkerId>;
        //NOTE: This must match what's in dotnet/Jayne/Services/Impl/WorkerKeyCacheServiceImpl.cs
        std::string key{"uk:"};
        key.append(user_key);
        const auto worker_id_str_o = _redis->get(key);
        if(!worker_id_str_o) {
            return Result::Error(Code::WorkerAuthentication_UserKeyNotFound);
        }
        try {
            const auto worker_id = std::stoull(*worker_id_str_o);
            return Result::Ok(worker_id);
        }
        catch (std::exception ex) {
            sys_log_critical("Invalid worker id value found for key {}", key);
            return Result::Error(Code::WorkerAuthentication_InvalidValueFound);
        }
    }
}