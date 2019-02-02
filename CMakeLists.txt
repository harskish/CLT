cmake_minimum_required(VERSION 3.0)
project(CLT VERSION 0.1.0 DESCRIPTION "CLT - OpenCL Toolkit")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Cannot mix cl.hpp and ch2.hpp
# Allow parent project to choose
option(CLT_USE_CL_1_HEADER "Use cl.hpp instead of cl2.hpp" OFF)
if (CLT_USE_CL_1_HEADER)
    # Use cl.hpp
    add_definitions(-DCLT_CL_1_HEADER)
    message("-- CLT: using legacy header (cl.hpp)")
else()
    # Use cl2.hpp in OCL 1.2 mode
    add_definitions(-DCL_HPP_MINIMUM_OPENCL_VERSION=120)
    add_definitions(-DCL_HPP_TARGET_OPENCL_VERSION=120)
endif()

# Version 1.2+ for getArgInfo
find_package(OpenCL 1.2 REQUIRED)
set(CLT_CL_INCLUDE_DIR ${OpenCL_INCLUDE_DIR} PARENT_SCOPE)

# OpenGL for CL-GL shared context
find_package(OpenGL)
if (OPENGL_FOUND)
	add_definitions(-DCLT_HAS_GL)
	list(APPEND LIBRARIES ${OPENGL_LIBRARIES})
endif()

add_library(CLT STATIC
    include/clt.hpp
	src/Kernel.cpp
	src/Kernel.hpp
	src/kernelreader.cpp
	src/kernelreader.hpp
	src/utils.cpp
	src/utils.hpp
	external/xxhash/xxhash.c
)

set(CLT_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include PARENT_SCOPE)
set_target_properties(CLT PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(CLT PROPERTIES PUBLIC_HEADER include/clt.hpp)

set(INCLUDE_DIRS
	src
	external
    ${OpenCL_INCLUDE_DIR}
)

list(APPEND LIBRARIES
	${OpenCL_LIBRARY}
)

include_directories(${INCLUDE_DIRS})
target_link_libraries(CLT ${LIBRARIES})
