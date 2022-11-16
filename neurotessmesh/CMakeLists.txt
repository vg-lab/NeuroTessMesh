set(Name NeuroTessMesh)
set(NAME NEUROTESSMESH)
set(namespace neurotessmesh)
set(NAMESPACE NEUROTESSMESH)
set(INCLUDE_NAME neurotessmesh)

set(PROJECT_VERSION_ABI ${${PROJECT_NAME}_VERSION_ABI})
configure_file(${CMAKE_SOURCE_DIR}/CMake/common/cpp/api.h
  ${PROJECT_BINARY_DIR}/include/neurotessmesh/api.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/CMake/common/cpp/version.h
  ${PROJECT_BINARY_DIR}/include/neurotessmesh/version.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/CMake/common/cpp/version.cpp
  ${PROJECT_BINARY_DIR}/src/neurotessmesh/version.cpp @ONLY)

set( NEUROTESSMESH_SOURCES
  ${PROJECT_BINARY_DIR}/src/neurotessmesh/version.cpp
  mainwindow.ui
  resources.qrc
  neurotessmesh.cpp
  MainWindow.cpp
  OpenGLWidget.cpp
  ColorSelectionWidget.cpp
  CMakeSetup.rc
  Scene.cpp
  LoaderThread.cpp
  )

set( NEUROTESSMESH_HEADERS
  ${PROJECT_BINARY_DIR}/include/neurotessmesh/version.h
  Scene.h
  LoaderThread.h
  )

set(NEUROTESSMESH_MOC_HEADERS
  MainWindow.h
  OpenGLWidget.h
  ColorSelectionWidget.h
  )

set( NEUROTESSMESH_LINK_LIBRARIES
  ${OPENGL_LIBRARIES}
  ${GLEW_LIBRARIES}
  Qt5::Core
  Qt5::Widgets
  Qt5::OpenGL
  nsol
  ReTo
  nlgeometry
  nlgenerator
  nlrender
  acuterecorder
  )
  
if (NEUROTESSMESH_OPTIONALS_AS_REQUIRED)
  list( APPEND NEUROTESSMESH_LINK_LIBRARIES SimIL QSimIL Brion Brain)
endif()

if ( ZEROEQ_FOUND )
  list( APPEND NEUROTESSMESH_LINK_LIBRARIES ZeroEQ )
  if ( LEXIS_FOUND )
    list( APPEND NEUROTESSMESH_LINK_LIBRARIES Lexis )
  endif( )
  if ( GMRVLEX_FOUND )
    list( APPEND NEUROTESSMESH_LINK_LIBRARIES gmrvlex )
  endif( )
endif( )

include_directories(
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_BINARY_DIR}/neurotessmesh
  ${PROJECT_SOURCE_DIR}
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
  add_definitions(-DNEUROTESSMESH_STATIC)
endif( )

if ( APPLE )
  set( NEUROTESSMESH_ICON neurotessmesh.icns )
endif()

if ( UNIX AND NOT APPLE )
  set( NEUROTESSMESH_ICON rsc/neurotessmesh.png )
endif( )


common_application( NeuroTessMesh GUI ${COMMON_APP_ARGS} )
