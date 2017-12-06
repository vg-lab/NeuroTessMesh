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
#include <nlrender/version.h>
#include <nlrender/nlrender.h>
#include <nlgeometry/nlgeometry.h>
#include <nlgenerator/nlgenerator.h>
#include <rockets/server.h>
#include <lexis/render/imageJPEG.h>
#include <turbojpeg.h>
#include "LookAtCamera.h"

class RemoteRender
{
private:
  struct ImageJPEG
  {
    size_t size{0};
    uint8_t* data;
  };

  struct imageSize
  {
    size_t width{600};
    size_t height{600};
  };

public:

  RemoteRender( void );

  ~RemoteRender( void );

  void loadMorphologies( void );

  void run( void );

protected:

  void _init( void );

  void _renderToImage( void );

  ImageJPEG _encodeJpeg(
    const uint32_t width, const uint32_t height, uint8_t* rawData );

  rockets::Server* _server;
  LookAtCamera* _camera;
  nlrender::Version* _version;
  lexis::render::ImageJPEG* _image;
  nlrender::Renderer* _render;

  std::vector< nlgeometry::MeshPtr > _meshes;
  std::vector< Eigen::Matrix4f > _models;

  imageSize _size;
  unsigned int _fbo;
  unsigned int _texture;

};
