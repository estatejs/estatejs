cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME riverd)
project(${PROJECT_NAME})

include(../../flags.cmake)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
        estate-internal
        estate-river
        stdc++fs
        stdc++
        )

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ../../lib/internal/include
        ../../lib/runtime/include
        ../../lib/river/include
        )