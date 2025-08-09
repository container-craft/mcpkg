find_package(JNI REQUIRED)

add_library(mcpkg_jni SHARED
  mcpkg_jni.c   # your JNI glue
)

target_include_directories(mcpkg_jni
  PRIVATE
    ${JNI_INCLUDE_DIRS}
    $<TARGET_PROPERTY:libmcpkg,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(mcpkg_jni
  PRIVATE
    libmcpkg
)
