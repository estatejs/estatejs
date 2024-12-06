//
// Created by scott on 4/26/20.
//

#include "estate/internal/net_util.h"

#include <estate/internal/deps/boost.h>
#include <estate/runtime/result.h>

#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

namespace estate {
    Result<boost::asio::ip::address> get_ip_address(const std::string &interface_name) {
        using Result = Result<boost::asio::ip::address>;

        struct ifaddrs *ifAddrStruct = nullptr;
        struct ifaddrs *ifa = nullptr;
        void *tmpAddrPtr = nullptr;

        getifaddrs(&ifAddrStruct);

        for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr) {
                continue;
            }
            if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
                // is a valid IP4 Address
                tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
                char addressBuffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
                if (strcmp(ifa->ifa_name, interface_name.c_str()) == 0) {
                    freeifaddrs(ifa);
                    return Result::Ok(boost::asio::ip::make_address(addressBuffer));
                }
            } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
                // is a valid IP6 Address
                tmpAddrPtr = &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr;
                char addressBuffer[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
                if (strcmp(ifa->ifa_name, interface_name.c_str()) == 0) {
                    freeifaddrs(ifa);
                    auto address = boost::asio::ip::make_address(addressBuffer);
                    return Result::Ok(boost::asio::ip::make_address(addressBuffer));
                }
            }
        }

        if (ifAddrStruct != nullptr)
            freeifaddrs(ifAddrStruct);

        return Result::Error();
    }
    Endpoint make_endpoint(std::string address, u16 port) {
        return boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port);
    }
}