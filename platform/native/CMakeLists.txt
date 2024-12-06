cmake_minimum_required(VERSION 3.15)
project(estate-native)
include(./flags.cmake)

include(CMakeRC.cmake)

add_subdirectory(lib/internal)
add_subdirectory(lib/serenity)
add_subdirectory(lib/serenity-client)
add_subdirectory(lib/river)
add_subdirectory(lib/runtime)
add_subdirectory(daemon/serenity)
add_subdirectory(daemon/river)
add_subdirectory(tests)
