# Copyright (C) 2018. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero Licence (see in file LICENCE).

cmake_minimum_required(VERSION 2.8.10)
project (SimGrid-FMI CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/tools/cmake/")

# Search for SimGrid
set(SimGrid_PATH "/home/mecsyco/Documents/simgrid")
#find_package(SimGrid REQUIRED)
set(SimGrid_INCLUDE_DIR "${SimGrid_PATH}/include")
set(SimGrid_LIBRARY "${SimGrid_PATH}/lib/libsimgrid.so")

include_directories("${SimGrid_INCLUDE_DIR}" SYSTEM)

include_directories(include)
include_directories(deps/fmipp/common/fmi_v1.0)
include_directories(deps/fmipp/common/fmi_v2.0)
include_directories(deps/fmipp/common)
include_directories(deps/fmipp/import/base/include)
include_directories(deps/fmipp)
enable_testing()

# Build the library
add_library(simgrid-fmi SHARED src/fmi_model.cpp)
find_library(fmilibpath NAMES libfmippim.so ${CMAKE_CURRENT_BINARY_DIR}/deps/fmipp/build/import)
add_library(fmilib SHARED IMPORTED)
set_property(TARGET fmilib PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/deps/fmipp/build/import/libfmippim.so")
target_link_libraries(simgrid-fmi fmilib ${SimGrid_LIBRARY})

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

foreach (example thermal-cloud)
  add_executable (s4u-${example}  examples/${example}/s4u-${example}.cpp)
  target_link_libraries(s4u-${example}  simgrid-fmi)
  set_target_properties(s4u-${example}  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/examples/${example})
endforeach()



