cmake_minimum_required(VERSION 3.10)
project(minecraft_package_manager VERSION 0.1)


find_package(PkgConfig REQUIRED)
# PC names libzstd.pc json-c.pc libcurl.pc msgpack-c.pc
pkg_check_modules(LibZStd  REQUIRED libzstd)       # libzstd-dev
pkg_check_modules(LibCJson REQUIRED libcjson)      # libcjson-dev
pkg_check_modules(LibCUrl  REQUIRED libcurl )      # libcurl4-openssl-dev
pkg_check_modules(LibMsgPackC REQUIRED msgpack-c)  # libavahi-client-dev
#target_link_libraries(testapp ${VAR_LIBRARIES})
#target_include_directories(testapp PUBLIC ${VAR_INCLUDE_DIRS})
#target_compile_options(testapp PUBLIC ${VAR_CFLAGS_OTHER})

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(libmcpkg)
add_subdirectory(mcpkg)
