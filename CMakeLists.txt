# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#
#   NeuroTessMesh
#   2017 (c) Universidad Rey Juan Carlos
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(neurotessmesh VERSION 0.0.0)
set(neurotessmesh_VERSION_ABI 0)

# Disable in source building
if( "${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}" )
  message( FATAL "no in source building allowed." )
endif()

list( APPEND CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/CMake
  ${CMAKE_SOURCE_DIR}/CMake/common )

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/CMake/common/Common.cmake)
  message(FATAL_ERROR "CMake/common missing, run: git submodule update --init")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

option( NEUROTESSMESH_OPTIONALS_AS_REQUIRED
  "Force optional dependencies as required" OFF )

if ( NEUROTESSMESH_OPTIONALS_AS_REQUIRED )
  set( NEUROTESSMESH_OPTS_FIND_ARGS "REQUIRED" )
else()
  set( NEUROTESSMESH_OPTS_FIND_ARGS "" )
endif()

include(GitExternal)

#description
set( NEUROTESSMESH_DESCRIPTION "NeuroTessMesh")
set( NEUROTESSMESH_MAINTAINER "Juan Jose Garcia Cantero <juanjose.garcia@urjc.es>")
set( NEUROTESSMESH_LICENSE LGPL)
set( COMMON_PROJECT_DOMAIN gmrv.es)

include(Common)

common_find_package(Qt5Core 5.4 SYSTEM )
common_find_package(Qt5Widgets 5.4 SYSTEM )
common_find_package(Qt5OpenGL 5.4 SYSTEM )
common_find_package(Eigen3 REQUIRED SYSTEM)
common_find_package(GLEW REQUIRED SYSTEM)
common_find_package(nsol REQUIRED)
common_find_package(ReTo REQUIRED)
common_find_package(neurolots REQUIRED)

list(APPEND NEUROTESSMESH_DEPENDENT_LIBRARIES Eigen3 GLEW nsol ReTo neurolots)

common_find_package_post( )

add_subdirectory( neurotessmesh )

include(CPackConfig)
include(DoxygenRule)
