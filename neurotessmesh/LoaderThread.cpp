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

// deps
#include <nsol/nsol.h>

#ifdef NEUROTESSMESH_USE_SIMIL

#include <simil/simil.h>

#endif

// Qt
#include <QString>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QIcon>

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

        emit progress( tr( "Loading Spikes" ) , 75 );
#ifdef NEUROTESSMESH_USE_SIMIL
        { // load the spikes data with SimIL, only for blueconfig.
          auto spikesData = new simil::SpikeData( m_fileName ,
                                                  simil::TDataType::TBlueConfig ,
                                                  m_target );
          spikesData->reduceDataToGIDS( );

          m_player = new simil::SpikesPlayer( );
          m_player->LoadData( spikesData );
        }
#endif
#else
        std::cerr << "Error: Brion support not built-in" << std::endl;
#endif
        break;

      case DataFileType::SWC:
        emit progress( tr( "Loading Neuron" ) , 50F );
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
      {
        H5Morphologies morphologies( m_fileName , "" );
        morphologies.load();


      }

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
  setFixedSize( 600 , sizeHint( ).height( ));
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
