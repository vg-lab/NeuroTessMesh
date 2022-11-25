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
#include "Scene.h"

#include <QColor>
#include <QDebug>
#include <utility>
#include <nlgenerator/nlgenerator.h>

constexpr float COLOR_FACTOR = 1 / 255.0f;

namespace neurotessmesh
{
  Scene::Scene( reto::OrbitalCameraController* camera ,
                nsol::DataSet* dataset
#ifdef NEUROTESSMESH_USE_SIMIL
                , simil::SpikesPlayer* player
#endif
                )
    : _mode( VISUALIZATION )
    , _camera( camera )
    , _animation( nullptr )
    , _renderer( new nlrender::Renderer( ))
    , _unselectedColor( 0.5f , 0.5f , 0.8f )
    , _selectedColor( 0.8f , 0.5f , 0.5f )
    , _dataSet( dataset )
#ifdef NEUROTESSMESH_USE_SIMIL
    , _simulationPlayer( player )
#endif
    , _paintUnselectedSoma( true )
    , _paintUnselectedNeurites( true )
    , _paintSelectedSoma( true )
    , _paintSelectedNeurites( true )
    , _editNeuron( nullptr )
    , _editMesh( nullptr )
    , _boundingBox( Eigen::Vector3f::Zero( ) , Eigen::Vector3f::Zero( ))
    , _activationTimestamps( )
    , _gradient( {{ 0.0f , Eigen::Vector3f{ 1.0f , 0.0f , 0.0f }} ,
                  { 1.0f , Eigen::Vector3f{ 0.0f , 0.0f , 1.0f }}} )
    , _delay( 20.0f )
  {
    _attribsFormat.resize( 3 );
    _attribsFormat[ 0 ] = nlgeometry::TAttribType::POSITION;
    _attribsFormat[ 1 ] = nlgeometry::TAttribType::CENTER;
    _attribsFormat[ 2 ] = nlgeometry::TAttribType::TANGENT;

    generateMeshes( );
    _boundingBox = computeBoundingBox( );
    _camera->position( _boundingBox.center( ));
    _camera->radius(
      _boundingBox.radius( ) / std::sin( _camera->camera( )->fieldOfView( )));
    conformRenderTuples( );
  }

  Scene::~Scene( )
  {
    delete _renderer;
    if ( _dataSet )
    {
      _dataSet->close( );
    }
    delete _dataSet;
  }

  void Scene::mode( const Scene::TSceneMode mode_ )
  {
    _mode = mode_;
  }

  Scene::TSceneMode Scene::mode( ) const
  {
    return _mode;
  }

  void Scene::update( )
  {
#ifdef NEUROTESSMESH_USE_SIMIL
    static float timeStamp = -1;

    if ( _simulationPlayer != nullptr && _simulationPlayer->isPlaying( ))
    {
      const auto currentTime = _simulationPlayer->currentTime();

      if(currentTime - timeStamp > std::numeric_limits<float>::epsilon())
      {
        timeStamp = currentTime;

        auto spikes = _simulationPlayer->spikesNow( );

        for ( auto spike = spikes.first; spike != spikes.second; ++spike )
        {
          auto neuronIt = _dataSet->neurons( ).find( spike->second );
          if ( neuronIt == _dataSet->neurons( ).end( )) continue;
          auto morphIt = _neuronMeshes.find( neuronIt->second->morphology( ));
          if ( morphIt == _neuronMeshes.end( )) continue;

          _activationTimestamps[ morphIt->second ] = spike->first;
        }
      }
    }
#endif
  }

  void Scene::render( )
  {
    Eigen::Matrix4f projection( _camera->camera( )->projectionMatrix( ));
    _renderer->projectionMatrix( ) = projection;
    Eigen::Matrix4f view( _camera->camera( )->viewMatrix( ));
    _renderer->viewMatrix( ) = view;

    switch ( _mode )
    {
      case VISUALIZATION:
#ifdef NEUROTESSMESH_USE_SIMIL
        if ( _simulationPlayer != nullptr )
        {
          const auto timeStamp = _simulationPlayer->currentTime();
          _renderer->render( std::get< 0 >( _unselectedNeurons ) ,
                             std::get< 1 >( _unselectedNeurons ) ,
                             calculateUnselectedColors(timeStamp) , true ,
                             _paintUnselectedSoma ,
                             _paintUnselectedNeurites );
        }
        else
        {
          _renderer->render( std::get< 0 >( _unselectedNeurons ) ,
                             std::get< 1 >( _unselectedNeurons ) ,
                             _unselectedColor, true ,
                             _paintUnselectedSoma ,
                             _paintUnselectedNeurites );
        }
#else
        _renderer->render( std::get< 0 >( _unselectedNeurons ) ,
                           std::get< 1 >( _unselectedNeurons ) ,
                           _unselectedColor, true ,
                           _paintUnselectedSoma ,
                           _paintUnselectedNeurites );
#endif

        _renderer->render( std::get< 0 >( _selectedNeurons ) ,
                           std::get< 1 >( _selectedNeurons ) ,
                           _selectedColor , true , _paintSelectedSoma ,
                           _paintSelectedNeurites );
        break;
      case EDITION:
        if ( isEditNeuronMeshExtraction( ))
        {
          _renderer->render( _editMesh , _editNeuron->transform( ) ,
                             _unselectedColor , true ,
                             _paintUnselectedSoma , _paintUnselectedNeurites );
        }
        break;
      default:
        assert( false );
    }
  }

  void Scene::close( )
  {
    _editNeuron = nullptr;
    _editMesh = nullptr;
    for ( auto neuronMesh: _neuronMeshes )
      delete neuronMesh.second;
    _neuronMeshes.clear( );

    std::get< 0 >( _unselectedNeurons ).clear( );
    std::get< 1 >( _unselectedNeurons ).clear( );
    std::get< 0 >( _selectedNeurons ).clear( );
    std::get< 1 >( _selectedNeurons ).clear( );

    _dataSet->close( );
#ifdef NEUROTESSMESH_USE_SIMIL
    if ( _simulationPlayer )
    {
      delete _simulationPlayer;
      _simulationPlayer = nullptr;
    }
#endif
    mode( Scene::VISUALIZATION );
  }

  void Scene::home( )
  {
    mode( Scene::VISUALIZATION );
    _editNeuron = nullptr;
    _editMesh = nullptr;

    const float FOV = std::sin( _camera->camera( )->fieldOfView( ));
    const auto position = _boundingBox.center( );
    const auto radius = _boundingBox.radius( ) / FOV;

    animateCamera( position , radius );
  }

  void
  Scene::cameraPosition( const Eigen::Vector3f& position , const float radius ,
                         const Eigen::Matrix3f& rotation )
  {
    animateCamera( position , radius , rotation , true );
  }

  nlgeometry::AxisAlignedBoundingBox Scene::computeBoundingBox(
    const std::vector< unsigned int >& indices_ )
  {
    Eigen::Array3f minimum =
      Eigen::Array3f::Constant( std::numeric_limits< float >::max( ));
    Eigen::Array3f maximum =
      Eigen::Array3f::Constant( std::numeric_limits< float >::min( ));

    for ( const auto id: indices_ )
    {
      auto neuronMapIt = _dataSet->neurons( ).find( id );
      if ( neuronMapIt != _dataSet->neurons( ).end( ))
      {
        auto neuron = neuronMapIt->second;
        auto morphology = neuron->morphology( );
        if ( morphology )
        {
          auto radius = morphology->soma( )->maxRadius( );
          auto center = morphology->soma( )->center( );
          Eigen::Vector4f position = neuron->transform( ) *
                                     nsol::Vec4f( center.x( ) , center.y( ) ,
                                                  center.z( ) , 1.0f );
          Eigen::Array3f minVec( position.x( ) - radius ,
                                 position.y( ) - radius ,
                                 position.z( ) - radius );
          Eigen::Array3f maxVec( position.x( ) + radius ,
                                 position.y( ) + radius ,
                                 position.z( ) + radius );
          minimum = minimum.min( minVec );
          maximum = maximum.max( maxVec );
        }
      }
    }
    return { minimum , maximum };
  }

  nlgeometry::AxisAlignedBoundingBox Scene::computeBoundingBox( )
  {
    std::vector< unsigned int > indices;
    for ( const auto& neuronIt: _dataSet->neurons( ))
    {
      indices.push_back( neuronIt.first );
    }
    return computeBoundingBox( indices );
  }

  void Scene::generateMeshes( )
  {
    for ( auto neuronIt: _dataSet->neurons( ))
    {
      auto morphology = neuronIt.second->morphology( );
      if ( morphology )
      {
        if ( _neuronMeshes.find( morphology ) == _neuronMeshes.end( ))
        {
          auto simplifier = nsol::Simplifier::Instance( );
          simplifier->adaptSoma( morphology );
          simplifier->simplify( morphology ,
                                nsol::Simplifier::DIST_NODES_RADIUS );

          auto mesh = nlgenerator::MeshGenerator::generateMesh( morphology );
          mesh->uploadGPU( _attribsFormat , nlgeometry::Facet::PATCHES );
          mesh->clearCPUData( );
          _neuronMeshes[ morphology ] = mesh;
        }
      }
      else
      {
        throw std::runtime_error( "Unable to load neuron morphology" );
      }
    }
  }

  void Scene::paintUnselectedSoma( bool paint_ )
  {
    _paintUnselectedSoma = paint_;
  }

  void Scene::paintUnselectedNeurites( bool paint_ )
  {
    _paintUnselectedNeurites = paint_;
  }

  void Scene::paintSelectedSoma( bool paint_ )
  {

    _paintSelectedSoma = paint_;
  }

  void Scene::paintSelectedNeurites( bool paint_ )
  {

    _paintSelectedNeurites = paint_;
  }

  void Scene::unselectedNeuronColor( Eigen::Vector3f color_ )
  {
    _unselectedColor = std::move( color_ );

    _gradient[1] = { 1.0f , { _unselectedColor[0], _unselectedColor[1], _unselectedColor[2] } };
  }

  void Scene::unselectedNeuronColor( const QColor& color_ )
  {
    unselectedNeuronColor( Eigen::Vector3f(
      float( color_.red( )) * COLOR_FACTOR ,
      float( color_.green( )) * COLOR_FACTOR ,
      float( color_.blue( )) * COLOR_FACTOR
    ));
  }

  void Scene::selectedNeuronColor( Eigen::Vector3f color_ )
  {
    _selectedColor = std::move( color_ );
  }

  void Scene::selectedNeuronColor( const QColor& color_ )
  {
    selectedNeuronColor( Eigen::Vector3f(
      float( color_.red( )) * COLOR_FACTOR ,
      float( color_.green( )) * COLOR_FACTOR ,
      float( color_.blue( )) * COLOR_FACTOR
    ));
  }

  void Scene::levelOfDetail( float lod_ )
  {
    if ( _renderer )
      _renderer->lod( ) = lod_;
  }

  void Scene::maximumDistance( float maximumDistance_ )
  {
    if ( _renderer )
      _renderer->maximumDistance( ) =
        maximumDistance_ * _camera->camera( )->farPlane( );
  }

  void Scene::subdivisionCriteria(
    nlrender::Renderer::TTessCriteria subdivisionCriteria_ )
  {
    _renderer->tessCriteria( subdivisionCriteria_ );
  }

  std::vector< unsigned int > Scene::neuronIndices( )
  {
    std::vector< unsigned int > indices;

    for ( auto neuronIt: _dataSet->neurons( ))
    {
      indices.push_back( neuronIt.first );
    }
    return indices;
  }

  void Scene::setNeuronToEdit( unsigned int id_ )
  {
    auto neuronIt = _dataSet->neurons( ).find( id_ );
    if ( neuronIt != _dataSet->neurons( ).end( ))
    {
      _editNeuron = neuronIt->second;
      if ( _editNeuron )
      {
        auto meshIt = _neuronMeshes.find( _editNeuron->morphology( ));
        if ( meshIt != _neuronMeshes.end( ))
        {
          _editMesh = meshIt->second;
          mode( Scene::EDITION );
          std::vector< unsigned int > indices = { id_ };
          auto aabb = computeBoundingBox( indices );
          animateCamera( aabb.center( ) , aabb.radius( ) / std::sin(
            _camera->camera( )->fieldOfView( )));
        }
        else
          _editNeuron = nullptr;
      }
    }
  }

  unsigned int Scene::numEditMorphologyNeurites( ) const
  {
    if ( _editNeuron )
      return static_cast<unsigned int>(_editNeuron->morphology( )->neurites( ).size( ));
    return 0;
  }

  void Scene::regenerateEditNeuronMesh(
    const float alphaRadius_ ,
    const std::vector< float >& alphaNeurites_ )
  {
    if ( isEditNeuronMeshExtraction( ))
    {
      auto mesh = nlgenerator::MeshGenerator::generateMesh(
        _editNeuron->morphology( ) , alphaRadius_ , alphaNeurites_ );
      if ( mesh )
      {
        mesh->uploadGPU( _attribsFormat , nlgeometry::Facet::PATCHES );
        mesh->clearCPUData( );
        delete _editMesh;
        _editMesh = mesh;
        _neuronMeshes[ _editNeuron->morphology( ) ] = mesh;
        conformRenderTuples( );
      }
    }
  }

  bool Scene::isEditNeuronMeshExtraction( )
  {
    return ( _editNeuron != nullptr ) && ( _editMesh != nullptr );
  }

  void Scene::extractEditNeuronMesh( const std::string& path_ )
  {
    auto extractedMesh = _renderer->extract(
      _editMesh , _editNeuron->transform( ) , _paintUnselectedSoma ,
      _paintUnselectedNeurites );
    nlgeometry::ObjWriter::writeMesh( extractedMesh , path_ );
    delete extractedMesh;
  }

  void Scene::conformRenderTuples( )
  {
    nlgeometry::Meshes unselectedMeshes;
    std::vector< Eigen::Matrix4f > unselectedModels;
    nlgeometry::Meshes selectedMeshes;
    std::vector< Eigen::Matrix4f > selectedModels;
    for ( const auto neuronIt: _dataSet->neurons( ))
    {
      auto neuron = neuronIt.second;
      auto meshIt = _neuronMeshes.find( neuron->morphology( ));
      if ( meshIt != _neuronMeshes.end( ))
      {
        if ( _selectedIndices.find( neuronIt.first ) != _selectedIndices.end( ))
        {
          selectedMeshes.push_back( meshIt->second );
          selectedModels.push_back( neuron->transform( ));
        }
        else
        {
          unselectedMeshes.push_back( meshIt->second );
          unselectedModels.push_back( neuron->transform( ));
        }
      }
    }
    _unselectedNeurons = std::make_tuple( unselectedMeshes , unselectedModels );
    _selectedNeurons = std::make_tuple( selectedMeshes , selectedModels );
  }

  void Scene::changeSelectedIndices(
    const std::vector< unsigned int >& indices_ )
  {
    _selectedIndices = std::set< unsigned int >(
      indices_.begin( ) , indices_.end( ));
    conformRenderTuples( );
  }

  void Scene::focusOnIndices( const std::vector< unsigned int >& indices_ )
  {
    if ( !indices_.empty( ))
    {
      auto aabb = computeBoundingBox( indices_ );
      animateCamera( aabb.center( ) ,
                     aabb.radius( ) /
                     std::sin( _camera->camera( )->fieldOfView( )));
    }
    else
    {
      animateCamera( _boundingBox.center( ) , _boundingBox.radius( ) / std::sin(
        _camera->camera( )->fieldOfView( )));
    }
  }

  void
  Scene::animateCamera( const Eigen::Vector3f& position , const float radius ,
                        const Eigen::Matrix3f& rotation ,
                        bool rotAnimation )
  {
    if ( _camera->isAniming( ))
    {
      _camera->stopAnim( );
      delete _animation;
    }

    constexpr float CAMERA_ANIMATION_DURATION = 2.f;
    const auto rotInterpolation = rotAnimation ? reto::CameraAnimation::LINEAR
                                               : reto::CameraAnimation::NONE;

    _animation = new reto::CameraAnimation( reto::CameraAnimation::LINEAR ,
                                            rotInterpolation ,
                                            reto::CameraAnimation::LINEAR );

    auto startCam = new reto::KeyCamera( 0.f , _camera->position( ) ,
                                         _camera->rotation( ) ,
                                         _camera->radius( ));
    _animation->addKeyCamera( startCam );

    auto targetCam = new reto::KeyCamera( CAMERA_ANIMATION_DURATION ,
                                          position , rotation , radius );
    _animation->addKeyCamera( targetCam );

    _camera->startAnim( _animation );
  }

  std::vector< Eigen::Vector3f > Scene::calculateUnselectedColors( float timestamp )
  {
    auto calculateGradientColor = [ ]( const Gradient& gradient , float t )
    {
      if ( gradient.empty( )) return Eigen::Vector3f( 1.0f , 0.0f , 1.0f );
      // Return last if t < 0.
      if ( t < 0 ) return gradient[ gradient.size( ) - 1 ].second;
      int size = static_cast<int>(gradient.size( ));
      int first = size - 1;
      for ( int i = 0; i < size; i++ )
      {
        if ( gradient[ i ].first > t )
        {
          first = i - 1;
          break;
        }
      }

      if ( first == -1 ) return gradient[ 0 ].second;
      if ( first == size - 1 ) return gradient[ first ].second;

      float start = gradient[ first ].first;
      float end = gradient[ first + 1 ].first;
      float normalizedT = ( t - start ) / ( end - start );

      const Eigen::Vector3f mixedColor = (gradient[ first ].second * (1. - normalizedT)) + (gradient[ first + 1 ].second * normalizedT);
      return mixedColor;
    };

    auto& neurons = std::get< 0 >( _unselectedNeurons );

    auto colors = std::vector< Eigen::Vector3f >(neurons.size(), _unselectedColor);
    if(timestamp < 0
#ifdef NEUROTESSMESH_USE_SIMIL
        || !_simulationPlayer
#endif
        ) return colors;

    uint32_t index = 0;
    for ( const auto& mesh: neurons )
    {
      auto it = _activationTimestamps.find( mesh );
      if ( it == _activationTimestamps.end( ))
      {
        colors[ index ] = calculateGradientColor( _gradient , INFINITY);
      }
      else
      {
        colors[ index ] = calculateGradientColor( _gradient ,
                                                 ( timestamp - it->second ) / _delay );
      }
      ++index;
    }
    return colors;
  }
}
