cmake_minimum_required(VERSION 3.16)
set(TARGET_NAME mcpkg)
project(${TARGET_NAME}
    VERSION 0.1
    LANGUAGES C)

set(SOURCES
    main.c
    update.c
    cache.c
    get.c
    config.c
    activate.c
)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/update.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cache.h
    ${CMAKE_CURRENT_SOURCE_DIR}/get.h
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/activate.h
)

add_executable(${TARGET_NAME} ${HEADERS} ${SOURCES})
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${TARGET_NAME} PRIVATE
    ${BASE_LIBS}
    libmcpkg
)
if (UNIX)
    install(TARGETS ${TARGET_NAME}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else()
# not sure how to handle windows install.
endif()
