add_subdirectory(network)
add_subdirectory(cut)
add_subdirectory(views)

add_library(ifpga_database INTERFACE)
target_link_libraries(ifpga_database INTERFACE ifpga_network ifpga_cut ifpga_views)
target_include_directories(ifpga_database INTERFACE ${CMAKE_SOURCE_DIR})