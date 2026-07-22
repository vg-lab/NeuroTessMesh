cmake_minimum_required(VERSION 3.25)

project(NeuroTessMesh
  VERSION 0.5.6
  LANGUAGES CXX
  DESCRIPTION "NeuroTessMesh"
)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)

# Prevent in-source builds
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. Please create a separate build directory.")
endif()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# Include custom modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Options
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_DOCS "Build documentation" ON)
# option(NEUROTESSMESH_WITH_EXAMPLES "Build examples" ON)
# option(NEUROTESSMESH_WITH_TESTS "Build tests" ON)
option(NEUROTESSMESH_OPTIONALS_AS_REQUIRED "Force optional dependencies as required" OFF)

# Dependencies
find_package(Boost CONFIG COMPONENTS filesystem REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Eigen3 REQUIRED NO_MODULE)
# find_package(OpenGL REQUIRED)

include(FetchContent)
FetchContent_Declare(
    neurolots
    GIT_REPOSITORY https://github.com/vg-lab/neurolots.git
    GIT_SHALLOW ON
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(neurolots)

FetchContent_Declare(
    nsol
    GIT_REPOSITORY https://github.com/vg-lab/nsol.git
    GIT_SHALLOW ON
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(nsol)

FetchContent_Declare(
    ReTo
    GIT_REPOSITORY https://github.com/vg-lab/ReTo.git
    GIT_SHALLOW ON
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(ReTo)

FetchContent_Declare(
    SimIL
    GIT_REPOSITORY https://github.com/vg-lab/SimIL.git
    GIT_SHALLOW ON
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(SimIL)

FetchContent_Declare(
    AcuteRecorder
    GIT_REPOSITORY https://github.com/vg-lab/AcuteRecorder.git
    GIT_SHALLOW ON
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(AcuteRecorder)

# Optional dependencies
set(NEUROTESSMESH_OPTS_FIND_ARGS "")
if(NEUROTESSMESH_OPTIONALS_AS_REQUIRED)
  set(NEUROTESSMESH_OPTS_FIND_ARGS REQUIRED)
endif()

# find_package(Foo ${NEUROTESSMESH_OPTS_FIND_ARGS} QUIET)

# Definitions
option(NEUROTESSMESH_USE_SIMIL "" ON)
# NEUROTESSMESH_USE_SIMIL
# NEUROTESSMESH_USE_ZEROEQ
# NEUROTESSMESH_USE_LEXIS
# NEUROTESSMESH_USE_GMRVLEX

set(DEFAULT_CONTEXT_OPENGL_MAJOR 4)
set(DEFAULT_CONTEXT_OPENGL_MINOR 1)

# Add subdirectories
add_subdirectory(${PROJECT_NAME_LOWER})

add_subdirectory(neurotessmeshServer)

# Define installation directory variables
include(GNUInstallDirs)

# # Create an export
# install(TARGETS ${PROJECT_NAME}
#   EXPORT ${PROJECT_NAME}Targets
#   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#   # INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#   # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME_LOWER}
#   # FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#   # FILE_SET public_headers_generated DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )

# # Install the project targets file
# install(EXPORT ${PROJECT_NAME}Targets
#   FILE ${PROJECT_NAME_LOWER}-targets.cmake
#   NAMESPACE ${PROJECT_NAME}::
#   DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME_LOWER}
# )

# # Include the CMake module that provides helpers for creating package config files
# include(CMakePackageConfigHelpers)

# # Generate the project-specific cmake config file
# configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/project-config.cmake.in
#   "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_LOWER}-config.cmake"
#   INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME_LOWER}
# )

# # Generate the project-specific cmake config version file
# write_basic_package_version_file(
#   "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_LOWER}-config-version.cmake"
#   VERSION ${PROJECT_VERSION}
#   COMPATIBILITY SameMajorVersion
# )

# # Install the cmake config files
# install(FILES
#   "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_LOWER}-config.cmake"
#   "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_LOWER}-config-version.cmake"
#   DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME_LOWER}
# )

# # Install repo data
# install(FILES
#   "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
#   "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt"
#   DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME_LOWER}
# )

# # Allow the configured project-config.cmake in the build directory to be used by
# # other projects, without needing it to be installed
# export(EXPORT ${PROJECT_NAME}Targets
#   NAMESPACE ${PROJECT_NAME}::
#   FILE ${PROJECT_NAME_LOWER}-targets.cmake
# )

# Generate documentation
find_package(Doxygen)
if(DOXYGEN_FOUND AND BUILD_DOCS)
  set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs_doxygen)
  set(DOXYFILE_IN ${PROJECT_SOURCE_DIR}/cmake/Doxyfile.in)
  set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

  doxygen_add_docs(
    ${PROJECT_NAME}_doxygen
    CONFIG_FILE ${DOXYFILE_OUT}
  )

  install(DIRECTORY
    ${DOXYGEN_OUTPUT_DIR}/html
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    COMPONENT ${PROJECT_NAME}_documentation
    OPTIONAL
  )
endif()

# Packaging
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  /.git
  /.cache
  /build
  /.DS_Store
  /.idea
  /.vscode
)
include(CPack)
