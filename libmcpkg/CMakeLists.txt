cmake_minimum_required(VERSION 3.16)
set(TARGET_NAME libmcpkg)
project(${TARGET_NAME}
    VERSION 0.1
    LANGUAGES C)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mcpkg_activate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcpkg_cache.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mcpkg_get.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/code_names.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/array_helper.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/mcpkg_fs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/api/mcpkg_info_entry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/api/mcpkg_deps_entry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/api/mcpkg_entry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/api/mcpkg_api_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/api/modrith_client.c
)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/mcpkg_visited_set.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mcpkg_activate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/compat.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/mcpkg_export.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/mcpkg_msgpack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mcpkg.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mcpkg_cache.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mcpkg_get.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mcpkg_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/mcpkg_fs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/code_names.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/array_helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/api/mcpkg_info_entry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/api/mcpkg_deps_entry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/api/mcpkg_entry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/api/mcpkg_api_client.h
    ${CMAKE_CURRENT_SOURCE_DIR}/api/modrith_client.h
)

if (STATIC_BUILD)
    add_library(${TARGET_NAME} STATIC ${HEADERS} ${SOURCES}) 
else()
    add_library(${TARGET_NAME} ${HEADERS} ${SOURCES})
endif()


target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

target_link_libraries(${TARGET_NAME} PRIVATE ${BASE_LIBS})

## setup install and in the next couple of days public vs private headers
if (UNIX)
    install(DIRECTORY /var/cache/mcpkg DESTINATION /var/cache)
    install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mcpkg)
    install(TARGETS ${TARGET_NAME}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else()
# not sure how to handle windows install.
endif()




