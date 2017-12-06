 ########## RETO ##########
  include( reto_generate_shaders )

  reto_generate_shaders(
    REMOTERENDER_SHADERS
    remoteRender
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/
    ${CMAKE_CURRENT_SOURCE_DIR}/Shaders.h
    )
  message( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR})
  ########## RETO ##########

  set( REMOTERENDERER_SOURCES
  remoteRenderer.cpp
  LookAtCamera.cpp
  RemoteRender.cpp
  )

set( REMOTERENDERER_HEADERS
  LookAtCamera.h
  RemoteRender.h
  Shaders.h
  )

set( REMOTERENDERER_LINK_LIBRARIES
  Rockets
  nlrender
  nlgeometry
  nlgenerator
  nsol
  ${LibJpegTurbo_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${GLUT_LIBRARIES}
  )


common_application( remoteRenderer )
