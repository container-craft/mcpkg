cmake_minimum_required(VERSION 3.16)
project(mcpkg-mpgen VERSION 0.1.0 LANGUAGES NONE)

# Where to place generated files (headers and sources)

## uncomment to get a bunch of debugging.
# set(ENV{MPGEN_DEBUG} 3)


# Python 3 is required to run the generator
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Collect schemas and templates
# ${CMAKE_CURRENT_SOURCE_DIR}/schemas/sig.yaml
set(MPGEN_SCHEMAS
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_devproof.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_devlink.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_delegate.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_devsig.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_attestation.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_reward.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_tx.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_revoke.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_sth.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_block.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_audit_node.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_audit_path.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_hash32.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/ledger/ledger_consistency.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/pkg/pkg_origin.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/pkg/pkg_meta.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/pkg/pkg_file.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/pkg/pkg_digest.yaml
 ${CMAKE_CURRENT_SOURCE_DIR}/schemas/pkg/pkg_depends.yaml
)
set(MPGEN_TEMPLATES
    ${CMAKE_CURRENT_SOURCE_DIR}/templates/c_header.jinja
    ${CMAKE_CURRENT_SOURCE_DIR}/templates/c_source.jinja
)

# Ensure output directory exists
file(MAKE_DIRECTORY "${MPGEN_OUT_DIR}")

# Single rule to run the generator for all schemas
add_custom_command(
    OUTPUT "${MPGEN_OUT_DIR}/.stamp"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${MPGEN_OUT_DIR}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/mpgen.py"
            --schemas ${MPGEN_SCHEMAS}
            --templates "${CMAKE_CURRENT_SOURCE_DIR}/templates"
            --out "${libmcpkg_SOURCE_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${MPGEN_OUT_DIR}/.stamp"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/mpgen.py"
        ${MPGEN_SCHEMAS}
        ${MPGEN_TEMPLATES}
    COMMENT "mpgen: generating MessagePack pack/unpack C code"
    VERBATIM
)

# Always run generation as part of the build
add_custom_target(mpgen_generate ALL
    DEPENDS "${MPGEN_OUT_DIR}/.stamp"
)

# Expose include path for generated headers (e.g., ${MPGEN_OUT_DIR}/pkg/*.h)
add_library(mcpkg_mpgen_headers INTERFACE)
add_dependencies(mcpkg_mpgen_headers mpgen_generate)
target_include_directories(mcpkg_mpgen_headers INTERFACE "${MPGEN_OUT_DIR}")
