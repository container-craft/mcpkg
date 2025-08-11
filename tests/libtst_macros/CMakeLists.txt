set(TARGET_NAME libtst_macros)
project(${TARGET_NAME} LANGUAGES C)

add_library(${TARGET_NAME} STATIC tst_macros.c tst_macros.h)

# expose default verbosity as a macro (1/0)
target_compile_definitions(${TARGET_NAME}
  PUBLIC TST_VERBOSE_DEFAULT=$<IF:$<BOOL:${TST_VERBOSE}>,1,0>)

set_property(TARGET ${TARGET_NAME} PROPERTY C_STANDARD 23)
set_property(TARGET ${TARGET_NAME} PROPERTY C_STANDARD_REQUIRED YES)
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
