set(TARGET_NAME tst_libmcpkg)
project(${TARGET_NAME} LANGUAGES C)

enable_testing()

# if (TST_VERBOSE)
#   message("TESTS ARE VERBOSE")
#   set(ENV{} "1")
# endif()
# if(TST_ONLINE)
#   message("ONLINE TESTS")
#   set(ENV{MCPKG_TEST_ONLINE} "1")
# endif()


set(ENV{MY_VARIABLE} "my_value")
message("MY_VARIABLE is: $ENV{MCPKG_TST_VERBOSE}")

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
  tst_net_downloader.h
  tst_threads_basic.h
  tst_threads_concurrent.h
)

add_executable(${TARGET_NAME} ${TST_LIBMCPKG_SOURCE})
target_link_libraries(${TARGET_NAME} PRIVATE
  libmcpkg
  libtst_macros
  ${BASE_LIBS}
)
target_compile_definitions(${TARGET_NAME} PRIVATE
        TST_VERBOSE=$<IF:$<BOOL:${TST_VERBOSE}>,1,0>
        TST_ONLINE=$<IF:$<BOOL:${TST_ONLINE}>,1,0>)




add_test(NAME ${TARGET_NAME} COMMAND $<TARGET_FILE:${TARGET_NAME}>)

