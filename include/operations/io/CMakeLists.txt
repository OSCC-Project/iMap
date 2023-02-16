add_library(ifpga_io INTERFACE)
target_link_libraries(ifpga_io INTERFACE ifpga_network ifpga_utils lorina)
target_include_directories(ifpga_io INTERFACE ${PROJECT_SOURCE_DIR}/include/operations)