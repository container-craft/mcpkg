cmake_minimum_required(VERSION 3.16)
set(TARGET_NAME libmcpkg)
project(${TARGET_NAME}
    VERSION 0.1
    LANGUAGES C)

  # str/array_helper.c
set(MCPKG_SOURCES
  ## Api
  # api/mcpkg_activate.c
  # api/mcpkg_cache.c
  # api/mcpkg_get.c
  # api/mcpkg_api_client.c

  ## Filesystem
  # fs/mcpkg_fs.c
  # fs/mcpkg_fs.c
  fs/mcpkg_fs_dir.c
  fs/mcpkg_fs_file.c
  fs/mcpkg_fs_util.c
  fs/mcpkg_fs_error.c
  fs/mcpkg_fs_std_paths.c



  # providers/modrith_client.c

  ## Packages meta-data
  # packages/mcpkg_info_entry.c
  # packages/mcpkg_entry.c
  # packages/mcpkg_deps_entry.c

  ## Lists
  container/mcpkg_list.c
  container/mcpkg_hash.c
  container/mcpkg_map.c
  container/mcpkg_str_list.c

  # Crypto
  crypto/mcpkg_crypto.c
  crypto/mcpkg_crypto_hash.c
  crypto/mcpkg_crypto_hex.c
  crypto/mcpkg_crypto_init.c
  # crypto/mcpkg_crypto_io.c
  crypto/mcpkg_crypto_provider_verify.c
  crypto/mcpkg_crypto_rand.c
  crypto/mcpkg_crypto_sign.c
  crypto/mcpkg_crypto_util.c
  crypto/mcpkg_sip_hash.c
  crypto/third_party/md5/md5sum.c
  crypto/third_party/sha1/sha1.c

  ## Minecraft
  # mc/mcpkg_mc_versions.c
)

# win/mcpkg_win_compat.c
# ./str/mcpkg_str.h
# ./str/array_helper.h
# ./str/mcpkg_str_list.h

set(MCPKG_PUBLIC_HEADERS
  # API
  # api/mcpkg_install.h
  # api/mcpkg_get.h
  # api/mcpkg_activate.h
  # api/mcpkg_cache.h
  # api/mcpkg_config.h
  # api/mcpkg_api_client.h


  # Crypto
  crypto/mcpkg_crypto.h
  crypto/mcpkg_crypto_hash.h
  crypto/mcpkg_crypto_hex.h
  crypto/mcpkg_crypto_init.h
  # crypto/mcpkg_crypto_io.h
  crypto/mcpkg_crypto_provider_verify.h
  crypto/mcpkg_crypto_rand.h
  crypto/mcpkg_crypto_sign.h
  crypto/mcpkg_crypto_util.h
  crypto/mcpkg_sip_hash.h
  crypto/third_party/md5/md5sum.h
  crypto/third_party/sha1/sha1.h



  # Filesystem
  fs/mcpkg_fs_dir.h
  fs/mcpkg_fs_error.h
  fs/mcpkg_fs_file.h
  # fs/# mcpkg_fs.h
  fs/mcpkg_fs_util.h
  fs/mcpkg_fs_std_paths.h
  ## Providers
  # providers/modrith_client.h

  ## Bad Math
  math/mcpkg_math.h

  # Package meta-data
  # packages/mcpkg_entry.h
  # packages/mcpkg_msgpack.h
  # packages/mcpkg_visited_set.h
  # packages/mcpkg_info_entry.h
  # packages/mcpkg_deps_entry.h

  ## lists, maps, so on
  container/mcpkg_map.h
  container/mcpkg_hash.h
  container/mcpkg_container_util.h
  container/mcpkg_container_error.h
  container/mcpkg_list.h
  container/mcpkg_str_list.h


  ## Minecraft
  # mc/mcpkg_mc_providers.h
  # mc/mcpkg_mc_loaders.h
  # mc/mcpkg_mc_versions.h

  ## McPkg
  mcpkg.h
  mcpkg_export.h
)
# win/mcpkg_win_compat.h
set(MCPKG_PRIVATE_HEADERS
    # API
    # CONTAINER
    container/mcpkg_str_list_p.h
    container/mcpkg_map_p.h
    container/mcpkg_hash_p.h
    # FILESYSTEM
    # MINECRAFT
    # PACKAGES
    # PROVIDERS
    # WIN32
)
if (MCPKG_BUILD_SHARED)
    message("BUILDING SHARED")
    add_library(${TARGET_NAME}
        ${MCPKG_PRIVATE_HEADERS} ${MCPKG_PUBLIC_HEADERS}
        ${MCPKG_SOURCES}
    )
    if(NOT MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE -fvisibility=hidden)
    endif()
else()
    message("BUILDING STATIC")
    add_library(${TARGET_NAME} STATIC
        ${MCPKG_PRIVATE_HEADERS} ${MCPKG_PUBLIC_HEADERS}
        ${MCPKG_SOURCES}
    )
endif()

target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/container
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/fs
    ${CMAKE_CURRENT_SOURCE_DIR}/math
    ${CMAKE_CURRENT_SOURCE_DIR}/mc
    ${CMAKE_CURRENT_SOURCE_DIR}/packages
    ${CMAKE_CURRENT_SOURCE_DIR}/providers
)

target_link_libraries(${TARGET_NAME} PRIVATE ${BASE_LIBS})

set_property(TARGET ${TARGET_NAME} PROPERTY C_STANDARD 23)
set_property(TARGET ${TARGET_NAME} PROPERTY C_STANDARD_REQUIRED YES)

if(MCPKG_BUILD_SHARED)
  install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
else()
  install(TARGETS ${TARGET_NAME} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
# install(DIRECTORY ${MCPKG_PUBLIC_INCLUDE_DIR}/
#         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mcpkg
#         FILES_MATCHING PATTERN "*.h")



