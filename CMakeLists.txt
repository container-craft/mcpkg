cmake_minimum_required(VERSION 3.10)
project(minecraft_package_manager VERSION 0.1)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MCPKG_BUILD_SHARED "Build libmcpkg as a shared" ON)
## test options
option(TST_BUILD "Build tests for modules" ON)
option(TST_VERBOSE "More output to uart no effect when TST_BUILD is off" ON)
option(TST_COMPILE_FAIL "compile time checks that should fail" OFF)
option(TST_ONLINE "test downloads and other things that need internet" ON )

## generator
option(MCPGEN "option to generate the API message pack code" OFF)

if (UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set_property(GLOBAL PROPERTY CMAKE_CXX_STANDARD 23)
    add_compile_definitions(_POSIX_C_SOURCE=200809L)
    add_compile_definitions(_GNU_SOURCE)
    find_package(PkgConfig REQUIRED)
    find_package(Threads REQUIRED)
    # PkgConfig Debian Sid, names libsodium.pc libzstd.pc json-c.pc libcurl.pc msgpack-c.pc in /usr/lib/${MULTI_ARCH}/pkgconfig
    pkg_check_modules(LibZStd  REQUIRED libzstd)       # libzstd-dev
    pkg_check_modules(LibCJson REQUIRED libcjson)      # libcjson-dev
    pkg_check_modules(LibCUrl  REQUIRED libcurl )      # libcurl4-openssl-dev
    pkg_check_modules(LibMsgPackC REQUIRED msgpack-c)  # libmsgpack-c-dev
    pkg_check_modules(LibSodium REQUIRED libsodium)    # libsodium-dev
    pkg_check_modules(LibPthread REQUIRED libsodium)    # libsodium-dev
    include(GNUInstallDirs)
    list(APPEND BASE_LIBS
        ${LibMsgPackC_LIBRARIES}
        ${LibZStd_LIBRARIES}
        ${LibCUrl_LIBRARIES}
        ${LibCJson_LIBRARIES}
        ${LibSodium_LIBRARIES}
        Threads::Threads
    )
else()
    find_package(Zstd    CONFIG REQUIRED)   # https://facebook.github.io/zstd/
    find_package(CURL    CONFIG REQUIRED)   # https://curl.se/windows/
    find_package(cJSON   CONFIG REQUIRED)   # https://github.com/DaveGamble/cJSON/blob/master/README.md vcpkg provides cJSON as "cJSON"
    find_package(msgpack CONFIG REQUIRED)   # https://msgpack.org/ targets: msgpackc (C)
    find_package(sodium  CONFIG REQUIRED)   # https://doc.libsodium.org/installation#pre-built-libraries

    ## I have no clue if this links
    list(APPEND BASE_LIBS
        ZSTD::libzstd
        CURL::libcurl
        cjson::cjson
        msgpackc
        sodium:sodium
        ws2_32
    )
endif()


if (MCPGEN)
    add_subdirectory(mpgen)
endif()

add_subdirectory(libmcpkg)

## disabled for now while the backend gets refactored
# add_subdirectory(mcpkg)

if(TST_BUILD)
    add_subdirectory(tests)
endif()
