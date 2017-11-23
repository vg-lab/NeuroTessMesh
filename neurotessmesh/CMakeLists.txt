set( NEUROTESSMESH_SOURCES
  neurotessmesh.cpp
  )

set( NEUROTESSMESH_HEADERS

  )

set( NEUROTESSMESH_LINK_LIBRARIES
  nsol
  )

include_directories(
  ${PROJECT_BINARY_DIR}/neurotessmesh
  ${PROJECT_SOURCE_DIR}
  )

if (MSVC)
  add_definitions(-DNEUROTESSMES_STATIC)
  if (WITH_NO_CONSOLE)
    message(STATUS "No console, WIN32 added to common app args")
    set(COMMON_APP_ARGS WIN32)
  else()
    message(STATUS "neurotessmes with console")
  endif()
endif( )

common_application( NeuroTessMesh )
