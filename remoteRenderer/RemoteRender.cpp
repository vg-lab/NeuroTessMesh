/**
 * Copyright (c) 2015-2017 GMRV/URJC.
 *
 * Authors: Juan Jose Garcia Cantero <juanjose.garcia@urjc.es>
 *
 * This file is part of neurolots <https://github.com/gmrvvis/neurolots>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#include "RemoteRender.h"

#ifdef Darwin
  #define __gl_h_
  #define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
#endif

#include <reto/reto.h>
#include "Shaders.h"

#include <iostream>

using namespace rockets::http;

RemoteRender::RemoteRender( void )
{
  _server = new rockets::Server( ":8080", "rockets" );
  std::cout << "Server listening at: " << _server->getURI( ) << std::endl;
  _camera = new LookAtCamera( );
  _version = new nlrender::Version( );
  _image = new lexis::render::ImageJPEG( );

  _init( );

  _render = new nlrender::Renderer( );
  _render->tessCriteria( ) = nlrender::Renderer::LINEAR;
  _render->lod( ) = 1.0f;
  _render->maximumDistance( ) = 2000.0f;
  _server->handle(Method::GET, "v1/version/schema",
                     [&](const Request&) {
                       return make_ready_response(
                         Code::OK, _version->getSchema( ),
                         "application/json");
                     });
  _server->handle(Method::GET, "v1/version",
                     [&](const Request&) {
                       return make_ready_response(
                         Code::OK, _version->toJSON( ), "application/json" );
                     });
  _server->handle(Method::GET, "v1/camera/schema",
                     [&](const Request&) {
                       return make_ready_response(
                         Code::OK, _camera->jsonSchema( ),
                         "application/json" );
                     });
  _server->handle(Method::GET, "v1/camera",
                     [&](const Request&) {
                       return make_ready_response(
                         Code::OK, _camera->toJson( ), "application/json" );
                     });
  _server->handle( Method::PUT, "v1/camera",
                      [&](const Request& request) {
                        _camera->fromJson( request.body );
                        _renderToImage( );
                        return make_ready_response(
                          Code::OK );
                     });
  _server->handle( Method::GET, "v1/image-jpeg/schema",
                      [&](const Request&) {
                       return make_ready_response(
                         Code::OK, _image->getSchema( ), "application/json" );
                     });
  _server->handle( Method::GET, "v1/image-jpeg",
                      [&](const Request&) {
                        return make_ready_response(
                          Code::OK, _image->toJSON( ), "application/json" );
                      });

}

RemoteRender::~RemoteRender( void )
{

}

void RemoteRender::loadMorphologies( void )
{
  nlgeometry::MeshPtr mesh;
  nlgeometry::AttribsFormat format( 3 );
  format[0] = nlgeometry::TAttribType::POSITION;
  format[1] = nlgeometry::TAttribType::CENTER;
  format[2] = nlgeometry::TAttribType::TANGENT;

  // nsol::SwcReader swcr;
  // auto morphology = swcr.readMorphology( "/home/jgarcia/data/a_s.swc" );
  nsol::VasculatureReader vr;
  auto morphology = vr.loadMorphology( "/home/jgarcia/data/raw_copy.h5" );
  nsol::Simplifier::Instance( )->simplify(
        morphology, nsol::Simplifier::DIST_NODES_RADIUS );
  mesh = nlgenerator::MeshGenerator::generateMesh( morphology );

  mesh->uploadGPU( format, nlgeometry::Facet::PATCHES );
  mesh->computeBoundingBox( );

  _camera->origin( mesh->aaBoundingBox( ).center( ) +
                   Eigen::Vector3f( 0.0f, 0.0f,
                                    mesh->aaBoundingBox( ).radius( ) /
                                    sin( _camera->fieldOfView( ))));
  _camera->lookAt( mesh->aaBoundingBox( ).center( ));
  _meshes.push_back( mesh );
  _models.push_back( Eigen::Matrix4f::Identity( ));
}

void RemoteRender::run( void )
{
  loadMorphologies( );
  _renderToImage( );
  while( true )
  {
    _server->process( 1 );
    // std::this_thread::sleep_for( std::chrono::milliseconds(1));

  }
}

void RemoteRender::_init( )
{
  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
  glEnable( GL_DEPTH_TEST );
  glEnable( GL_CULL_FACE );
  glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

  _texture = 0;
  glGenTextures( 1, &_texture );
  glBindTexture( GL_TEXTURE_2D, _texture );

  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, _size.width, _size.height, 0,
                GL_RGB, GL_UNSIGNED_BYTE, 0 );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glActiveTexture(GL_TEXTURE0);

  _fbo = 0;
  glGenFramebuffers( 1, &_fbo );
  glBindFramebuffer( GL_FRAMEBUFFER, _fbo );
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
   GL_TEXTURE_2D, _texture, 0 );

  unsigned int depthBuffer = 0;
  glGenRenderbuffers( 1, &depthBuffer );
  glBindRenderbuffer( GL_RENDERBUFFER, depthBuffer );
  glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _size.width,
                         _size.height );
  glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                             GL_RENDERBUFFER, depthBuffer );

  glBindFramebuffer( GL_FRAMEBUFFER, _fbo );
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                          _texture, 0 );

  glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void RemoteRender::_renderToImage( void )
{
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo );
  glViewport( 0, 0, _size.width, _size.height );

  glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  _render->projectionMatrix( ) = _camera->projectionMatrix( );
  _render->viewMatrix( ) = _camera->viewMatrix( );
  _render->render( _meshes, _models, Eigen::Vector3f( 0.8, 0.3, 0.3 ));

  glFlush( );

  size_t size = _size.width * _size.height * 3;
  uint8_t* data  = (uint8_t*)malloc( size * sizeof( uint8_t ));
  glBindTexture(GL_TEXTURE_2D, _texture );
  glGetTexImage( GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, data );
  ImageJPEG imageJpeg = _encodeJpeg( _size.width, _size.height, data );
  _image->setData( imageJpeg.data, imageJpeg.size );
  _server->broadcastBinary((const char*)imageJpeg.data, imageJpeg.size );

  //////////////////////////
  // RENDER TEXTURE
  //////////////////////////

  // reto::ShaderProgram program;
  // program.loadVertexShaderFromText( remoteRender::example_vert );
  // program.loadFragmentShaderFromText( remoteRender::example_frag );
  // program.compileAndLink( );
  // program.autocatching( );
	// GLuint texID = glGetUniformLocation(program.program( ), "renderedTexture");

  // static const float quadBufferData[ ] = {
  //   -1.0f, 1.0f,
  //   -1.0f, -1.0f,
  //   1.0f,  -1.0f,
  //   1.0f,  1.0f,
  // };

  // static const unsigned int quadIndices[ ] =
  // {
  //   0, 1, 2,
  //   0, 2, 3,
  // };

  // unsigned int vao;
  // glGenVertexArrays( 1, &vao );
  // glBindVertexArray( vao );

  // unsigned int vbo[2];
  // glGenBuffers( 2, vbo );
  // glBindBuffer( GL_ARRAY_BUFFER, vbo[0]);
  // glBufferData( GL_ARRAY_BUFFER, sizeof( quadBufferData ), quadBufferData,
  //               GL_STATIC_DRAW );
  // glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, 0 );
  // glEnableVertexAttribArray( 0 );
  // glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbo[1] );
  // glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( quadIndices ), quadIndices,
  //               GL_STATIC_DRAW );


  // glActiveTexture(GL_TEXTURE0);
  // glBindTexture(GL_TEXTURE_2D, _texture );
  // glUniform1i( texID, 0);

  // glEnable( GL_DEPTH_TEST );

  // glBindFramebuffer( GL_FRAMEBUFFER, 0 );
  // glViewport( 0, 0, 600, 600 );

  // glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

  // glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  // program.use( );

  // // glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );

  // _render->projectionMatrix( ) = _camera->projectionMatrix( );
  // _render->viewMatrix( ) = _camera->viewMatrix( );
  // _render->render( _meshes, _models );

  // glutSwapBuffers( );
}

RemoteRender::ImageJPEG RemoteRender::_encodeJpeg(
  const uint32_t width, const uint32_t height, uint8_t* rawData )
{
  tjhandle compressor= tjInitCompress( );
  const int32_t color_components = 3;
  const int32_t tjPitch = width * color_components;
  // const int32_t tjFlags = TJXOP_ROT180;
  const int JPEG_QUALITY = 75;

  RemoteRender::ImageJPEG image;
  tjCompress2( compressor, rawData, width, tjPitch, height, TJPF_RGB,
               &image.data, &image.size, TJSAMP_444, JPEG_QUALITY,
               TJXOP_ROT180 );

  return image;
}
