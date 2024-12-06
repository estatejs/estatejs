cmake_minimum_required(VERSION 3.15)
project(tests)

include(../flags.cmake)

# set(DEPS_DIR ../deps)

add_executable(tests
        main.cpp
        estate_test.h
        estate_test.cpp
        logging.h
        contract/call_service_method_tests.cpp
        contract/setup_worker_tests.cpp
        contract/delete_worker_tests.cpp
        contract/get_save_object_tests.cpp
        contract/innerspace_tests.cpp
        unit/buffer_pool_tests.cpp
        unit/thread_pool_tests.cpp
        logging.cpp val_def.h)

target_link_directories(tests
        PUBLIC
        /usr/local/lib
        $ENV{ESTATE_NATIVE_DEPS_DIR}/googletest/cmake-build-release/lib
        )

target_include_directories(tests
        PUBLIC
        ../lib/runtime/include
        ../lib/internal/include
        ../lib/serenity/include
        $ENV{ESTATE_NATIVE_DEPS_DIR}/nlohmann
        $ENV{ESTATE_NATIVE_DEPS_DIR}/googletest/googletest/include
)

target_link_libraries(tests
        estate-runtime
        estate-internal
        estate-serenity
        gtest
        stdc++fs
        stdc++
        m
        boost_timer
        rt
        )

add_test(NAME tests COMMAND tests)
