set(TARGET_NAME tst_libmcpkg)
project(${TARGET_NAME} LANGUAGES C)

# templete -> tst_tmp.h
set(TST_LIBMCPKG_SOURCE
  main.c
  tst_containers.h
  tst_filesystem.h
  tst_crypto.h
  tst_pack.h
  tst_pkg_roundtrip.h
  tst_ledger_roundtrip.h
  tst_mc.h
  tst_net.h
)

add_executable(${TARGET_NAME} ${TST_LIBMCPKG_SOURCE})
target_link_libraries(${TARGET_NAME} PRIVATE
  libmcpkg
  libtst_macros
  ${BASE_LIBS}
)
