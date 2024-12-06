//
// Created by sjone on 1/24/2022.
//

#include <estate/internal/innerspace/innerspace.h>

namespace estate {
    ResultCode<ResolvedEndpoint, Code> resolve_endpoint(const std::string &host, u16 port, IoContextS io_context) {
        using Result = ResultCode<ResolvedEndpoint, Code>;
        Resolver resolver{*io_context};

        boost::system::error_code ec;
        auto endpoint = resolver.resolve(host, std::to_string(port), ec);
        if (ec) {
            sys_log_critical("Unable to resolve {}:{}. Error: {}", host, port, ec.message());
            return Result::Error(Code::Innerspace_UnableToResolveHostPort);
        }
        if (endpoint.empty()) {
            sys_log_critical("Unable to resolve {}:{}. No endpoints returned.", host, port);
            return Result::Error(Code::Innerspace_NoEndpoints);
        }

        if (endpoint.size() > 1) {
            sys_log_warn("Resolution to {}:{} expected a single endpoint but returned {}.", host, port, endpoint.size());
        }

        return Result::Ok(endpoint);
    }
    bool Breaker::fault() {
        bool prev{false};
        return _fault.compare_exchange_strong(prev, true);
    }
    bool Breaker::is_faulted() const {
        return _fault;
    }
    std::size_t InnerspaceClientEndpoint::Hasher::operator()(const InnerspaceClientEndpoint &client_endpoint) const {
        using boost::hash_value;
        using boost::hash_combine;
        std::size_t seed = 0;
        hash_combine(seed, hash_value(client_endpoint.host));
        hash_combine(seed, hash_value(client_endpoint.port));
        return seed;
    }
    bool InnerspaceClientEndpoint::operator==(const InnerspaceClientEndpoint &rhs) const {
        return host == rhs.host &&
               port == rhs.port;
    }
}