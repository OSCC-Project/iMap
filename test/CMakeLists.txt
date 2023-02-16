
add_library(catch2 INTERFACE)
target_include_directories(catch2 INTERFACE ${PROJECT_SOURCE_DIR}/include)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories( ${PROJECT_SOURCE_DIR}/include/ )

############################################################
###        TEST class
############################################################
enable_testing()

add_executable( test_cut
    ${PROJECT_SOURCE_DIR}/test/test_cut.cpp
)
target_link_libraries( test_cut
    ifpga_cut
)
add_test(NAME test_cut COMMAND test_cut)
add_custom_command(
    TARGET test_cut
    COMMENT "unit_test_cut"
    POST_BUILD
    COMMAND test_cut
    DEPENDS test_cut
)
add_executable( test_cutset
    ${PROJECT_SOURCE_DIR}/test/test_cutset.cpp
)
target_link_libraries( test_cutset
    ifpga_cut
)
add_test(NAME test_cutset COMMAND test_cutset)
add_custom_command(
    TARGET test_cutset
    COMMENT "unit_test_cutset"
    POST_BUILD
    COMMAND test_cutset
    DEPENDS test_cutset
)

 add_executable( test_aig_with_choice
    ${PROJECT_SOURCE_DIR}/test/test_aig_with_choice.cpp )
 target_link_libraries(test_aig_with_choice PRIVATE catch2 ifpga_algorithms)
add_test(NAME test_aig_with_choice COMMAND test_aig_with_choice)
add_custom_command(
   TARGET test_aig_with_choice
   COMMENT "utest_aig_with_choice"
   POST_BUILD
   COMMAND test_aig_with_choice
   DEPENDS test_aig_with_choice
)

 add_executable( test_choice_computation
    ${PROJECT_SOURCE_DIR}/test/test_choice_computation.cpp )
 target_link_libraries(test_choice_computation PRIVATE catch2 ifpga_algorithms)
add_test(NAME test_choice_computation COMMAND test_choice_computation)
add_custom_command(
   TARGET test_choice_computation
   COMMENT "utest_choice_computation"
   POST_BUILD
   COMMAND test_choice_computation
   DEPENDS test_choice_computation
)

 add_executable( test_and_balance
    ${PROJECT_SOURCE_DIR}/test/test_and_balance.cpp )
 target_link_libraries(test_and_balance PRIVATE catch2 ifpga_algorithms)
add_test(NAME test_and_balance COMMAND test_and_balance)
add_custom_command(
   TARGET test_and_balance
   COMMENT "utest_and_balannce"
   POST_BUILD
   COMMAND test_and_balance
   DEPENDS test_and_balance
)

add_executable( test_refactor
${PROJECT_SOURCE_DIR}/test/test_refactor.cpp )
target_link_libraries(test_refactor PRIVATE catch2 ifpga_algorithms)
add_test(NAME test_refactor COMMAND test_refactor)
add_custom_command(
  TARGET test_refactor
  COMMENT "utest_refactor"
  POST_BUILD
  COMMAND test_refactor
  DEPENDS test_refactor
)

# subgraph database
add_executable( test_subgraph_to_network
    ${PROJECT_SOURCE_DIR}/test/test_subgraph_to_network.cpp )
target_link_libraries(test_subgraph_to_network PRIVATE catch2 ifpga_optimization ifpga_algorithms)
add_test(NAME test_subgraph_to_network COMMAND test_subgraph_to_network)
add_custom_command(
    TARGET test_subgraph_to_network
    COMMENT "utest_subgraph_to_network"
    POST_BUILD
    COMMAND test_subgraph_to_network
    DEPENDS test_subgraph_to_network
)

add_executable( test_choice_miter
    ${PROJECT_SOURCE_DIR}/test/test_choice_miter.cpp )
target_link_libraries(test_choice_miter PRIVATE catch2 ifpga_algorithms)
add_test(NAME test_choice_miter COMMAND test_choice_miter)
add_custom_command(
    TARGET test_choice_miter
    COMMENT "utest_choice_miter"
    POST_BUILD
    COMMAND test_choice_miter
    DEPENDS test_choice_miter
)

# add_executable( test_cut_enumeration
#    ${PROJECT_SOURCE_DIR}/test/test_cut_enumeration.cpp )
# target_link_libraries(test_cut_enumeration PRIVATE catch2 ifpga_cut ifpga_network ifpga_algorithms )
# add_test(NAME test_cut_enumeration COMMAND test_cut_enumeration)
# add_custom_command(
#    TARGET test_cut_enumeration
#    COMMENT "utest_cut_enumeration"
#    POST_BUILD
#    COMMAND test_cut_enumeration
#    DEPENDS test_cut_enumeration
# )

add_executable( test_aig
    ${PROJECT_SOURCE_DIR}/test/test_aig.cpp
)
target_link_libraries(test_aig PRIVATE catch2 ifpga_network)


add_executable( test_klut_network
    ${PROJECT_SOURCE_DIR}/test/test_klut_network.cpp
)
target_link_libraries(test_klut_network PRIVATE catch2 ifpga_network )

add_executable( test_network_to_klut
    ${PROJECT_SOURCE_DIR}/test/test_network_to_klut.cpp
)
target_link_libraries(test_network_to_klut PRIVATE catch2 ifpga_algorithms ifpga_network kitty)
