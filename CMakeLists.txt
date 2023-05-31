cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(ifpga LANGUAGES CXX)

############################################################
###        common settings
############################################################
message("STEP2: common settings")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(ENABLE_EXAMPLE "enable examples" ON)
option(ENABLE_TEST "enable test" ON)
option(ENABLE_GPROF "enable gprof tool for profiling" OFF)
option(ENABLE_DOXYGEN "enable doxygen tool for generate the docs" OFF)


# ###########################################################
# ##        config bebug/release mode
# ###########################################################
set(CMAKE_CXX_FLAGS "-Wall -Wno-unknown-pragmas -Werror=return-type")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

find_package(OpenMP REQUIRED)
if(OpenMP_FOUND)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
else()
    message(FATAL_ERROR "openmp not found!")
endif()

message("STEP3: config bebug/release mode")
if( CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug") )
    if(ENABLE_GPROF)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
        SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
    endif(ENABLE_GPROF)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
    message("Debug mode CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
elseif( CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Release") )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
    message("Release mode CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
    message("default Release mode CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
endif()

############################################################
###        sub directories
############################################################
message("STEP4: add subdirectories")
add_subdirectory(include)
add_subdirectory(libs)

if(ENABLE_TEST)
    add_subdirectory(test)
endif(ENABLE_TEST)

if(ENABLE_EXAMPLE)
    add_subdirectory(examples)
endif(ENABLE_EXAMPLE)

############################################################
###        EXECUTION
############################################################
message("STEP5: compile ifpga ing")
add_definitions( -D_PATH_FPGA=\"${CMAKE_CURRENT_SOURCE_DIR}\")
# include_directories(${YAML_CPP_INCLUDE_DIR})

add_executable( ifpga
    ${PROJECT_SOURCE_DIR}/ifpga.cpp
)
target_link_libraries( ifpga
    yaml-cpp
    ifpga_header
    args
)

############################################################
###        INSTALL
############################################################
message("STEP6: install ifpga ing")
message("install direcotry: ${CMAKE_INSTALL_PREFIX}/bin")
install(TARGETS ifpga
    RUNTIME DESTINATION bin
)

############################################################
###        Doxygen
############################################################
find_package(Doxygen)
if (ENABLE_DOXYGEN)
    message("STEP7: it will generate doxygen docs when making it!")
    if (DOXYGEN_FOUND)
        # set input and output files
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.cfg)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        # request to configure the file
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        # note the option ALL which allows to build the docs together with the application
        add_custom_target( docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
    # else (DOXYGEN_FOUND AND BUILD_DOXYGEN)
    else ()
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif(ENABLE_DOXYGEN)
