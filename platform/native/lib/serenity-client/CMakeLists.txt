cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME estate-serenity-client)
project(${PROJECT_NAME})

include(../../flags.cmake)

add_library(${PROJECT_NAME} SHARED
        src/serenity-client.cpp
        include/serenity-client.h
        )

target_link_libraries(${PROJECT_NAME}
        estate-runtime
        estate-internal
        )

target_link_directories(${PROJECT_NAME}
        PUBLIC
        /usr/local/lib
        )

target_include_directories(${PROJECT_NAME}
        PUBLIC
        include
        ../../lib/runtime/include
        ../../lib/internal/include
        )