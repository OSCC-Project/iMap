include_directories(commands)

add_executable(imap    
    ${PROJECT_SOURCE_DIR}/cli/imap.cpp
)
target_link_libraries(imap
    ifpga_header
    alice
)

# function(add_alice_python_module target_name)
#   cmake_parse_arguments(ARG "" "" "" ${ARGN})
#   add_library("${target_name}_python" MODULE ${ARG_UNPARSED_ARGUMENTS})
#   target_link_libraries("${target_name}_python" PRIVATE alice_python)
#   set_target_properties("${target_name}_python" PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}" OUTPUT_NAME ${target_name} SUFFIX "${PYTHON_MODULE_EXTENSION}")
# endfunction()


# add_alice_python_module(imap 
#     ${PROJECT_SOURCE_DIR}/cli/imap.cpp
# )