set(NEUROTESSMESH_SOURCES
  # mainwindow.ui
  # resources.qrc
  neurotessmesh.cpp
  # MainWindow.cpp
  # OpenGLWidget.cpp
  # ColorSelectionWidget.cpp
  # CMakeSetup.rc
)

set(NEUROTESSMESH_HEADERS
  ${PROJECT_BINARY_DIR}/include/neurotessmesh/version.h
)

set(NEUROTESSMESH_MOC_HEADERS
  # MainWindow.h
  # OpenGLWidget.h
  # ColorSelectionWidget.h
  )

include_directories(${CMAKE_CURRENT_BINARY_DIR})


if ( NOT DEFAULT_CONTEXT_OPENGL_MAJOR )
  set( DEFAULT_CONTEXT_OPENGL_MAJOR 4 )
endif( )
if ( NOT DEFAULT_CONTEXT_OPENGL_MINOR )
  set( DEFAULT_CONTEXT_OPENGL_MINOR 0 )
endif( )

add_definitions( "-DDEFAULT_CONTEXT_OPENGL_MAJOR=${DEFAULT_CONTEXT_OPENGL_MAJOR}" )
add_definitions( "-DDEFAULT_CONTEXT_OPENGL_MINOR=${DEFAULT_CONTEXT_OPENGL_MINOR}" )
if ( MSVC )
  add_definitions( -DNEUROTESSMESH_STATIC )
endif( )

if ( APPLE )
  set( NEUROTESSMESH_ICON neurotessmesh.icns )
endif()

if ( UNIX AND NOT APPLE )
  set( NEUROTESSMESH_ICON neurotessmesh.png )
endif( )

common_application(NeuroTessMesh GUI ${COMMON_APP_ARGS})
