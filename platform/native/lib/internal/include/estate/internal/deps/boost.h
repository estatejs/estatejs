//
// Created by scott on 4/25/20.
//
#pragma once

//see https://github.com/boostorg/asio/issues/312
#define BOOST_ASIO_DISABLE_CONCEPTS
#define BOOST_BEAST_USE_STD_STRING_VIEW
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <boost/any.hpp>
#include <boost/config.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/beast.hpp>
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/beast/core/buffers_adaptor.hpp>
#include <boost/variant.hpp>
#include <boost/crc.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/anonymous_shared_memory.hpp>

#include <memory>
using IoContext = boost::asio::io_context;
using IoContextS = std::shared_ptr<IoContext>;
using Socket = boost::asio::ip::tcp::socket;
using Resolver = boost::asio::ip::tcp::resolver;
using Acceptor = boost::asio::ip::tcp::acceptor;
using ResolvedEndpoint = boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>;
using Endpoint = boost::asio::ip::tcp::endpoint;
using ExecutorStrand = boost::asio::strand<boost::asio::io_context::executor_type>;
