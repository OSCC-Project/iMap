add_library(ifpga_cut INTERFACE)
target_link_libraries(ifpga_cut INTERFACE ifpga_utils )
target_include_directories(ifpga_cut INTERFACE ${PROJECT_SOURCE_DIR}/include/database)