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
#include "LookAtCamera.h"
#include "json.hpp"

#include <iostream>

using json = nlohmann::json;

LookAtCamera::LookAtCamera( void )
  : _origin( Eigen::Vector3f( 0.0f, 0.0f, -100.0f))
  , _lookAt( Eigen::Vector3f( 0.0f, 0.0f, 0.0f))
  , _up( Eigen::Vector3f( 0.0f, 1.0f, 0.0f ))
  , _ratio( 1.0f )
{
  _fov = 45.0f * ( M_PI / 360.0f );
  _nearPlane = 0.001f;
  _farPlane = 10000.0f;
  _buildProjectionMatrix( );
  _buildViewMatrix( );
}

LookAtCamera::~LookAtCamera( void )
{

}

std::string LookAtCamera::toJson( void )
{
  std::string jsonString;
  jsonString += "{\n  \"origin\": [ " + std::to_string( _origin.x( )) + ", " +
    std::to_string( _origin.y( )) + ", " +
    std::to_string( _origin.z( )) + "],\n";

  jsonString += "  \"look_at\": [ " + std::to_string( _lookAt.x( )) + ", " +
    std::to_string( _lookAt.y( )) + ", " +
    std::to_string( _lookAt.z( )) + "],\n";

  jsonString += "  \"up\": [ " + std::to_string( _up.x( )) + ", " +
    std::to_string( _up.y( )) + ", " +
    std::to_string( _up.z( )) + "],\n";

  jsonString += "  \"field_of_view\": " + std::to_string( _fov ) + "\n}";
  return jsonString;
}

void LookAtCamera::fromJson( const std::string& json_ )
{
  auto jsonData = json::parse(json_);

  auto originData = jsonData.find( "origin" );
  if ( originData != jsonData.end( ))
  {
    _origin.x( ) = originData->at(0);
    _origin.y( ) = originData->at(1);
    _origin.z( ) = originData->at(2);
  }

  auto lookAtData = jsonData.find( "look_at" );
  if ( lookAtData != jsonData.end( ))
  {
    _lookAt.x( ) = lookAtData->at(0);
    _lookAt.y( ) = lookAtData->at(1);
    _lookAt.z( ) = lookAtData->at(2);
  }

  auto upData = jsonData.find( "up" );
  if ( upData != jsonData.end( ))
  {
    _up.x( ) = upData->at(0);
    _up.y( ) = upData->at(1);
    _up.z( ) = upData->at(2);
  }

  auto fovData = jsonData.find( "field_of_view" );
  if ( fovData != jsonData.end( ))
  {
    _fov = (float)(*fovData ) * ( M_PI / 360.0f );
    _buildProjectionMatrix( );
  }

  _buildViewMatrix( );
}


void LookAtCamera::_buildProjectionMatrix( void )
{
  float f = 1.0f / tan( _fov );
  auto nf = 1.0f / ( _nearPlane - _farPlane );

  _projectionMatrix <<
    f /_ratio, 0.0f, 0.0f, 0.0f,
    0.0f, f, 0.0f, 0.0f,
    0.0f, 0.0f, (_farPlane+_nearPlane)*nf, -1.0f,
    0.0f, 0.0f, (2.0f*_farPlane*_nearPlane)*nf, 0.0f;
}

void LookAtCamera::_buildViewMatrix( void )
{
  Eigen::Vector3f vz = _origin - _lookAt;
  vz.normalize( );
  Eigen::Vector3f vx = _up.normalized( ).cross( vz );
  vx.normalize( );
  Eigen::Vector3f vy = vz.cross( vx );
  vy.normalize( );

  Eigen::Vector3f tr = -_origin;

  _viewMatrix <<
    vx.x( ), vy.x( ), vz.x( ), 0.0f,
    vx.y( ), vy.y( ), vz.y( ), 0.0f,
    vx.z( ), vy.z( ), vz.z( ), 0.0f,
    vx.dot(tr), vy.dot(tr), vz.dot(tr), 1.0f;
}

std::string LookAtCamera::jsonSchema( void )
{
  return R"({
    "$schema": "http://json-schema.org/schema#",
    "title": "Camera",
    "description": "Class Camera of namespace ['brayns','v1']",
    "type": "object",
    "additionalProperties": false,
    "properties": {
        "origin": {
            "type": "array",
            "minItems": 3,
            "maxItems": 3,
            "items": {
                "type": "number"
            }
        },
        "look_at": {
            "type": "array",
            "minItems": 3,
            "maxItems": 3,
            "items": {
                "type": "number"
            }
        },
        "up": {
            "type": "array",
            "minItems": 3,
            "maxItems": 3,
            "items": {
                "type": "number"
            }
        },
        "field_of_view": {
            "type": "number"
        },
        "aperture": {
            "type": "number"
        },
        "focal_length": {
            "type": "number"
        },
        "stereo_mode": {
            "$schema": "http://json-schema.org/schema#",
            "title": "CameraStereoMode",
            "description": "Enum CameraStereoMode of type uint",
            "type": "string",
            "additionalProperties": false,
            "enum": [
                "none",
                "left",
                "right",
                "side_by_side"
            ]
        },
        "eye_separation": {
            "type": "number"
        }
    }
})";
}
