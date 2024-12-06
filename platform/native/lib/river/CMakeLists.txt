cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME estate-river)
project(${PROJECT_NAME})

include(../../flags.cmake)

# set(DEPS_DIR ../../deps)

set(INCLUDE_FILES
        include/estate/internal/river/river.h
        include/estate/internal/river/handler.h
        )

set(SOURCE_FILES
        src/river.cpp
        src/handler.cpp
        )

add_library(${PROJECT_NAME}
        ${INCLUDE_FILES}
        ${SOURCE_FILES}
        )

target_link_libraries(${PROJECT_NAME}
        estate-runtime
        estate-internal
        stdc++fs
        stdc++
        m
        )

target_link_directories(${PROJECT_NAME}
        PUBLIC
        /usr/local/lib
        )

target_include_directories(${PROJECT_NAME}
        PUBLIC
        include
        )