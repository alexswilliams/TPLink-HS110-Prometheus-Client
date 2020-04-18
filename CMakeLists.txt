cmake_minimum_required(VERSION 3.7)
project(TPLink-HS110-Metrics-Client LANGUAGES C)

find_package(cJSON REQUIRED)
include_directories(SYSTEM /usr/local/include)

add_executable(
        tplink-hs110-client
        src/main.c
        src/config.c src/config.h
        src/metrics.c src/metrics.h
        src/device.c src/device.h
        src/prometheus.c src/prometheus.h
        src/connection.c src/connection.h)

target_compile_options(tplink-hs110-client PRIVATE -Os -Wall -Wextra)

if (BUILD_STATIC STREQUAL On)
    set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
    set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
    target_link_libraries(tplink-hs110-client -static cjson.a)
else ()
    target_link_libraries(tplink-hs110-client cjson)
endif ()
