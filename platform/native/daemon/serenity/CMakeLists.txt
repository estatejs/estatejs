cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME serenityd)
project(${PROJECT_NAME})

include(../../flags.cmake)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
        estate-internal
        estate-serenity
        stdc++fs
        stdc++
        m
        boost_timer
        procps
        rt
        )

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ../../lib/internal/include
        ../../lib/runtime/include
        ../../lib/serenity/include
        )