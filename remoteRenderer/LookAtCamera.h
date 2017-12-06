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
#include <string>
#include <Eigen/Dense>

class LookAtCamera
{

public:
  LookAtCamera( void );

  ~LookAtCamera( void );

  std::string jsonSchema( void );

  std::string toJson( void );

  void fromJson( const std::string& json_ );

  float& ratio( void ){ return _ratio; }

  Eigen::Matrix4f& projectionMatrix( void ){ return _projectionMatrix; }

  Eigen::Matrix4f& viewMatrix( void ){ return _viewMatrix; }

  void origin( Eigen::Vector3f origin_ )
  {
    _origin = origin_;
    _buildViewMatrix( );
  }

  void lookAt( Eigen::Vector3f lookAt_ )
  {
    _lookAt = lookAt_;
    _buildViewMatrix( );
  }

  void up( Eigen::Vector3f up_ )
  {
    _up = up_;
    _buildViewMatrix( );
  }

  float fieldOfView( void )
  {
    return _fov;
  }

protected:

  void _buildViewMatrix( );
  void _buildProjectionMatrix( );
  
  Eigen::Vector3f _origin;
  Eigen::Vector3f _lookAt;
  Eigen::Vector3f _up;

  Eigen::Matrix4f _projectionMatrix;
  Eigen::Matrix4f _viewMatrix;

  float _fov;
  float _nearPlane;
  float _farPlane;
  float _ratio;
};
