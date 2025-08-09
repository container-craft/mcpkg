cmake_minimum_required(VERSION 3.16)
set(TARGET_NAME mcpkg)
project(${TARGET_NAME}
    VERSION 0.1
    LANGUAGES C)

set(SOURCES
    main.c
    update.c
    cache.c
    install.c
)

set(HEADERS
    update.h
    cache.h
    install.h
)

add_executable(${TARGET_NAME}
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_libraries(${TARGET_NAME} PRIVATE
    libmcpkg
    ${LibMsgPackC_LIBRARIES}
    ${LibZStd_LIBRARIES}
    ${LibCUrl_LIBRARIES}
    ${LibCJson_LIBRARIES}
)
