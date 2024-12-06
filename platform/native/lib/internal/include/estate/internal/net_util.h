//
// Created by scott on 4/26/20.
//

#pragma once

#include <estate/internal/deps/boost.h>
#include <estate/runtime/result.h>
#include <string>

namespace estate {
    Result<boost::asio::ip::address> get_ip_address(const std::string& interface_name);
    Endpoint make_endpoint(const std::string address, u16 port);
}