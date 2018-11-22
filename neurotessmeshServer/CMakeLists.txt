set(Name neurotessmeshServer)
set(NAME NEUROTESSMESHSERVER)
set(namespace neurotessmeshServer)
set(NAMESPACE NEUROTESSMESHSERVER)
set(INCLUDE_NAME neurotessmeshServer)

set(PROJECT_VERSION_ABI ${${PROJECT_NAME}_VERSION_ABI})
configure_file(${CMAKE_SOURCE_DIR}/CMake/common/cpp/api.h
  ${PROJECT_BINARY_DIR}/include/neurotessmeshServer/api.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/CMake/common/cpp/version.h
  ${PROJECT_BINARY_DIR}/include/neurotessmeshServer/version.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/CMake/common/cpp/version.cpp
  ${PROJECT_BINARY_DIR}/src/neurotessmeshServer/version.cpp @ONLY)

set( NEUROTESSMESHSERVER_SOURCES
  ${PROJECT_BINARY_DIR}/src/neurotessmeshServer/version.cpp
  neurotessmeshServer.cpp
  )
set( NEUROTESSMESHSERVER_HEADERS
  ${PROJECT_BINARY_DIR}/include/neurotessmeshServer/version.h
  )

include_directories(
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_BINARY_DIR}/neurotessmesh
  ${PROJECT_SOURCE_DIR}
  )

set( NEUROTESSMESHSERVER_LINK_LIBRARIES
  ${OPENGL_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${GLUT_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  ${Boost_FILESYSTEM_LIBRARIES}
  nsol
  ReTo
  nlgeometry
  nlgenerator
  nlrender
  )

if ( NOT DEFAULT_CONTEXT_OPENGL_MAJOR )
  set( DEFAULT_CONTEXT_OPENGL_MAJOR 4 )
endif( )
if ( NOT DEFAULT_CONTEXT_OPENGL_MINOR )
  set( DEFAULT_CONTEXT_OPENGL_MINOR 0 )
endif( )

add_definitions( "-DDEFAULT_CONTEXT_OPENGL_MAJOR=${DEFAULT_CONTEXT_OPENGL_MAJOR}" )
add_definitions( "-DDEFAULT_CONTEXT_OPENGL_MINOR=${DEFAULT_CONTEXT_OPENGL_MINOR}" )

if (MSVC)
  add_definitions(-DNEUROTESSMESHSERVER_STATIC)
endif( )


common_application( neurotessmeshServer NOHELP )
