/*
 * Copyright (c) 2022 VG-Lab/URJC.
 *
 * Authors: Felix de las Pozas Alvarez <felix.delaspozas@urjc.es>
 *
 * This file is part of SimIL <https://github.com/vg-lab/SimIL>
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

#include "LoaderThread.h"

// NSOL
#include <memory>
#include <nsol/nsol.h>

// SimIL
#ifdef NEUROTESSMESH_USE_SIMIL
#include <simil/simil.h>
#endif

// Eigen
#include <Eigen/Geometry>
#include <Eigen/Eigen>

// Qt
#include <QString>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QIcon>

// C++
#include <memory>

using namespace neurotessmesh;

LoaderThread::LoaderThread( const std::string& arg1 , const std::string& arg2 ,
                            const LoaderThread::DataFileType type )
  : QThread( )
  , m_fileName{ arg1 }
  , m_target{ arg2 }
  , m_type{ type }
  , m_dataset{ nullptr }
  , m_player{ nullptr }
{
}

void LoaderThread::run( )
{
  try
  {
    m_dataset = new nsol::DataSet( );
    QFileInfo fi( QString::fromStdString( m_fileName ));
    emit progress( QString( "Loading %1" ).arg( fi.fileName( )) , 10 );

    switch ( m_type )
    {
      case DataFileType::BlueConfig:
#ifdef NSOL_USE_BRION
        emit progress( tr( "Loading Hierarchy" ) , 25 );

        m_dataset->loadBlueConfigHierarchy< nsol::Node ,
          nsol::NeuronMorphologySection ,
          nsol::Dendrite ,
          nsol::Axon ,
          nsol::Soma ,
          nsol::NeuronMorphology ,
          nsol::Neuron ,
          nsol::MiniColumn ,
          nsol::Column >( m_fileName , m_target );

        emit progress( tr( "Loading Morphologies" ) , 50 );

        m_dataset->loadAllMorphologies< nsol::Node ,
          nsol::NeuronMorphologySection ,
          nsol::Dendrite ,
          nsol::Axon ,
          nsol::Soma ,
          nsol::NeuronMorphology ,
          nsol::Neuron ,
          nsol::MiniColumn ,
          nsol::Column >( );

//        emit progress( tr( "Loading Spikes" ) , 75 );
#ifdef NEUROTESSMESH_USE_SIMIL
//        { // load the spikes data with SimIL, only for blueconfig.
//          auto spikesData = std::make_shared< simil::SpikeData >(
//            m_fileName ,
//            simil::TDataType::TBlueConfig ,
//            m_target
//          );
//          spikesData->reduceDataToGIDS( );
//
//          m_player = new simil::SpikesPlayer( );
//          m_player->LoadData( spikesData );
//        }
#endif
#else
        std::cerr << "Error: Brion support not built-in" << std::endl;
#endif
        break;

      case DataFileType::SWC:
        emit progress( tr( "Loading Neuron" ) , 50 );
        m_dataset->loadNeuronFromFile< nsol::Node ,
          nsol::NeuronMorphologySection ,
          nsol::Dendrite ,
          nsol::Axon ,
          nsol::Soma ,
          nsol::NeuronMorphology ,
          nsol::Neuron >( m_fileName , 1 );
        break;

      case DataFileType::NsolScene:
        emit progress( tr( "Loading Scene" ) , 50 );
        m_dataset->loadXmlScene< nsol::Node ,
          nsol::NeuronMorphologySection ,
          nsol::Dendrite ,
          nsol::Axon ,
          nsol::Soma ,
          nsol::NeuronMorphology ,
          nsol::Neuron >( m_fileName );
        break;

      case DataFileType::HDF5:
#ifdef NEUROTESSMESH_USE_SIMIL
        loadH5Morphology( );
#endif
        break;

      default:
        throw std::runtime_error( "Data file type not supported" );
    }

    emit progress( "Generating Meshes" , 100 );

  }
  catch ( const std::exception& e )
  {
    delete m_dataset;
    m_errors = QString::fromStdString( e.what( ));
  }
}

#ifdef NEUROTESSMESH_USE_SIMIL

uint8_t LoaderThread::getTypeFromLoaderType( const NeuronType& type )
{
  switch ( type )
  {
    case NeuronType::GRANULE_CELL:
      return nsol::Neuron::GRANULE;
    case NeuronType::GOLGI_CELL:
      return nsol::Neuron::GOLGI;
    case NeuronType::PURKINJE_CELL:
      return nsol::Neuron::PURKINJE;
    case NeuronType::STELLATE_CELL:
      return nsol::Neuron::STELLATE;
    case NeuronType::BASKET_CELL:
      return nsol::Neuron::BASKET;
    case NeuronType::UNDEFINED:
    default:
      break;
  }
  return nsol::Neuron::UNDEFINED;
}

void LoaderThread::loadH5Morphology( )
{
  H5Morphologies loader( m_fileName , "" );
  loader.load( );

  std::map< NeuronType , nsol::NeuronMorphology* > morphologiesByType;

  uint32_t neuronId = 0;
  for ( const auto& pair: loader.getMorphologies( ))
  {
    auto& neuron = pair.second;

    auto* soma = new nsol::Soma( );
    auto* morphology = new nsol::NeuronMorphology( soma );

    std::vector< nsol::NeuronMorphologySection* > sections;
    sections.resize( neuron.neurites.size( ));

    for ( uint32_t id = 0; id < neuron.neurites.size( ); ++id )
    {
      auto& item = neuron.neurites[ id ];
      if ( item.type == MorphologyType::SOMA )
      {
        for ( uint32_t i = 0; i < item.radii.size( ); ++i )
        {
          soma->addNode( new nsol::Node(
            nsol::Vec3f(
              static_cast< float >(item.x[ i ]) ,
              static_cast< float >(item.y[ i ]) ,
              static_cast< float >(item.z[ i ])
            ) ,
            0 ,
            static_cast< float >(item.radii[ i ])
          ));
        }
      }
      else
      {
        auto nsolType  = static_cast<nsol::Neuron::TMorphologicalType>(getTypeFromLoaderType(pair.first));

        auto* section = new nsol::NeuronMorphologySection( );
        sections[ id ] = section;
        if ( item.radii.empty( ))
        {
          std::cout << "WARNING! Neurite " << id << " has no nodes! (type "
                    << nsol::Neuron::typeToString(nsolType) << ")"
                    << std::endl;
          section->addNode( new nsol::Node(
            nsol::Vec3f( 0.0f , 0.0f , 0.0f ) , 0 , 0.0f
          ));
        }

        // Add nodes
        for ( uint32_t i = 0; i < item.radii.size( ); ++i )
        {
          section->addNode( new nsol::Node(
            nsol::Vec3f(
              static_cast< float >(item.x[ i ]) ,
              static_cast< float >(item.y[ i ]) ,
              static_cast< float >(item.z[ i ])
            ) ,
            0 ,
            static_cast< float >(item.radii[ i ])
          ));
        }

        auto parentType = neuron.neurites[ item.parent ].type;
        if ( parentType == item.type )
        {
          auto* parent = sections[ item.parent ];
          if ( parent != nullptr )
          {
            section->parent( parent );
            parent->addChild( section );
          }
          else
          {
            std::cout << "WARNING! Neurite " << id << " has no parent! (type "
                      << nsol::Neuron::typeToString(nsolType) << ")"
                      << std::endl;
          }
        }
        else
        {
          nsol::Neurite* neurite;
          if ( item.type == MorphologyType::AXON ) neurite = new nsol::Axon( );
          else neurite = new nsol::Dendrite( );
          neurite->firstSection( section );
          morphology->addNeurite( neurite );
        }
      }
    }

    morphologiesByType[ pair.first ] = morphology;
  }

  for ( const auto& neuron: loader.getNeurons( ))
  {
    auto morphology = morphologiesByType[ neuron.type ];

    Eigen::Affine3f transform(
      Eigen::Translation3f( neuron.x , neuron.y , neuron.z ));


    auto* result = new nsol::Neuron(
      morphology ,
      0 ,
      neuronId++ ,
      transform.matrix( ) ,
      nullptr ,
      static_cast<nsol::Neuron::TMorphologicalType>(
        getTypeFromLoaderType( neuron.type )) ,
      nsol::Neuron::UNDEFINED_FUNCTIONAL_TYPE
    );

    m_dataset->addNeuron( result );
  }
}

#endif

LoadingDialog::LoadingDialog( QWidget* p )
: QDialog( p , Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint )
{
  setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));

  auto layout = new QVBoxLayout( );
  m_progress = new QProgressBar( this );
  m_progress->setMinimumWidth( 590 );
  m_progress->setValue( 0 );
  m_progress->setFormat( "" );
  layout->addWidget( m_progress , 1 , Qt::AlignHCenter | Qt::AlignVCenter );
  layout->setMargin( 4 );
  setLayout( layout );

  setSizePolicy( QSizePolicy::MinimumExpanding ,
                 QSizePolicy::MinimumExpanding );
  setFixedSize( 600 , sizeHint().height( ));
}

void
LoadingDialog::progress( const QString& message , const unsigned int value )
{
  m_progress->setValue( value );

  if ( !message.isEmpty( ))
    m_progress->setFormat( tr( "%1 - %p%" ).arg( message ));
  else
    m_progress->setFormat( "%p%" );
}

void LoadingDialog::closeDialog( )
{
  close( );
  deleteLater( );
}
