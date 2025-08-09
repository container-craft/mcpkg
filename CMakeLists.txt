cmake_minimum_required(VERSION 3.10)
project(minecraft_package_manager VERSION 0.1)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(STATIC_BUILD "Build mcpkg as a static binary" OFF)

if (UNIX)
    find_package(PkgConfig REQUIRED)
    # PkgConfig Debian Sid, names libzstd.pc json-c.pc libcurl.pc msgpack-c.pc
    pkg_check_modules(LibZStd  REQUIRED libzstd)       # libzstd-dev
    pkg_check_modules(LibCJson REQUIRED libcjson)      # libcjson-dev
    pkg_check_modules(LibCUrl  REQUIRED libcurl )      # libcurl4-openssl-dev
    pkg_check_modules(LibMsgPackC REQUIRED msgpack-c)  # libmsgpack-c-dev
    include(GNUInstallDirs)
    list(APPEND BASE_LIBS
        ${LibMsgPackC_LIBRARIES}
        ${LibZStd_LIBRARIES}
        ${LibCUrl_LIBRARIES}
        ${LibCJson_LIBRARIES}
    )
else()
    find_package(Zstd    CONFIG REQUIRED)   # https://facebook.github.io/zstd/
    find_package(CURL    CONFIG REQUIRED)   # https://curl.se/windows/
    find_package(cJSON   CONFIG REQUIRED)   # https://github.com/DaveGamble/cJSON/blob/master/README.md vcpkg provides cJSON as "cJSON"
    find_package(msgpack CONFIG REQUIRED)   # https://msgpack.org/ targets: msgpackc (C)

    ## I have no clue if this links
    list(APPEND BASE_LIBS
        ZSTD::libzstd
        CURL::libcurl
        cjson::cjson
        msgpackc
    )
endif()

add_subdirectory(libmcpkg)
add_subdirectory(mcpkg)
