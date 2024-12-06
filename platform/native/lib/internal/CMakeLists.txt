cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME estate-internal)
project(${PROJECT_NAME})

include(../../flags.cmake)

cmrc_add_resource_library(javascript-resources ALIAS estate::js::rc NAMESPACE estate::js::rc
        js/Data-factory.js.mustache
        js/Data-factory-imports.js.in
        js/Service-factory.js.mustache
        js/Service-factory-imports.js.in
        )

set(INCLUDE_FILES
        include/estate/internal/innerspace/innerspace.h
        include/estate/internal/outerspace/outerspace.h
        include/estate/internal/stopwatch.h
        include/estate/internal/flatbuffers_util.h
        include/estate/internal/thread_pool.h
        include/estate/internal/processor/processor.h
        include/estate/internal/net_util.h
        include/estate/internal/buffer_pool.h
        include/estate/internal/pool.h
        include/estate/internal/processor/service.h
        include/estate/internal/file_util.h
        include/estate/internal/logging.h
        include/estate/internal/deps/v8.h
        include/estate/internal/deps/spdlog.h
        include/estate/internal/deps/fmt.h
        include/estate/internal/deps/rocksdb.h
        include/estate/internal/deps/boost.h
        include/estate/internal/processor/service_provider/buffer_pool_service_provider.h
        include/estate/internal/processor/service_provider/database_service_provider.h
        include/estate/internal/processor/service_provider/script_engine_service_provider.h
        include/estate/internal/worker_authentication.h
        include/estate/internal/local_config.h
        include/estate/internal/deps/redis++.h
        include/estate/internal/database_keys.h
        include/estate/internal/server/server.h
        include/estate/internal/server/server_fwd.h
        include/estate/internal/server/v8_macro.h
        include/estate/internal/server/server_decl.h
        include/estate/internal/server/v8_macro_undef.inl
        include/estate/internal/outerspace/subscription.h
        include/estate/internal/outerspace/outerspace-fwd.h
        include/estate/internal/env_util.h
        include/estate/internal/innerspace/innerspace-client.h)

set(SOURCE_FILES
        src/server/server_rc.inl
        src/stopwatch.cpp
        src/thread_pool.cpp
        src/server/server.cpp
        src/processor/service_provider/database_service_provider.cpp
        src/processor/service_provider/buffer_pool_service_provider.cpp
        src/outerspace/outerspace.cpp
        src/net_util.cpp
        src/file_util.cpp
        src/logging.cpp
        src/deps/fmt.cpp
        src/deps/rocksdb.cpp
        src/deps/spdlog.cpp
        src/deps/v8.cpp
        src/deps/boost.cpp
        src/worker_authentication.cpp
        src/local_config.cpp
        src/buffer_pool.cpp
        src/database_keys.cpp
        src/outerspace/subscription.cpp
        src/innerspace/innerspace-client.cpp
        src/innerspace/innerspace.cpp
        src/env_util.cpp)

add_library(${PROJECT_NAME}
        ${INCLUDE_FILES}
        ${SOURCE_FILES}
        )

target_link_directories(${PROJECT_NAME}
        PUBLIC
        /usr/local/lib
        $ENV{ESTATE_NATIVE_DEPS_DIR}/boost/lib
        $ENV{ESTATE_NATIVE_DEPS_DIR}/rocksdb/cmake-build-release
        $ENV{ESTATE_NATIVE_DEPS_DIR}/fmt/cmake-build
        $ENV{ESTATE_NATIVE_DEPS_DIR}/spdlog/cmake-build-release
        $ENV{ESTATE_NATIVE_DEPS_DIR}/flatbuffers/cmake-build-release
        $ENV{ESTATE_NATIVE_DEPS_DIR}/redis-plus-plus/cmake-build-release
        $ENV{ESTATE_NATIVE_DEPS_DIR}/v8-slim/v8/out/x64.release.static/obj
        )

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        estate-runtime
        spdlog
        #NOTE: as of 1/24/21 Serenity crashes on call method because the thread the Isolate was created on
        #is different than the thread it's being called on.
#        debug ${V8_MONOLITH_DEBUG}
#        optimized ${V8_MONOLITH_RELEASE}
#        ${V8_MONOLITH_RELEASE}
        v8_monolith
        boost_thread
        boost_system
        boost_filesystem
        boost_regex
        boost_program_options
        pthread
        fmt
        rocksdb
        lz4
        snappy
        z
        dl
        zstd
        bz2
        redis++
        hiredis
        mstch

        PRIVATE
        estate::js::rc
        )

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $ENV{ESTATE_NATIVE_DEPS_DIR}/boost/include
        include
        ../runtime/include
        $ENV{ESTATE_NATIVE_DEPS_DIR}
        $ENV{ESTATE_NATIVE_DEPS_DIR}/rocksdb/include
        $ENV{ESTATE_NATIVE_DEPS_DIR}/fmt/include
        $ENV{ESTATE_NATIVE_DEPS_DIR}/v8-slim/v8/include
        $ENV{ESTATE_NATIVE_DEPS_DIR}/spdlog/include
        $ENV{ESTATE_NATIVE_DEPS_DIR}/flatbuffers/include
        $ENV{ESTATE_NATIVE_DEPS_DIR}/redis-plus-plus/src
        $ENV{ESTATE_NATIVE_DEPS_DIR}/nlohmann
        $ENV{ESTATE_NATIVE_DEPS_DIR}/mstch/include
        )
