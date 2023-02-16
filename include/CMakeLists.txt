add_subdirectory(utils)
add_subdirectory(database)
add_subdirectory(operations)
add_subdirectory(flow)


add_library(ifpga_header INTERFACE)
target_link_libraries(ifpga_header INTERFACE ifpga_database ifpga_operations ifpga_flow ifpga_utils)
target_include_directories(ifpga_header INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})