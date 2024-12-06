cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME estate-runtime)
project(${PROJECT_NAME})

include(../../flags.cmake)

# set(DEPS_DIR ../../deps)

set(INCLUDE_FILES
        include/estate/runtime/protocol/interface_generated.h
        include/estate/runtime/protocol/model_generated.h
        include/estate/runtime/protocol/worker_process_engine_model_generated.h
        include/estate/runtime/protocol/worker_process_interface_generated.h
        include/estate/runtime/protocol/serenity_interface_generated.h
        include/estate/runtime/protocol/WorkerIndexProto_generated.h
        include/estate/runtime/deps/flatbuffers.h
        include/estate/runtime/code.h
        include/estate/runtime/numeric_types.h
        include/estate/runtime/result.h
        include/estate/runtime/version.h
        include/estate/runtime/model_types.h
        include/estate/runtime/event.h
        include/estate/runtime/buffer_view.h
        include/estate/runtime/blob.h
        include/estate/runtime/enum_op.h
        include/estate/runtime/limits.h
        include/estate/runtime/version_generated.h)
set(SOURCE_FILES
        src/deps/flatbuffers.cpp
        src/code.cpp
        src/event.cpp
        src/model_types.cpp)

add_library(${PROJECT_NAME}
        ${INCLUDE_FILES}
        ${SOURCE_FILES}
        )

target_link_directories(${PROJECT_NAME}
        PUBLIC
        /usr/local/lib
        $ENV{ESTATE_NATIVE_DEPS_DIR}/hiredis
        $ENV{ESTATE_NATIVE_DEPS_DIR}/mstch/cmake-build-release/src
        $ENV{ESTATE_NATIVE_DEPS_DIR}/flatbuffers/cmake-build-release
        )

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $ENV{ESTATE_NATIVE_DEPS_DIR}/boost/include
        include
        $ENV{ESTATE_NATIVE_DEPS_DIR}/flatbuffers/include
        )
