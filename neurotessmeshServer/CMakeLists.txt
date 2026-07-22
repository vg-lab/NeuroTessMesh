# Set the target name
set(CURRENT_TARGET_NAME neurotessmeshServer)
string(TOLOWER ${CURRENT_TARGET_NAME} CURRENT_TARGET_NAME_LOWER)
string(TOUPPER ${CURRENT_TARGET_NAME} CURRENT_TARGET_NAME_UPPER)

# Define headers
set(NEUROTESSMESHSERVER_HEADERS
)

# Define source files
set(NEUROTESSMESHSERVER_SOURCES
  neurotessmeshServer.cpp
)

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
    message(STATUS "Last ${CURRENT_TARGET_NAME} Git commit: ${GIT_COMMIT_HASH}")
  endif()
endif()

# Generate the version header
configure_file(
  "${PROJECT_SOURCE_DIR}/cmake/version-component.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

# Create the target
add_executable(${CURRENT_TARGET_NAME})

# set_target_properties(${CURRENT_TARGET_NAME} PROPERTIES
#   VERSION ${PROJECT_VERSION}
#   SOVERSION ${PROJECT_VERSION_MAJOR}
# )

# Add sources to the target
target_sources(${CURRENT_TARGET_NAME}
  PRIVATE
    ${NEUROTESSMESHSERVER_SOURCES}
  PRIVATE
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS
        ${PROJECT_SOURCE_DIR}
      FILES
        ${NEUROTESSMESHSERVER_HEADERS}
)

# Add generated sources to the target
target_sources(${CURRENT_TARGET_NAME}
  PRIVATE
    FILE_SET public_headers_generated
      TYPE HEADERS
      BASE_DIRS
        ${PROJECT_BINARY_DIR}
      FILES
        # Generated headers
        # ${CMAKE_CURRENT_BINARY_DIR}/${CURRENT_TARGET_NAME_LOWER}.h
        # ${CMAKE_CURRENT_BINARY_DIR}/${CURRENT_TARGET_NAME_LOWER}_export.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

# Add compile definitions 
target_compile_definitions(${CURRENT_TARGET_NAME}
  PUBLIC
  PRIVATE
)

# Link libraries
target_link_libraries(${CURRENT_TARGET_NAME}
  PUBLIC
    GLEW::GLEW
    # OpenGL::GL
    neurolots::nlgeometry
    neurolots::nlrender
    neurolots::nlgenerator
    nsol::nsol
    ReTo::ReTo
  PRIVATE
    Boost::filesystem
)

# Define installation directory variables
include(GNUInstallDirs)

# Create an export
install(TARGETS ${CURRENT_TARGET_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
