cmake_minimum_required(VERSION 3.0)
project(Example1)

# OpenCL
find_package(OpenCL 1.2 REQUIRED)

# CLT library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/clt)

# Copy kernel to binary directory
configure_file(device.cl ${CMAKE_CURRENT_BINARY_DIR}/device.cl COPYONLY)

set(INCLUDE_DIRS
    ../include
    ../external
    ${OpenCL_INCLUDE_DIR}
)

set(SOURCE_FILES
    main.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/device.cl
)

set(LIBRARIES
    CLT
    ${OpenCL_LIBRARY}
)

include_directories(${INCLUDE_DIRS})
add_executable(Example1 ${SOURCE_FILES})
target_link_libraries(Example1 ${LIBRARIES})