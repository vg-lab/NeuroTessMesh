/**
 * Copyright (c) 2015-2017 VG-Lab/URJC.
 *
 * Authors: Juan Jose Garcia Cantero <juanjose.garcia@urjc.es>
 *
 * This file is part of neurolots <https://github.com/vg-lab/neurolots>
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

constexpr float CAMERA_ANIMATION_DURATION = 1.5f;

namespace neurotessmesh
{
  Scene::Scene( reto::OrbitalCameraController* camera ,
                nsol::DataSet* dataset
#ifdef NEUROTESSMESH_USE_SIMIL
                , simil::SpikesPlayer* player
#endif
                )
    : _mode( VISUALIZATION )
    , _colorMode(SELECTION)
    , _camera( camera )
    , _animation( nullptr )
    , _renderer( new nlrender::Renderer( ))
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

    initColors();
    generateMeshes( );
    _boundingBox = computeBoundingBox( );

    const auto fov = _camera->camera()->fieldOfView();
    const auto radius = _boundingBox.radius( ) / std::sin(fov);

    _camera->position( _boundingBox.center( ));
    _camera->radius(radius);
    conformRenderTuples( );
    rebuildNeuronsColors();
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

  void Scene::render()
  {
    Eigen::Matrix4f projection( _camera->camera( )->projectionMatrix( ));
    _renderer->projectionMatrix( ) = projection;
    Eigen::Matrix4f view( _camera->camera( )->viewMatrix( ));
    _renderer->viewMatrix( ) = view;

    switch ( _mode )
    {
      case VISUALIZATION:
#ifdef NEUROTESSMESH_USE_SIMIL
      {
        const float timeStamp = _simulationPlayer ? _simulationPlayer->currentTime() : 0.f;
          _renderer->render( std::get< 0 >( _unselectedNeurons ) ,
                             std::get< 1 >( _unselectedNeurons ) ,
                             _unselectedColors, _simulationPlayer ?
                             calculateUnselectedColors(timeStamp):_unselectedColors , true ,
                             _paintUnselectedSoma ,
                             _paintUnselectedNeurites );
      }
#else
      _renderer->render( std::get< 0 >( _unselectedNeurons ) ,
                         std::get< 1 >( _unselectedNeurons ) ,
                         _unselectedColors, _unselectedColors, true , _paintSelectedSoma ,
                         _paintSelectedNeurites );
#endif

        _renderer->render( std::get< 0 >( _selectedNeurons ) ,
                           std::get< 1 >( _selectedNeurons ) ,
                           _selectedColors, _selectedColors, true , _paintSelectedSoma ,
                           _paintSelectedNeurites );
        break;
      case EDITION:
        if ( isEditNeuronMeshExtraction( ) && _editMesh)
        {
          auto idOfNeuron = [this](const std::pair<unsigned int, nsol::NeuronPtr> &n){ return n.second == this->_editNeuron; };
          auto it = std::find_if(_dataSet->neurons().cbegin(), _dataSet->neurons().cend(), idOfNeuron);
          auto color = _selectedColor;
          if(it != _dataSet->neurons().cend())
            color = neuronColor((*it).first);

          _renderer->render( _editMesh , _editNeuron->transform( ) ,
                             color , true ,
                             _paintUnselectedSoma , _paintUnselectedNeurites );
        }
        break;
      default:
        assert(false);
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
      const auto neuronMapIt = _dataSet->neurons( ).find( id );
      if ( neuronMapIt != _dataSet->neurons( ).end( ))
      {
        const auto neuron = neuronMapIt->second;
        auto morphology = neuron->morphology( );
        if ( morphology )
        {
          const auto radius = morphology->soma( )->maxRadius( );
          const auto center = morphology->soma( )->center( );
          const Eigen::Vector4f position = neuron->transform( ) *
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
    for (const auto& neuronIt: _dataSet->neurons())
    {
      indices.push_back( neuronIt.first );
    }
    return computeBoundingBox( indices );
  }

  void Scene::generateMeshes()
  {
    for (const auto neuronIt: _dataSet->neurons( ))
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

  nsol::NeuronsMap& Scene::neurons() const
  {
    return _dataSet->neurons();
  }

  void Scene::setNeuronToEdit( const unsigned int id_ )
  {
    const auto neuronIt = _dataSet->neurons( ).find( id_ );
    if ( neuronIt != _dataSet->neurons( ).end( ))
    {
      _editNeuron = neuronIt->second;
      if ( _editNeuron )
      {
        const auto meshIt = _neuronMeshes.find( _editNeuron->morphology( ));
        if ( meshIt != _neuronMeshes.end( ))
        {

          _editMesh = meshIt->second;
          mode( Scene::EDITION );
          std::vector< unsigned int > indices = { id_ };
          const auto aabb = computeBoundingBox( indices );
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
      const auto neuron = neuronIt.second;
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

  void Scene::changeSelectedIndices(const std::vector< unsigned int >& indices_ )
  {
    _selectedIndices = std::set<unsigned int>(indices_.begin(), indices_.end());
    conformRenderTuples();
  }

  void Scene::focusOnIndices(const std::vector< unsigned int >& indices_)
  {
    if(!indices_.empty())
    {
      const auto aabb = computeBoundingBox( indices_ );
      animateCamera(aabb.center(),
                    aabb.radius() /
                    std::sin( _camera->camera()->fieldOfView( )));
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
      _animation = nullptr;
    }

    const auto rotInterpolation = rotAnimation ? reto::CameraAnimation::LINEAR
                                               : reto::CameraAnimation::NONE;

    _animation = std::make_shared<reto::CameraAnimation>( reto::CameraAnimation::LINEAR ,
                                                          rotInterpolation ,
                                                          reto::CameraAnimation::LINEAR );

    const auto startCam = new reto::KeyCamera( 0.f , _camera->position( ) ,
                                         _camera->rotation( ) ,
                                         _camera->radius( ));
    _animation->addKeyCamera( startCam );

    const auto targetCam = new reto::KeyCamera( CAMERA_ANIMATION_DURATION ,
                                                position , rotation , radius );
    _animation->addKeyCamera( targetCam );

    _camera->startAnim( _animation.get() );
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

      const Eigen::Vector3f mixedColor = (gradient[ first ].second * (1.f - normalizedT)) + (gradient[ first + 1 ].second * normalizedT);
      return mixedColor;
    };

    auto& neurons = std::get< 0 >( _unselectedNeurons );
    auto colors = _unselectedColors;
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

  void Scene::coloringMode(Scene::TColoringMode mode_)
  {
    if(_colorMode != mode_)
    {
      _colorMode = mode_;
      rebuildNeuronsColors();
    }
  }

  Scene::TColoringMode Scene::coloringMode() const
  {
    return _colorMode;
  }

  void Scene::rebuildNeuronsColors()
  {
    _selectedColors.clear();
    _unselectedColors.clear();
    if(!_dataSet) return;

    for(const auto &neuron: _dataSet->neurons())
    {
      const auto color = neuronColor(neuron.first);

      if(_selectedIndices.find(neuron.first) != _selectedIndices.end())
      {
        _selectedColors.push_back(color);
      }
      else
      {
        _unselectedColors.push_back(color);
      }
    }
  }

  Eigen::Vector3f Scene::neuronColor(const unsigned int id)
  {
    Eigen::Vector3f color{0,0,0};

    const auto neuronIt = _dataSet->neurons().find(id);
    if(neuronIt == _dataSet->neurons().cend()) return color;

    const auto isSelected = _selectedIndices.find(id) != _selectedIndices.end();
    const nsol::NeuronPtr neuron = _colorMode != SELECTION ? (*neuronIt).second : nullptr;

    switch(_colorMode)
    {
      case MORPHOLOGY:
        color = _colors[MORPHOLOGY][static_cast<int>(neuron->morphologicalType())];
        break;
      case LAYER:
        color = _colors[LAYER][neuron->layer()];
        break;
      case FUNCTION:
        color = _colors[FUNCTION][static_cast<int>(neuron->functionalType())];
        break;
      default:
      case SELECTION:
        color = _colors[SELECTION][static_cast<int>(isSelected)];
      break;
    }

    // If Type, layer or function is unknown use selection colors.
    if(color.isZero())
    {
      color = isSelected ? _selectedColor : _unselectedColor;
    }

    return color;
  }

  void Scene::initColors()
  {
    _colors[SELECTION][0] = _unselectedColor;
    _colors[SELECTION][1] = _selectedColor;

    for(int i = 0; i <= nsol::Neuron::TMorphologicalType::DEEP_CEREBELLAR_NUCLEI; ++i)
      _colors[MORPHOLOGY][i] = nsol::Neuron::typeToColor(static_cast<nsol::Neuron::TMorphologicalType>(i));

    for(int i = 1; i <= 6; ++i)
      _colors[LAYER][i] = nsol::Neuron::layerToColor(i);

    for(int i = 0; i <= nsol::Neuron::TFunctionalType::EXCITATORY; ++i)
      _colors[FUNCTION][i] = nsol::Neuron::functionToColor(static_cast<nsol::Neuron::TFunctionalType>(i));
  }

  void Scene::setColor(const int type, const Eigen::Vector3f color)
  {
    switch(_colorMode)
    {
      case SELECTION:
        if(type < 0 || type > 1) return;
        break;
      case MORPHOLOGY:
        if(type < 0 || type > static_cast<int>(nsol::Neuron::TMorphologicalType::DEEP_CEREBELLAR_NUCLEI)) return;
        break;
      case LAYER:
        if(type < 1 || type > 6) return;
        break;
      case FUNCTION:
        if(type < 0 || type > static_cast<int>(nsol::Neuron::TFunctionalType::EXCITATORY)) return;
        break;
      default:
        return;
        break;
    }

    _colors[_colorMode][type] = color;
    rebuildNeuronsColors();
  }

  Eigen::Vector3f Scene::color(const int type) const
  {
    Eigen::Vector3f color{0,0,0};

    switch(_colorMode)
    {
      case SELECTION:
        if(type < 0 || type > 1) return color;
        break;
      case MORPHOLOGY:
        if(type < 0 || type > static_cast<int>(nsol::Neuron::TMorphologicalType::DEEP_CEREBELLAR_NUCLEI)) return color;
        break;
      case LAYER:
        if(type < 1 || type > 6) return color;
        break;
      case FUNCTION:
        if(type < 0 || type > static_cast<int>(nsol::Neuron::TFunctionalType::EXCITATORY)) return color;
        break;
      default:
        return color;
        break;
    }

    return _colors.at(_colorMode).at(type);
  }
}
