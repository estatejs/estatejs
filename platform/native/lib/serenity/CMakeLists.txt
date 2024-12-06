cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME estate-serenity)
project(${PROJECT_NAME})

include(../../flags.cmake)

add_library(${PROJECT_NAME}
        include/estate/internal/serenity/handler/admin.h
        include/estate/internal/serenity/handler/get_worker_process_endpoint.h
        include/estate/internal/serenity/handler/user.h
        include/estate/internal/serenity/system/worker-process.h
        include/estate/internal/serenity/system/worker-loader.h
        include/estate/internal/serenity/worker-process-table.h
        src/handler/admin.cpp
        src/handler/get_worker_process_endpoint.cpp
        src/system/worker-loader.cpp
        src/handler/user.cpp
        src/system/worker-process.cpp
        src/worker-process-table.cpp
        include/estate/internal/serenity/system/launcher.h
        src/system/launcher.cpp)

target_link_libraries(${PROJECT_NAME}
        estate-runtime
        estate-internal
        stdc++fs
        stdc++
        m
        boost_timer
        )

target_include_directories(${PROJECT_NAME}
        PUBLIC
        include
        )
