cmake_minimum_required(VERSION 3.16)
set(TARGET_NAME libmcpkg)
project(${TARGET_NAME}
    VERSION 0.1
    LANGUAGES C)

set(MCPKG_SOURCES
  ## Api
  # api/mcpkg_activate.c
  # api/mcpkg_cache.c
  # api/mcpkg_get.c
  # api/mcpkg_api_client.c

  ## Filesystem
  fs/mcpkg_fs_dir.c
  fs/mcpkg_fs_file.c
  fs/mcpkg_fs_util.c
  fs/mcpkg_fs_error.c
  fs/mcpkg_fs_std_paths.c

  ## NETWORKING
  net/mcpkg_net_util.c
  net/mcpkg_net_url.c
  net/mcpkg_net_client.c
  # net/modrith_client.c

  ## MessagePack
  mp/mcpkg_mp_util.c
  mp/mcpkg_mp_ledger_attestation.c
  mp/mcpkg_mp_ledger_audit_node.c
  mp/mcpkg_mp_ledger_audit_path.c
  mp/mcpkg_mp_ledger_block.c
  mp/mcpkg_mp_ledger_consistency.c
  mp/mcpkg_mp_ledger_delegate.c
  mp/mcpkg_mp_ledger_devlink.c
  mp/mcpkg_mp_ledger_devproof.c
  mp/mcpkg_mp_ledger_devsig.c
  mp/mcpkg_mp_ledger_hash32.c
  mp/mcpkg_mp_ledger_revoke.c
  mp/mcpkg_mp_ledger_reward.c
  mp/mcpkg_mp_ledger_sth.c
  mp/mcpkg_mp_ledger_tx.c

  mp/mcpkg_mp_pkg_digest.c
  mp/mcpkg_mp_pkg_depends.c
  mp/mcpkg_mp_pkg_file.c
  mp/mcpkg_mp_pkg_meta.c
  mp/mcpkg_mp_pkg_origin.c

  ## Lists
  container/mcpkg_list.c
  container/mcpkg_hash.c
  container/mcpkg_map.c
  container/mcpkg_str_list.c

  # Crypto
  crypto/mcpkg_crypto.c
  # crypto/mcpkg_crypto_ledger.c
  crypto/mcpkg_crypto_hash.c
  crypto/mcpkg_crypto_hex.c
  crypto/mcpkg_crypto_init.c
  crypto/mcpkg_crypto_provider_verify.c
  crypto/mcpkg_crypto_rand.c
  crypto/mcpkg_crypto_sign.c
  crypto/mcpkg_crypto_util.c
  crypto/mcpkg_sip_hash.c
  crypto/third_party/md5/md5sum.c
  crypto/third_party/sha1/sha1.c

  ## Minecraft
  mc/mcpkg_mc.c
  mc/mcpkg_mc_loaders.c
  mc/mcpkg_mc_loaders_msgpack.c
  mc/mcpkg_mc_providers.c
  mc/mcpkg_mc_util.c
  mc/mcpkg_mc_versions.c
  mc/mcpkg_mc_versions_msgpack.c
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
  # crypto/mcpkg_crypto_ledger.h
  crypto/mcpkg_crypto_hash.h
  crypto/mcpkg_crypto_hex.h
  crypto/mcpkg_crypto_init.h
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

  ## Networking.
  net/mcpkg_net_util.h
  net/mcpkg_net_url.h
  net/mcpkg_net_client.h

  ## Providers
  # providers/modrith_client.h

  ## Bad Math
  math/mcpkg_math.h

  # Message Packs(Generated minus the utils)
  mp/mcpkg_mp_util.h
  mp/mcpkg_mp_ledger_attestation.h
  mp/mcpkg_mp_ledger_audit_node.h
  mp/mcpkg_mp_ledger_audit_path.h
  mp/mcpkg_mp_ledger_block.h
  mp/mcpkg_mp_ledger_consistency.h
  mp/mcpkg_mp_ledger_delegate.h
  mp/mcpkg_mp_ledger_devlink.h
  mp/mcpkg_mp_ledger_devproof.h
  mp/mcpkg_mp_ledger_devsig.h
  mp/mcpkg_mp_ledger_hash32.h
  mp/mcpkg_mp_ledger_revoke.h
  mp/mcpkg_mp_ledger_reward.h
  mp/mcpkg_mp_ledger_sth.h
  mp/mcpkg_mp_ledger_tx.h
  mp/mcpkg_mp_pkg_depends.h
  mp/mcpkg_mp_pkg_digest.h
  mp/mcpkg_mp_pkg_file.h
  mp/mcpkg_mp_pkg_meta.h
  mp/mcpkg_mp_pkg_origin.h
  # mp/mcpkg_mp_sig_env.h

  # pack/mcpkg_visited_set.h

  ## Container
  container/mcpkg_map.h
  container/mcpkg_hash.h
  container/mcpkg_container_util.h
  container/mcpkg_container_error.h
  container/mcpkg_list.h
  container/mcpkg_str_list.h

  ## Minecraft
  mc/mcpkg_mc.h
  mc/mcpkg_mc_loaders.h
  mc/mcpkg_mc_loaders_msgpack.h
  mc/mcpkg_mc_providers.h
  mc/mcpkg_mc_util.h
  mc/mcpkg_mc_versions.h
  mc/mcpkg_mc_versions_msgpack.h

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

