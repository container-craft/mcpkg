cmake_minimum_required(VERSION 3.16)
set(TARGET_NAME libmcpkg)
project(${TARGET_NAME}
    VERSION 0.1
    LANGUAGES C)

set(SOURCES
    mcpkg_cache.c
    utils/code_names.c
    utils/array_helper.c
    utils/mcpkg_fs.c
    api/mcpkg_info_entry.c
    api/mcpkg_deps_entry.c
    api/mcpkg_entry.c
    api/mcpkg_api_client.c
    api/modrith_client.c
)

set(HEADERS
    mcpkg.h
    mcpkg_cache.h
    utils/mcpkg_fs.h
    utils/code_names.h
    utils/array_helper.h
    api/mcpkg_info_entry.h
    api/mcpkg_deps_entry.h
    api/mcpkg_entry.h
    api/mcpkg_api_client.h
    api/modrith_client.h
)

add_library(${TARGET_NAME} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/api
)

target_link_libraries(${TARGET_NAME} PRIVATE
    ${LibMsgPackC_LIBRARIES}
    ${LibZStd_LIBRARIES}
    ${LibCUrl_LIBRARIES}
    ${LibCJson_LIBRARIES}
)
