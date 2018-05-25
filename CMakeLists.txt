# Copyright (C) 2018. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero Licence (see in file LICENCE).

cmake_minimum_required(VERSION 2.8.10)
project (SimGrid-FMI CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/tools/cmake/")

# Search for SimGrid
find_package(SimGrid REQUIRED)
include_directories("${SimGrid_INCLUDE_DIR}" SYSTEM)

include_directories(include)
include_directories(deps/fmipp/common/fmi_v1.0)
include_directories(deps/fmipp/import/utility/include)
include_directories(deps/fmipp/import/integrators/include)
include_directories(deps/fmipp)
enable_testing()

# Build the library
add_library(simgrid-fmi SHARED src/fmi_model.cpp)

# Enable Testing
# include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Tests.cmake)
# add_subdirectory(${PROJECT_SOURCE_DIR}/examples)

# Install everything
install(TARGETS simgrid-fmi DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/)
foreach(file include/simgrid-fmi.hpp)
  get_filename_component(location ${file} PATH)
    string(REPLACE "${CMAKE_CURRENT_BINARY_DIR}/" "" location "${location}")
  install(FILES ${file} DESTINATION $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${location})
endforeach()


