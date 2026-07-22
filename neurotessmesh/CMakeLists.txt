# Define headers
set(NEUROTESSMESH_HEADERS
  Scene.h
  LoaderThread.h
  SaveScreenshotDialog.h

  MainWindow.h
  OpenGLWidget.h
  ColorSelectionWidget.h
)

# Define source files
set(NEUROTESSMESH_SOURCES
  mainwindow.ui
  SaveScreenshotDialog.ui
  resources.qrc
  neurotessmesh.cpp
  MainWindow.cpp
  OpenGLWidget.cpp
  ColorSelectionWidget.cpp
  CMakeSetup.rc
  Scene.cpp
  LoaderThread.cpp
  SaveScreenshotDialog.cpp
)

# # Create the includes strings
# set(HEADER_INCLUDES "")
# foreach(header ${NEUROTESSMESH_PUBLIC_HEADERS})
#   string(APPEND HEADER_INCLUDES "#include <${PROJECT_NAME_LOWER}/${header}>\n")
# endforeach()

# # Generate the unified header
# configure_file(
#   "${PROJECT_SOURCE_DIR}/cmake/unified_header.h.in"
#   "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_LOWER}.h"
# )

# Get the current Git commit for the version header
find_package(Git)
if(Git_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} --no-pager log -n 1 HEAD "--pretty=format:%h"
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    RESULT_VARIABLE git_result
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(git_result EQUAL 0)
    message(STATUS "Last ${PROJECT_NAME} Git commit: ${GIT_COMMIT_HASH}")
  endif()
endif()

# Generate the version header
configure_file (
  "${PROJECT_SOURCE_DIR}/cmake/version.h.in"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWER}/version.h"
)

# Enable Qt generators
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Create the target
add_executable(${PROJECT_NAME})

# set_target_properties(${PROJECT_NAME} PROPERTIES
#   VERSION ${PROJECT_VERSION}
#   SOVERSION ${PROJECT_VERSION_MAJOR}
# )

# Add sources to the target
target_sources(${PROJECT_NAME}
  PRIVATE
    ${NEUROTESSMESH_SOURCES}
  PRIVATE
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS
        ${PROJECT_SOURCE_DIR}
      FILES
        ${NEUROTESSMESH_HEADERS}
)

# Add generated sources to the target
target_sources(${PROJECT_NAME}
  PRIVATE
    FILE_SET public_headers_generated
      TYPE HEADERS
      BASE_DIRS
        ${PROJECT_BINARY_DIR}
      FILES
        # Generated headers
        # ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_LOWER}.h
        # ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_LOWER}_export.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

# Add compile definitions 
target_compile_definitions(${PROJECT_NAME}
  PUBLIC
  PRIVATE
    DEFAULT_CONTEXT_OPENGL_MAJOR=${DEFAULT_CONTEXT_OPENGL_MAJOR}
    DEFAULT_CONTEXT_OPENGL_MINOR=${DEFAULT_CONTEXT_OPENGL_MINOR}
    # $<$<BOOL:${NEUROTESSMESH_USE_BRION}>:NEUROTESSMESH_USE_BRION>
    $<$<BOOL:${NEUROTESSMESH_USE_SIMIL}>:NEUROTESSMESH_USE_SIMIL>
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Qt5::Core
    Qt5::Widgets
    Qt5::OpenGL
    GLEW::GLEW
    # OpenGL::GL
    # Eigen3::Eigen
    neurolots::nlgeometry
    neurolots::nlrender
    neurolots::nlgenerator
    nsol::nsol
    ReTo::ReTo
    "$<$<BOOL:${NEUROTESSMESH_USE_SIMIL}>:SimIL::SimIL>"
    "$<$<BOOL:${NEUROTESSMESH_USE_SIMIL}>:SimIL::QSimIL>"
    acuterecorder::acuterecorder
  PRIVATE
    Boost::filesystem
)

# Define installation directory variables
include(GNUInstallDirs)

# Create an export
install(TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
