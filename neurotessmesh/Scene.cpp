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

#include <nlgenerator/nlgenerator.h>

namespace neurotessmesh
{

  Scene::Scene( reto::Camera* camera_ )
    : _mode( VISUALIZATION )
    , _camera( camera_ )
    , _unselectedColor( 0.5f, 0.5f, 0.8f )
    , _selectedColor( 0.8f, 0.5f, 0.5f )
    , _paintUnselectedSoma( true )
    , _paintUnselectedNeurites( true )
    , _paintSelectedSoma( true )
    , _paintSelectedNeurites( true )
    , _editNeuron( nullptr )
    , _editMesh( nullptr )
    , _boundingBox( Eigen::Vector3f::Zero( ), Eigen::Vector3f::Zero( ))
  {
    _attribsFormat.resize( 3 );
    _attribsFormat[0] = nlgeometry::TAttribType::POSITION;
    _attribsFormat[1] = nlgeometry::TAttribType::CENTER;
    _attribsFormat[2] = nlgeometry::TAttribType::TANGENT;
    _renderer = new nlrender::Renderer( );
    _dataSet = new nsol::DataSet( );
  }

  Scene::~Scene( void )
  {
    if(_renderer) delete _renderer;
    if(_dataSet)  delete _dataSet;
  }

  void Scene::mode( const Scene::TSceneMode mode_ )
  {
    _mode = mode_;
  }

  Scene::TSceneMode Scene::mode( void ) const
  {
    return _mode;
  }

  void Scene::render( void )
  {
    Eigen::Matrix4f projection( _camera->projectionMatrix( ));
    _renderer->projectionMatrix( ) = projection;
    Eigen::Matrix4f view( _camera->viewMatrix( ));
    _renderer->viewMatrix( ) = view;

    switch( _mode )
    {
    case VISUALIZATION:
      _renderer->render( std::get<0>( _unselectedNeurons ),
                         std::get<1>( _unselectedNeurons ),
                         _unselectedColor , _paintUnselectedSoma,
                         _paintUnselectedNeurites );
      _renderer->render( std::get<0>( _selectedNeurons ),
                         std::get<1>( _selectedNeurons ),
                         _selectedColor , _paintSelectedSoma,
                         _paintSelectedNeurites );
      break;
    case EDITION:
      if ( isEditNeuronMeshExtraction( ))
      {
        _renderer->render( _editMesh, _editNeuron->transform( ),
                           _unselectedColor,
                           _paintUnselectedSoma, _paintUnselectedNeurites );
      }
      break;
    }
  }

  void Scene::close( void )
  {
    _editNeuron = nullptr;
    _editMesh = nullptr;
    for ( auto neuronMesh: _neuronMeshes )
      delete neuronMesh.second;
    _neuronMeshes.clear( );

    std::get<0>( _unselectedNeurons ).clear( );
    std::get<1>( _unselectedNeurons ).clear( );
    std::get<0>( _selectedNeurons ).clear( );
    std::get<1>( _selectedNeurons ).clear( );

    _dataSet->close( );
    mode( Scene::VISUALIZATION );
  }

  void Scene::home( void )
  {
    mode( Scene::VISUALIZATION );
    _editNeuron = nullptr;
    _editMesh = nullptr;
    _camera->targetPivot( _boundingBox.center( ));
    _camera->targetRadius( _boundingBox.radius( ) / sin( _camera->fov( )));
  }

  nlgeometry::AxisAlignedBoundingBox Scene::computeBoundingBox(
    std::vector< unsigned int > indices_ )
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
            nsol::Vec4f( center.x( ) , center.y( ), center.z( ), 1.0f );
          Eigen::Array3f minVec( position.x( ) - radius, position.y( ) - radius,
                                 position.z( ) - radius );
          Eigen::Array3f maxVec( position.x( ) + radius, position.y( ) + radius,
                                 position.z( ) + radius );
          minimum = minimum.min( minVec );
          maximum = maximum.max( maxVec );
        }
      }
    }
    return nlgeometry::AxisAlignedBoundingBox( minimum, maximum );
  }

  nlgeometry::AxisAlignedBoundingBox Scene::computeBoundingBox( void )
  {
    std::vector< unsigned int > indices;
    for ( const auto& neuronIt: _dataSet->neurons( ))
    {
      indices.push_back( neuronIt.first );
    }
    return computeBoundingBox( indices );
  }

  void Scene::generateMeshes( void )
  {
    for ( auto neuronIt: _dataSet->neurons( ))
    {
      auto morphology = neuronIt.second->morphology( );
      if ( _neuronMeshes.find( morphology ) == _neuronMeshes.end( ))
      {
        auto simplifier = nsol::Simplifier::Instance( );
        simplifier->adaptSoma( morphology );
        simplifier->simplify( morphology, nsol::Simplifier::DIST_NODES_RADIUS );

        auto mesh = nlgenerator::MeshGenerator::generateMesh( morphology );
        mesh->uploadGPU( _attribsFormat, nlgeometry::Facet::PATCHES );
        mesh->clearCPUData( );
        _neuronMeshes[ morphology ] = mesh;
      }
    }
  }

  void Scene::loadData( const std::string& fileName_,
                        const TDataFileType fileType_,
#ifdef NSOL_USE_BRION
                        const std::string& target_
#else
                        const std::string& /*target_*/
#endif
    )
  {
    close( );
    try{
      switch( fileType_ )
      {
      case TDataFileType::BlueConfig:
#ifdef NSOL_USE_BRION
        _dataSet->loadBlueConfigHierarchy< nsol::Node,
                                           nsol::NeuronMorphologySection,
                                           nsol::Dendrite,
                                           nsol::Axon,
                                           nsol::Soma,
                                           nsol::NeuronMorphology,
                                           nsol::Neuron,
                                           nsol::MiniColumn,
                                           nsol::Column >( fileName_,
                                                           target_ );

        _dataSet->loadAllMorphologies< nsol::Node,
                                       nsol::NeuronMorphologySection,
                                       nsol::Dendrite,
                                       nsol::Axon,
                                       nsol::Soma,
                                       nsol::NeuronMorphology,
                                       nsol::Neuron,
                                       nsol::MiniColumn,
                                       nsol::Column >( );
#else
        std::cerr << "Error: Brion support not built-in" << std::endl;
#endif
        break;

      case TDataFileType::SWC:
        _dataSet->loadNeuronFromFile< nsol::Node,
                                      nsol::NeuronMorphologySection,
                                      nsol::Dendrite,
                                      nsol::Axon,
                                      nsol::Soma,
                                      nsol::NeuronMorphology,
                                      nsol::Neuron >( fileName_, 1 );
        break;

      case TDataFileType::NsolScene:
        _dataSet->loadXmlScene< nsol::Node,
                                nsol::NeuronMorphologySection,
                                nsol::Dendrite,
                                nsol::Axon,
                                nsol::Soma,
                                nsol::NeuronMorphology,
                                nsol::Neuron >( fileName_ );
        break;

      default:
        throw std::runtime_error( "Data file type not supported" );
      }
    }
    catch( const std::exception &excep )
    {
      std::cerr << "Error: can't load file: " << fileName_ << std::endl;
      std::cerr << excep.what( ) << std::endl;
    }
    generateMeshes( );
    _boundingBox = computeBoundingBox( );
    _camera->pivot( _boundingBox.center( ));
    _camera->radius( _boundingBox.radius( ) / sin( _camera->fov( )));
    conformRenderTuples( );
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
    _unselectedColor = color_;
  }

  void Scene::selectedNeuronColor( Eigen::Vector3f color_ )
  {
    _selectedColor = color_;
  }

  void Scene::levelOfDetail( float lod_ )
  {
    if(_renderer)
      _renderer->lod( ) = lod_;
  }

  void Scene::maximumDistance( float maximumDistance_ )
  {
    if(_renderer)
      _renderer->maximumDistance( ) = maximumDistance_ * _camera->farPlane( );
  }

  void Scene::subdivisionCriteria(
    nlrender::Renderer::TTessCriteria subdivisionCriteria_ )
  {
    _renderer->tessCriteria( ) = subdivisionCriteria_;
  }

  std::vector< unsigned int > Scene::neuronIndices( void )
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
        auto meshIt = _neuronMeshes.find( _editNeuron->morphology( ) );
        if ( meshIt != _neuronMeshes.end( ))
        {
          _editMesh = meshIt->second;
          mode( Scene::EDITION );
          std::vector< unsigned int >indices = { id_ };
          auto aabb = computeBoundingBox( indices );
          _camera->targetPivot( aabb.center( ));
          _camera->targetRadius( aabb.radius( ) / sin( _camera->fov( )));
        }
        else
          _editNeuron = nullptr;
      }
    }
  }

  unsigned int Scene::numEditMorphologyNeurites( void ) const
  {
    if ( _editNeuron )
      return ( unsigned int )_editNeuron->morphology( )->neurites( ).size( );
    return 0;
  }

  void Scene::regenerateEditNeuronMesh(
    const float alphaRadius_,
    const std::vector< float >& alphaNeurites_ )
  {
    if (  isEditNeuronMeshExtraction( ))
    {
      auto mesh = nlgenerator::MeshGenerator::generateMesh(
        _editNeuron->morphology( ), alphaRadius_, alphaNeurites_ );
      if ( mesh )
      {
        mesh->uploadGPU( _attribsFormat, nlgeometry::Facet::PATCHES );
        mesh->clearCPUData( );
        delete _editMesh;
        _editMesh = mesh;
        _neuronMeshes[ _editNeuron->morphology( )] = mesh;
        conformRenderTuples( );
      }
    }
  }

  bool Scene::isEditNeuronMeshExtraction( void )
  {
    return ( _editNeuron != nullptr ) && ( _editMesh != nullptr );
  }

  void Scene::extractEditNeuronMesh( const std::string& path_ )
  {
    auto extractedMesh = _renderer->extract(
      _editMesh, _editNeuron->transform( ), _paintUnselectedSoma,
      _paintUnselectedNeurites );
    nlgeometry::ObjWriter::writeMesh( extractedMesh, path_ );
    delete extractedMesh;
  }

  void Scene::conformRenderTuples( void )
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
    _unselectedNeurons = std::make_tuple( unselectedMeshes, unselectedModels );
    _selectedNeurons = std::make_tuple( selectedMeshes, selectedModels );
  }

  void Scene::changeSelectedIndices(
    const std::vector< unsigned int >& indices_ )
  {
    _selectedIndices = std::set< unsigned int >(
      indices_.begin( ), indices_.end( ));
    conformRenderTuples( );
  }

  void Scene::focusOnIndices( const std::vector< unsigned int >& indices_ )
  {
    if ( indices_.size( ) > 0 )
    {
      auto aabb = computeBoundingBox( indices_ );
      _camera->targetPivot( aabb.center( ));
      _camera->targetRadius( aabb.radius( ) / sin( _camera->fov( )));
    }
    else
    {
      _camera->targetPivot( _boundingBox.center( ));
      _camera->targetRadius( _boundingBox.radius( ) / sin( _camera->fov( )));
    }
  }
}
