/**
 * @file    MainWindow.cpp
 * @brief
 * @author  Juan José García <juanjose.garcia@urjc.es>,
 * Pablo Toharia <pablo.toharia@urjc.es>
 * @date    2015
 * @remarks Copyright (c) 2015 VG-Lab/URJC. All rights reserved.
 * Do not distribute without further notice.
 */


#include "MainWindow.h"
#include "LoaderThread.h"
#include <neurotessmesh/version.h>
#include <nsol/nsol.h>
#include <neurotessmesh/Scene.h>

#ifdef NEUROLOTS_USE_GMRVZEQ
#include <gmrvzeq/version.h>
#endif
#ifdef NEUROLOTS_USE_DEFLECT
#include <deflect/version.h>
#endif

#include <acuterecorder/acuterecorder.h>

#ifdef NEUROTESSMESH_USE_SIMIL

#include <qsimil/qsimil.h>
#include <simil/simil.h>

#endif

#include <QFileDialog>
#include <QInputDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardItemModel>
#include <QDir>

constexpr const char* POSITION_KEY = "positionData";

MainWindow::MainWindow( QWidget* parent_ , bool updateOnIdle_ )
  : QMainWindow( parent_ )
  , _lastOpenedFileName( "" )
  , _ui( new Ui::MainWindow )
  , _openGLWidget( nullptr )
  , _scene( nullptr )
  , _recorder( nullptr )
  , m_dataLoader{ nullptr }
{
  _ui->setupUi( this );

  auto recorderAction = RecorderUtils::recorderAction( );
  _ui->menuTools->insertAction( _ui->menuTools->actions( ).first( ) ,
                                recorderAction );
  _ui->toolBar->addAction( recorderAction );

  connect( recorderAction , SIGNAL( triggered( bool )) ,
           this , SLOT( openRecorder( )) );

  _ui->actionUpdateOnIdle->setChecked( updateOnIdle_ );
  _ui->actionShowFPSOnIdleUpdate->setChecked( false );

#ifdef NSOL_USE_BRION
  _ui->actionOpenBlueConfig->setEnabled( true );
#else
  _ui->actionOpenBlueConfig->setEnabled( false );
#endif

#ifdef NSOL_USE_QT5CORE
  _ui->actionOpenXMLScene->setEnabled( true );
#else
  _ui->actionOpenXMLScene->setEnabled( false );
#endif

  connect( _ui->actionQuit , SIGNAL( triggered( )) ,
           QApplication::instance( ) , SLOT( quit( )) );

  connect( _ui->actionAbout , SIGNAL( triggered( )) ,
           this , SLOT( showAbout( )) );

  _openGLWidget = new OpenGLWidget( );
  this->setCentralWidget( _openGLWidget );
  _openGLWidget->setMinimumSize( QSize( 100 , 100 ));

  _initExtractionDock( );
  _initConfigurationDock( );
  _initRenderOptionsDock( );
  _initPlayerDock( );

  if ( _openGLWidget->format( ).version( ).first < 4 )
  {
    std::cerr << "This application requires at least OpenGL 4.0" << std::endl;
    exit( -1 );
  }

  auto positionsMenu = new QMenu( );
  positionsMenu->setTitle( "Camera positions" );
  _ui->actionCamera_Positions->setMenu( positionsMenu );
}

MainWindow::~MainWindow( )
{
  delete _ui;
}

void MainWindow::init( const std::string& zeqSession_ )
{
  _openGLWidget->idleUpdate( _ui->actionUpdateOnIdle->isChecked( ));

  // @felix If session is empty should it connect to DEFAULT?
  if ( !zeqSession_.empty( ))
    _openGLWidget->setZeqSession( zeqSession_ );

  connect( _ui->actionHome , SIGNAL( triggered( )) ,
           this , SLOT( home( )) );

  connect( _ui->actionUpdateOnIdle , SIGNAL( triggered( )) ,
           _openGLWidget , SLOT( toggleUpdateOnIdle( )) );

  connect( _ui->actionShowFPSOnIdleUpdate , SIGNAL( triggered( )) ,
           _openGLWidget , SLOT( toggleShowFPS( )) );

  connect( _ui->actionWireframe , SIGNAL( triggered( )) ,
           _openGLWidget , SLOT( toggleWireframe( )) );

  connect( _ui->actionOpenBlueConfig , SIGNAL( triggered( )) ,
           this , SLOT( openBlueConfigThroughDialog( )) );

  connect( _ui->actionOpenXMLScene , SIGNAL( triggered( )) ,
           this , SLOT( openXMLSceneThroughDialog( )) );

  connect( _ui->actionOpenSWCFile , SIGNAL( triggered( )) ,
           this , SLOT( openSWCFileThroughDialog( )) );

  connect( _ui->actionOpenHDF5File , SIGNAL( triggered( )) ,
           this , SLOT( openHDF5FileThroughDialog( )) );

  connect( _radiusSlider , SIGNAL( valueChanged( int )) ,
           this , SLOT( onActionGenerate( int )) );

  connect( _extractButton , SIGNAL( clicked( )) ,
           _openGLWidget , SLOT( extractEditNeuronMesh( )) );

  connect( _lotSlider , SIGNAL( valueChanged( int )) ,
           _openGLWidget , SLOT( onLotValueChanged( int )) );
  _lotSlider->valueChanged( _lotSlider->value( ));

  connect( _distanceSlider , SIGNAL( valueChanged( int )) ,
           _openGLWidget , SLOT( onDistanceValueChanged( int )) );
  _distanceSlider->valueChanged( _distanceSlider->value( ));

  connect( _radioHomogeneous , SIGNAL( clicked( )) ,
           _openGLWidget , SLOT( onHomogeneousClicked( )) );

  connect( _radioLinear , SIGNAL( clicked( )) ,
           _openGLWidget , SLOT( onLinearClicked( )) );
  _radioLinear->clicked( );

  connect( _neuronRender , SIGNAL( currentIndexChanged( int )) ,
           _openGLWidget , SLOT( changeNeuronPiece( int )) );
  _neuronRender->currentIndexChanged( 1 );

  // TODO: @felix rework selection
//  connect( _selectedNeuronRender , SIGNAL( currentIndexChanged( int )) ,
//           _openGLWidget , SLOT( changeSelectedNeuronPiece( int )) );
//  _selectedNeuronRender->currentIndexChanged( 0 );

  connect( _ui->actionLoad_camera_positions , SIGNAL( triggered( bool )) ,
           this ,
           SLOT( loadCameraPositions( )) );

  connect( _ui->actionSave_camera_positions , SIGNAL( triggered( bool )) ,
           this ,
           SLOT( saveCameraPositions( )) );

  connect( _ui->actionAdd_camera_position , SIGNAL( triggered( bool )) , this ,
           SLOT( addCameraPosition( )) );

  connect( _ui->actionRemove_camera_position , SIGNAL( triggered( bool )) ,
           this ,
           SLOT( removeCameraPosition( )) );

  connect( _backGroundColor , SIGNAL( colorChanged( QColor )) ,
           _openGLWidget , SLOT( changeClearColor( QColor )) );

#ifndef NEUROTESSMESH_USE_SIMIL
  _ui->actionOpenHDF5File->setEnabled( false );
#endif
}

void MainWindow::showStatusBarMessage( const QString& message )
{
  _ui->statusbar->showMessage( message );
}

void MainWindow::openBlueConfig( const std::string& fileName ,
                                 const std::string& targetLabel )
{
  loadData( fileName , targetLabel ,
            neurotessmesh::LoaderThread::DataFileType::BlueConfig );
}

void MainWindow::openXMLScene( const std::string& fileName )
{
  loadData( fileName , std::string( ) ,
            neurotessmesh::LoaderThread::DataFileType::NsolScene );
}

void MainWindow::openSWCFile( const std::string& fileName )
{
  loadData( fileName , std::string( ) ,
            neurotessmesh::LoaderThread::DataFileType::SWC );
}

void MainWindow::openHDF5File( const std::string& fileName )
{
  loadData( fileName , std::string( ) ,
            neurotessmesh::LoaderThread::DataFileType::HDF5 );
}

std::set<int> MainWindow::updateNeuronList( )
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  std::set<int> usedColoringValues;

  const auto currentIdx = _neuronList->currentRow();
  _neuronList->clear( );

  for ( const auto& n: _scene->neurons() )
  {
    const auto type = n.second->morphologicalType();
    const auto function = n.second->functionalType();
    const auto layer = n.second->layer();
    const auto id = n.first;

    switch(_renderColoring->currentIndex())
    {
      case 1:
        usedColoringValues.insert(static_cast<int>(type));
        break;
      case 2:
        usedColoringValues.insert(layer);
        break;
      case 3:
        usedColoringValues.insert(static_cast<int>(function));
        break;
      case 0: // No value returned in selection coloring mode.
        /** pass-through **/
      default:
        break;
    }


    const auto color = _scene->neuronColor(id);
    const auto qcolor = QColor::fromRgbF(color[0], color[1], color[2]);
    auto itemText = QString::fromStdString(nsol::Neuron::typeToString(type));

    if(_neuronAdditionalText->isChecked() && (function + layer > 0))
    {
      QString layerText, functionText, separator;
      if(function > 0) functionText += QString::fromStdString(nsol::Neuron::functionToString(function));
      if(layer > 0) layerText += QString("layer ") + QString::number(static_cast<unsigned int>(layer));
      if(function > 0 && layer > 0) separator = ", ";

      itemText += QString(" (%1%2%3)").arg(functionText).arg(separator).arg(layerText);
    }

    auto item = new NeuronListItem(id, itemText, qcolor);
    item->setData(TEXT_ROLE, static_cast<int>(type));
    item->setData(ID_ROLE, id);
    item->setData(COLOR_ROLE, qcolor);
    _neuronList->addItem(item);
  }

  if(currentIdx != -1)
    _neuronList->setCurrentRow(currentIdx);

  QApplication::restoreOverrideCursor();

  return usedColoringValues;
}

void MainWindow::home( )
{
  _scene->home( );
  _openGLWidget->home( );
  _generateNeuritesLayout( );
  _extractButton->setEnabled( false );
  _somaGroup->hide( );
}

void MainWindow::openBlueConfigThroughDialog( )
{
#ifdef NSOL_USE_BRION

  QString path = QFileDialog::getOpenFileName(
    this , tr( "Open BlueConfig" ) , _lastOpenedFileName ,
    tr( "BlueConfig ( BlueConfig CircuitConfig);; All files (*)" ) ,
    nullptr , QFileDialog::DontUseNativeDialog );

  if ( path != QString( "" ))
  {
    bool ok;
    QString text = QInputDialog::getText(
      this , tr( "Please select target" ) ,
      tr( "Cell target:" ) , QLineEdit::Normal ,
      "Column" , &ok );
    if ( ok && !text.isEmpty( ))
    {
      std::string targetLabel = text.toStdString( );
      _lastOpenedFileName = QFileInfo( path ).path( );
      std::string fileName = path.toStdString( );
      openBlueConfig( fileName , targetLabel );
    }
  }
#endif
}

void MainWindow::openXMLSceneThroughDialog( )
{
#ifdef NSOL_USE_QT5CORE
  QString path = QFileDialog::getOpenFileName(
    this , tr( "Open XML Scene" ) , _lastOpenedFileName ,
    tr( "XML ( *.xml);; All files (*)" ) , nullptr ,
    QFileDialog::DontUseNativeDialog );

  if ( path != QString( "" ))
  {
    std::string fileName = path.toStdString( );
    openXMLScene( fileName );
  }
#endif

}

void MainWindow::openSWCFileThroughDialog( )
{
  QString path = QFileDialog::getOpenFileName(
    this , tr( "Open Swc File" ) , _lastOpenedFileName ,
    tr( "swc ( *.swc);; All files (*)" ) , nullptr ,
    QFileDialog::DontUseNativeDialog );

  if ( path != QString( "" ))
  {
    std::string fileName = path.toStdString( );
    openSWCFile( fileName );
  }
}

void MainWindow::openHDF5FileThroughDialog( )
{
  QString path = QFileDialog::getOpenFileName(
    this , tr( "Open HD5 File" ) , _lastOpenedFileName ,
    tr( "hdf5 ( *.hdf5 *.h5);; All files (*)" ) , nullptr ,
    QFileDialog::DontUseNativeDialog );

  if ( path != QString( "" ))
  {
    std::string fileName = path.toStdString( );
    openHDF5File( fileName );
  }
}

void MainWindow::showAbout( )
{

  QMessageBox::about(
    this , tr( "About " ) + tr( "NeuroTessMesh" ) ,
    tr( "<p><BIG><b>" ) + tr( "NeuroTessMesh" ) + tr( "</b></BIG><br><br>" ) +
    tr( "version " ) +
    tr( neurotessmesh::Version::getString( ).c_str( )) +
    tr( " (" ) +
    tr( std::to_string( neurotessmesh::Version::getRevision( )).c_str( )) +
    tr( ")" ) +
    tr( "<br><br>Using: " ) +
    tr( "<ul>" ) +
    tr( "<li>nsol " ) +
    tr( nsol::Version::getString( ).c_str( )) +
    tr( " (" ) +
    tr( std::to_string( nsol::Version::getRevision( )).c_str( )) +
    tr( ")</li> " ) +
    #ifdef NSOL_USE_BRION
    tr( "<li>Brion " ) +
    tr( brion::Version::getString( ).c_str( )) +
    tr( " (" ) +
    tr( std::to_string( brion::Version::getRevision( )).c_str( )) +
    tr( ")" ) +
    tr( "</li> " ) +
    #endif
    #ifdef NEUROTESSMESH_USE_SIMIL
    tr( "<li>Simil " ) +
    tr( simil::Version::getString( ).c_str( )) +
    tr( " (" ) +
    tr( std::to_string( simil::Version::getRevision( )).c_str( )) +
    tr( ")" ) +
    tr( "</li> " ) +
    #endif
    #ifdef NEUROLOTS_USE_ZEROEQ
    tr( "<li>ZEQ " ) +
    tr( zeroeq::Version::getString( ).c_str( )) +
    tr( " (" ) +
    tr( std::to_string( zeroeq::Version::getRevision( )).c_str( )) +
    tr( ")" ) +
    tr ( "</li> " ) +
    #endif
    #ifdef NEUROLOTS_USE_GMRVLEX
    tr( "<li>gmrvzeq " ) +
    tr( gmrvlex::Version::getString( ).c_str( )) +
    tr( " (" ) +
    tr( std::to_string( gmrvlex::Version::getRevision( )).c_str( )) +
    tr( ")" ) +
    tr ( "</li> " ) +
    #endif
    #ifdef NEUROLOTS_USE_DEFLECT
    tr( "<li>Deflect " ) +
    tr( deflect::Version::getString( ).c_str( )) +
    tr( " (" ) +
    tr( std::to_string( deflect::Version::getRevision( )).c_str( )) +
    tr( ")" ) +
    tr ( "</li> " ) +
    #endif
    tr( "<li>AcuteRecorder " ) +
    tr( acuterecorder::Version::getString( ).c_str( )) +
    tr( " (" ) +
    tr( std::to_string( acuterecorder::Version::getRevision( )).c_str( )) +
    tr( ")" ) +
    tr( "</li> " ) +

    tr( "</ul>" ) +
    tr( "<br>VG-Lab - Universidad Rey Juan Carlos<br>"
        "<a href=www.vg-lab.es>www.vg-lab.es</a><br>"
        "<a href='mailto:dev@vg-lab.es'>dev@vg-lab.es</a><br><br>"
        "<br>(C) 2015-2022. Universidad Rey Juan Carlos<br><br>"
        "<img src=':/icons/rsc/logoVGLab.png' >&nbsp;&nbsp;&nbsp;&nbsp;"
        "<img src=':/icons/rsc/logoURJC.png' ><br><br> "
        "</p>"
        "" )
  );
}

void MainWindow::openRecorder( )
{
  auto action = qobject_cast< QAction* >( sender( ));

  // The button stops the recorder if found.
  if ( _recorder )
  {
    if ( action ) action->setDisabled( true );

    RecorderUtils::stopAndWait( _recorder , this );

    // Recorder will be deleted after finishing.
    _recorder = nullptr;
    return;
  }

  RSWParameters params;
  params.widgetsToRecord.emplace_back( "Viewport" , _openGLWidget );
  params.widgetsToRecord.emplace_back( "Main Widget" , this );
  params.includeScreens = false;

  if ( !_ui->actionAdvancedRecorderOptions->isChecked( ))
  {
    params.showWorker = false;
    params.showWidgetSourceMode = false;
    params.showSourceParameters = false;
  }

  RecorderDialog dialog( nullptr , params , true );
  dialog.setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));
  dialog.setFixedSize( 800 , 600 );
  if ( dialog.exec( ) == QDialog::Accepted )
  {
    _recorder = dialog.getRecorder( );
    connect( _recorder , SIGNAL( finished( )) ,
             _recorder , SLOT( deleteLater( )) );
    connect( _recorder , SIGNAL( finished( )) ,
             this , SLOT( finishRecording( )) );
    if ( action ) action->setChecked( true );
  }
  else
  {
    if ( action ) action->setChecked( false );
  }
}

void MainWindow::updateExtractMeshDock( void )
{
  if ( _ui->actionEditSave->isChecked( ))
    _extractMeshDock->show( );
  else
    _extractMeshDock->close( );
}

void MainWindow::updateConfigurationDock( void )
{
  if ( _ui->actionConfiguration->isChecked( ))
    _configurationDock->show( );
  else
    _configurationDock->close( );
}

void MainWindow::updateRenderOptionsDock( void )
{
  if ( _ui->actionRenderOptions->isChecked( ))
    _renderOptionsDock->show( );
  else
    _renderOptionsDock->close( );
}

void MainWindow::updatePlayerOptionsDock( )
{
#ifdef NEUROTESSMESH_USE_SIMIL
  auto playerWidget = qobject_cast< qsimil::QSimControlWidget* >(
    _playerDock->widget( ));
  bool enabled = false;
  if ( playerWidget )
  {
    auto player = dynamic_cast<simil::SpikesPlayer*>(playerWidget->getSimulationPlayer( ));
    enabled = player && !player->spikes( ).empty( );
  }

  _playerDock->setEnabled( enabled );
  if ( _ui->actionSimulation_player_options->isChecked( ))
    _playerDock->show( );
  else
    _playerDock->hide( );
#else
  _ui->actionSimulation_player_options->setEnabled(false);
  _playerDock->setVisible(false);
#endif
}

void MainWindow::onListClicked( QListWidgetItem* item )
{
  if(!item) return;

  const unsigned int id = item->data(ID_ROLE).toUInt();

  _scene->setNeuronToEdit( id );
  _openGLWidget->update( );
  _generateNeuritesLayout( );
  _extractButton->setEnabled( true );
  _somaGroup->show( );
}

void MainWindow::onActionGenerate( int /*value_*/ )
{
  float alphaRadius = static_cast<float>(_radiusSlider->value()) / 100.0f;
  std::vector< float > alphaNeurites;

  for ( auto& _neuriteSlider: _neuriteSliders )
  {
    alphaNeurites.push_back(
      static_cast<float>(_neuriteSlider->value( )) / 100.0f );
  }

  _openGLWidget->makeCurrent( );
  _openGLWidget->update( );
  _scene->regenerateEditNeuronMesh( alphaRadius , alphaNeurites );
}

void MainWindow::finishRecording( )
{
  auto actionRecorder = _ui->menuTools->actions( ).first( );
  if ( actionRecorder )
  {
    actionRecorder->setEnabled( true );
    actionRecorder->setChecked( false );
  }
}

void MainWindow::loadCameraPositions( )
{
  const QString title = "Load camera positions";

  auto actions = _ui->actionCamera_Positions->menu( )->actions( );
  const auto numActions = actions.size( );
  if ( numActions > 0 )
  {
    const auto warnText = tr( "Loading new camera positions will remove"
                              " %1 existing position%2. Are you sure?" ).arg(
      numActions ).arg(
      numActions > 1 ? "s" : "" );
    if ( QMessageBox::Ok != QMessageBox::warning( this , title , warnText ,
                                                  QMessageBox::Cancel |
                                                  QMessageBox::Ok ))
      return;
  }

  const QString nameFilter = "Camera positions (*.json)";
  QDir directory;

  if ( _lastOpenedFileName.isEmpty( ))
    directory = QDir::home( );
  else
    directory = QFileInfo( _lastOpenedFileName ).dir( );

  QFileDialog fDialog( this );
  fDialog.setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));
  fDialog.setWindowTitle( title );
  fDialog.setAcceptMode( QFileDialog::AcceptMode::AcceptOpen );
  fDialog.setDefaultSuffix( "json" );
  fDialog.setDirectory( directory );
  fDialog.setOption( QFileDialog::Option::DontUseNativeDialog , true );
  fDialog.setFileMode( QFileDialog::FileMode::ExistingFile );
  fDialog.setNameFilters( QStringList{ nameFilter } );
  fDialog.setNameFilter( nameFilter );

  if ( fDialog.exec( ) != QFileDialog::Accepted )
    return;

  if ( fDialog.selectedFiles( ).empty( )) return;

  auto file = fDialog.selectedFiles( ).first( );

  QFile posFile{ file };
  if ( !posFile.open( QIODevice::ReadOnly | QIODevice::Text ))
  {
    const QString errorText = tr( "Unable to open: %1" ).arg( file );
    QMessageBox::critical( this , title , errorText );
    return;
  }

  const auto contents = posFile.readAll( );
  QJsonParseError parserError;

  const auto jsonDoc = QJsonDocument::fromJson( contents , &parserError );
  if ( jsonDoc.isNull( ) || !jsonDoc.isObject( ))
  {
    const auto message = tr(
      "Couldn't read the contents of %1 or parsing error." ).arg( file );

    QMessageBox msgbox{ this };
    msgbox.setWindowTitle( title );
    msgbox.setIcon( QMessageBox::Icon::Critical );
    msgbox.setText( message );
    msgbox.setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));
    msgbox.setStandardButtons( QMessageBox::Ok );
    msgbox.setDetailedText( parserError.errorString( ));
    msgbox.exec( );
    return;
  }

  const auto jsonObj = jsonDoc.object( );
  if ( jsonObj.isEmpty( ))
  {
    const auto message = tr( "Error parsing the contents of %1." ).arg( file );

    QMessageBox msgbox{ this };
    msgbox.setWindowTitle( title );
    msgbox.setIcon( QMessageBox::Icon::Critical );
    msgbox.setText( message );
    msgbox.setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));
    msgbox.setStandardButtons( QMessageBox::Ok );
    msgbox.exec( );
    return;
  }

  const QFileInfo currentFile{ _lastOpenedFileName };
  const QString jsonPositionsFile = jsonObj.value( "filename" ).toString( );
  if ( !jsonPositionsFile.isEmpty( ) &&
       jsonPositionsFile.compare( currentFile.fileName( ) ,
                                  Qt::CaseInsensitive ) != 0 )
  {
    const auto message = tr( "This positions are from file '%1'. Current file"
                             " is '%2'. Do you want to continue?" )
      .arg( jsonPositionsFile )
      .arg( currentFile.fileName( ));

    QMessageBox msgbox{ this };
    msgbox.setWindowTitle( title );
    msgbox.setIcon( QMessageBox::Icon::Question );
    msgbox.setText( message );
    msgbox.setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));
    msgbox.setStandardButtons( QMessageBox::Cancel | QMessageBox::Ok );
    msgbox.setDefaultButton( QMessageBox::Ok );

    if ( QMessageBox::Ok != msgbox.exec( ))
      return;
  }

  // Clear existing actions before entering new ones.
  for ( auto action: actions )
  {
    _ui->actionCamera_Positions->menu( )->removeAction( action );
    delete action;
  }

  const auto jsonPositions = jsonObj.value( "positions" ).toArray( );

  auto createPosition = [ this ]( const QJsonValue& v )
  {
    const auto o = v.toObject( );

    const auto name = o.value( "name" ).toString( );
    const auto position = o.value( "position" ).toString( );
    const auto radius = o.value( "radius" ).toString( );
    const auto rotation = o.value( "rotation" ).toString( );

    auto action = new QAction( name );
    action->setProperty( POSITION_KEY ,
                         position + ";" + radius + ";" + rotation );

    connect( action , SIGNAL( triggered( bool )) ,
             this , SLOT( applyCameraPosition( )) );

    _ui->actionCamera_Positions->menu( )->addAction( action );
  };
  std::for_each( jsonPositions.constBegin( ) , jsonPositions.constEnd( ) ,
                 createPosition );

  const bool positionsExist = !_ui->actionCamera_Positions->menu( )->actions( ).isEmpty( );
  _ui->actionSave_camera_positions->setEnabled( positionsExist );
  _ui->actionRemove_camera_position->setEnabled( positionsExist );
  _ui->actionCamera_Positions->setEnabled( positionsExist );
}

void MainWindow::saveCameraPositions( )
{
  const QString nameFilter = "Camera positions (*.json)";
  QDir directory;
  QString filename;

  if ( _lastOpenedFileName.isEmpty( ))
  {
    directory = QDir::home( );
    filename = "positions.json";
  }
  else
  {
    QFileInfo fi( _lastOpenedFileName );
    directory = fi.dir( );
    filename = QString( "%1_positions.json" ).arg( fi.baseName( ));
  }

  QFileDialog fDialog( this );
  fDialog.setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));
  fDialog.setWindowTitle( "Save camera positions" );
  fDialog.setAcceptMode( QFileDialog::AcceptMode::AcceptSave );
  fDialog.setDefaultSuffix( "json" );
  fDialog.selectFile( filename );
  fDialog.setDirectory( directory );
  fDialog.setOption( QFileDialog::Option::DontUseNativeDialog , true );
  fDialog.setOption( QFileDialog::Option::DontConfirmOverwrite , false );
  fDialog.setFileMode( QFileDialog::FileMode::AnyFile );
  fDialog.setNameFilters( QStringList{ nameFilter } );
  fDialog.setNameFilter( nameFilter );

  if ( fDialog.exec( ) != QFileDialog::Accepted )
    return;

  if ( fDialog.selectedFiles( ).empty( )) return;

  filename = fDialog.selectedFiles( ).first( );

  QFile wFile{ filename };
  if ( !wFile.open(
    QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate ))
  {
    const auto message = tr( "Unable to open file %1 for writing." ).arg(
      filename );

    QMessageBox msgbox{ this };
    msgbox.setWindowTitle( tr( "Save camera positions" ));
    msgbox.setIcon( QMessageBox::Icon::Critical );
    msgbox.setText( message );
    msgbox.setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));
    msgbox.setDefaultButton( QMessageBox::Ok );
    msgbox.exec( );
    return;
  }

  QApplication::setOverrideCursor( Qt::WaitCursor );

  const auto actions = _ui->actionCamera_Positions->menu( )->actions( );

  QJsonArray positionsObjs;

  auto insertPosition = [ &positionsObjs ]( const QAction* a )
  {
    if ( !a ) return;
    const auto posData = a->property( POSITION_KEY ).toString( );
    const auto parts = posData.split( ";" );
    Q_ASSERT( parts.size( ) == 3 );
    const auto& position = parts.first( );
    const auto& radius = parts.at( 1 );
    const auto& rotation = parts.last( );

    QJsonObject positionObj;
    positionObj.insert( "name" , a->text( ));
    positionObj.insert( "position" , position );
    positionObj.insert( "radius" , radius );
    positionObj.insert( "rotation" , rotation );

    positionsObjs << positionObj;
  };
  std::for_each( actions.cbegin( ) , actions.cend( ) , insertPosition );

  QJsonObject obj;
  obj.insert( "filename" , QFileInfo{ _lastOpenedFileName }.fileName( ));
  obj.insert( "positions" , positionsObjs );

  QJsonDocument doc{ obj };
  wFile.write( doc.toJson( ));

  QApplication::restoreOverrideCursor( );

  if ( wFile.error( ) != QFile::NoError )
  {
    const auto message = tr( "Error saving file %1." ).arg( filename );

    QMessageBox msgbox{ this };
    msgbox.setWindowTitle( tr( "Save camera positions" ));
    msgbox.setIcon( QMessageBox::Icon::Critical );
    msgbox.setText( message );
    msgbox.setDetailedText( wFile.errorString( ));
    msgbox.setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));
    msgbox.setDefaultButton( QMessageBox::Ok );
    msgbox.exec( );
  }

  wFile.flush( );
  wFile.close( );
}

void MainWindow::addCameraPosition( )
{
  QStringList items;

  auto actions = _ui->actionCamera_Positions->menu( )->actions( );
  auto insertItemName = [ &items ]( const QAction* a )
  { items << a->text( ); };
  std::for_each( actions.cbegin( ) , actions.cend( ) , insertItemName );

  const QString title = tr( "Add camera position" );

  bool ok = false;
  QString name;
  while ( !ok || name.isEmpty( ))
  {
    name = QInputDialog::getText( this , title , tr( "Position name:" ) ,
                                  QLineEdit::Normal , tr( "New position" ) ,
                                  &ok );

    if(!ok) return;

    if ( ok && !name.isEmpty( ))
    {
      QString tempName( name );
      int collision = 0;
      while ( items.contains( tempName , Qt::CaseInsensitive ))
      {
        ++collision;
        tempName = tr( "%1 (%2)" ).arg( name ).arg( collision );
      }

      name = tempName;
    }
  }

  auto action = new QAction( name );

  const auto position = _openGLWidget->cameraPosition( );
  action->setProperty( POSITION_KEY , position.toString( ));

  connect( action , SIGNAL( triggered( bool )) ,
           this , SLOT( applyCameraPosition( )) );
  _ui->actionCamera_Positions->menu( )->addAction( action );
  _ui->actionCamera_Positions->setEnabled( true );
  _ui->actionSave_camera_positions->setEnabled( true );
  _ui->actionRemove_camera_position->setEnabled( true );
}

void MainWindow::removeCameraPosition( )
{
  bool ok = false;
  QStringList items;

  auto actions = _ui->actionCamera_Positions->menu( )->actions( );
  auto insertItemName = [ &items ]( const QAction* a )
  { items << a->text( ); };
  std::for_each( actions.cbegin( ) , actions.cend( ) , insertItemName );

  auto item = QInputDialog::getItem( this , tr( "Remove camera position" ) ,
                                     tr( "Position name:" ) , items , 0 ,
                                     false , &ok );
  if ( ok && !item.isEmpty( ))
  {
    auto actionOfName = [ &item ]( const QAction* a )
    { return a->text( ) == item; };
    const auto it = std::find_if( actions.cbegin( ) , actions.cend( ) ,
                                  actionOfName );
    auto distance = std::distance( actions.cbegin( ) , it );
    auto action = actions.at( static_cast<int>(distance));
    _ui->actionCamera_Positions->menu( )->removeAction( action );
    delete action;

    const auto enabled = actions.size( ) > 1;
    _ui->actionRemove_camera_position->setEnabled( enabled );
    _ui->actionSave_camera_positions->setEnabled( enabled );
    _ui->actionCamera_Positions->setEnabled( enabled );
  }
}

void MainWindow::applyCameraPosition( )
{
  auto action = qobject_cast< QAction* >( sender( ));
  if ( action )
  {
    auto positionString = action->property( POSITION_KEY ).toString( );
    CameraPosition position( positionString );
    _scene->cameraPosition( position.position , position.radius ,
                            position.rotation );
  }
}

void MainWindow::_generateNeuritesLayout( )
{
  const unsigned int numDendrites = _scene->numEditMorphologyNeurites( );

  _neuriteSliders.clear( );

  QLayoutItem* child;
  while (( child = _neuritesLayout->takeAt( 0 )) != 0 )
  {
    delete child->widget( );
  }

  QSlider* _neuriteSlider;
  for ( unsigned int i = 0; i < numDendrites; i++ )
  {
    _neuriteSlider = new QSlider( Qt::Horizontal );
    _neuriteSlider->setMinimum( 0 );
    _neuriteSlider->setMaximum( 200 );
    _neuriteSlider->setValue( 100 );
    _neuriteSlider->setToolTip(
      "Scales the distance between the position of the first tracing point of\n"
      "neurite n and the surface of the initial sphere used to generate the\n"
      "soma [0-2], 0 means that the first tracing point is placed on the\n"
      "surface and 2 means that the point is placed at a distance twice as\n"
      "big as the initial distance between the point and the surface." );

    _neuritesLayout->addWidget( new QLabel( QString( "Neurite " ) +
                                            QString::number( i ) +
                                            QString( " factor" )));

    _neuritesLayout->addWidget( _neuriteSlider );
    _neuriteSliders.push_back( _neuriteSlider );
    connect( _neuriteSlider , SIGNAL( valueChanged( int )) ,
             this , SLOT( onActionGenerate( int )) );
  }
  _radiusSlider->setValue( 100 );
}

void MainWindow::_initExtractionDock( )
{
  _extractMeshDock = new QDockWidget( );
  this->addDockWidget( Qt::DockWidgetAreas::enum_type::RightDockWidgetArea ,
                       _extractMeshDock , Qt::Vertical );
  _extractMeshDock->setSizePolicy( QSizePolicy::MinimumExpanding ,
                                   QSizePolicy::MinimumExpanding );

  _extractMeshDock->setFeatures( QDockWidget::DockWidgetClosable |
                                 QDockWidget::DockWidgetMovable |
                                 QDockWidget::DockWidgetFloatable );
  _extractMeshDock->setWindowTitle( QString( "Edit And Save" ));
  _extractMeshDock->setMinimumWidth( 250 );

  _extractMeshDock->close( );

  auto newWidget = new QWidget( );
  _extractMeshDock->setWidget( newWidget );

  auto _meshDockLayout = new QVBoxLayout( );
  _meshDockLayout->setAlignment( Qt::AlignTop );
  newWidget->setLayout( _meshDockLayout );

  // Checkbox
  _neuronAdditionalText = new QCheckBox("Show additional information");
  _neuronAdditionalText->setChecked(false);
  _meshDockLayout->addWidget(_neuronAdditionalText);
  connect(_neuronAdditionalText, SIGNAL(stateChanged(int)), this, SLOT(updateNeuronList()));

  //Neurons group
  auto* _neuronsGroup = new QGroupBox( QString( "Select Neuron" ));
  auto* _neuronsLayout = new QVBoxLayout( );
  _neuronsGroup->setLayout( _neuronsLayout );
  _meshDockLayout->addWidget( _neuronsGroup );

  _neuronList = new QListWidget( );
  _neuronList->setSortingEnabled(true);
  _neuronsLayout->addWidget( new QLabel( QString( "Neurons" )));
  _neuronsLayout->addWidget( _neuronList );

  // Soma reconstruction group
  _somaGroup = new QGroupBox( QString( "Parameters" ));
  auto* _somaGroupLayout = new QVBoxLayout( );
  _somaGroup->setLayout( _somaGroupLayout );
  _meshDockLayout->addWidget( _somaGroup );
  _somaGroup->hide( );

  _radiusSlider = new QSlider( Qt::Horizontal );
  _radiusSlider->setMinimum( 25 );
  _radiusSlider->setMaximum( 100 );
  _radiusSlider->setValue( 100 );
  _radiusSlider->setToolTip(
    "Scales the radius of the initial sphere used to generate the soma. [0-1]."
  );

  _somaGroupLayout->addWidget( new QLabel( QString( "Radius factor" )));
  _somaGroupLayout->addWidget( _radiusSlider );

  auto* _neuritesArea = new QScrollArea( );
  _neuritesArea->setSizePolicy( QSizePolicy::MinimumExpanding ,
                                QSizePolicy::Expanding );
  _neuritesArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
  _neuritesArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
  _neuritesArea->setWidgetResizable( true );
  _neuritesArea->setFrameShape( QFrame::NoFrame );
  _somaGroupLayout->addWidget( _neuritesArea );
  auto* _neuritesWidget = new QWidget( );
  _neuritesArea->setWidget( _neuritesWidget );
  _neuritesLayout = new QVBoxLayout( );
  _neuritesWidget->setLayout( _neuritesLayout );

  _extractButton = new QPushButton( QString( "Save" ));
  _extractButton->setSizePolicy( QSizePolicy::Fixed ,
                                 QSizePolicy::Fixed );
  _extractButton->setEnabled( false );
  _meshDockLayout->addWidget( _extractButton );

  connect( _neuronList , SIGNAL( itemClicked( QListWidgetItem * )) ,
           this , SLOT( onListClicked( QListWidgetItem * )) );

  connect( _extractMeshDock->toggleViewAction( ) , SIGNAL( toggled( bool )) ,
           _ui->actionEditSave , SLOT( setChecked( bool )) );

  connect( _ui->actionEditSave , SIGNAL( triggered( )) ,
           this , SLOT( updateExtractMeshDock( )) );

}

void MainWindow::_initConfigurationDock( )
{
  _configurationDock = new QDockWidget( );
  this->addDockWidget( Qt::DockWidgetAreas::enum_type::LeftDockWidgetArea ,
                       _configurationDock , Qt::Vertical );
  _configurationDock->setSizePolicy( QSizePolicy::MinimumExpanding ,
                                     QSizePolicy::MinimumExpanding );

  _configurationDock->setFeatures( QDockWidget::DockWidgetClosable |
                                   QDockWidget::DockWidgetMovable |
                                   QDockWidget::DockWidgetFloatable );
  _configurationDock->setWindowTitle( QString( "Configuration" ));
  _configurationDock->setMinimumWidth( 250 );

  _configurationDock->close( );

  auto* newWidget = new QWidget( );
  _configurationDock->setWidget( newWidget );

  auto* _configDockLayout = new QVBoxLayout( );
  _configDockLayout->setAlignment( Qt::AlignTop );
  newWidget->setLayout( _configDockLayout );

  auto* tessParamsGroup = new QGroupBox( QString( "Tessellation params" ));
  tessParamsGroup->setSizePolicy( QSizePolicy::Fixed ,
                                  QSizePolicy::Fixed );
  _configDockLayout->addWidget( tessParamsGroup );
  auto* vbox = new QVBoxLayout;
  tessParamsGroup->setLayout( vbox );

  _lotSlider = new QSlider( Qt::Horizontal );
  _lotSlider->setMinimum( 1 );
  _lotSlider->setMaximum( 30 );
  _lotSlider->setValue( 4 );
  _lotSlider->setToolTip(
    "Maximum level of subdivisions for the visualization [1-30]."
  );
  vbox->addWidget(
    new QLabel( QString( "Subdivision Level" )));
  vbox->addWidget( _lotSlider );

  _distanceSlider = new QSlider( Qt::Horizontal );
  _distanceSlider->setMinimum( 0 );
  _distanceSlider->setMaximum( 1000 );
  _distanceSlider->setValue( 10 );
  _distanceSlider->setToolTip(
    "Further distance to which the subdivision is applied [0-1], being 1\n"
    "the camera maximum visibility distance." );
  vbox->addWidget(
    new QLabel( QString( "Distance threshold" )));
  vbox->addWidget( _distanceSlider );

  // Radio Buttons Group
  auto tessMethodGroup = new QGroupBox( QString( "Tessellation criteria" ));
  _configDockLayout->addWidget( tessMethodGroup );
  vbox = new QVBoxLayout;
  tessMethodGroup->setLayout( vbox );

  _radioHomogeneous = new QRadioButton( QString( "Homogeneous" ));
  vbox->addWidget( _radioHomogeneous );
  _radioLinear = new QRadioButton( QString( "Camera Distance" ));
  vbox->addWidget( _radioLinear );

  _radioLinear->setChecked( true );

  connect( _radioLinear , SIGNAL( toggled( bool )) ,
           _distanceSlider , SLOT( setEnabled( bool )) );

  connect( _configurationDock->toggleViewAction( ) , SIGNAL( toggled( bool )) ,
           _ui->actionConfiguration , SLOT( setChecked( bool )) );

  connect( _ui->actionConfiguration , SIGNAL( triggered( )) ,
           this , SLOT( updateConfigurationDock( )) );
}

void MainWindow::_initRenderOptionsDock( )
{
  _renderOptionsDock = new QDockWidget( );
  this->addDockWidget( Qt::DockWidgetAreas::enum_type::LeftDockWidgetArea ,
                       _renderOptionsDock , Qt::Vertical );
  _renderOptionsDock->setSizePolicy( QSizePolicy::MinimumExpanding ,
                                     QSizePolicy::MinimumExpanding );
  _renderOptionsDock->setFeatures( QDockWidget::DockWidgetClosable |
                                   QDockWidget::DockWidgetMovable |
                                   QDockWidget::DockWidgetFloatable );
  _renderOptionsDock->setWindowTitle( QString( "Render Options" ));
  _renderOptionsDock->setMinimumWidth( 250 );

  _renderOptionsDock->close( );

  auto newWidget = new QWidget( );
  _renderOptionsDock->setWidget( newWidget );

  auto roDockLayout = new QVBoxLayout( );
  roDockLayout->setAlignment( Qt::AlignTop );
  newWidget->setLayout( roDockLayout );

  auto colorGroup = new QGroupBox( QString( "Color" ));
  roDockLayout->addWidget( colorGroup );
  auto gridbox = new QGridLayout;
  colorGroup->setLayout( gridbox );

  gridbox->addWidget( new QLabel( QString( "Background color" )) , 0 , 0 );
  _backGroundColor = new ColorSelectionWidget( this );
  _backGroundColor->color(QColor(255,255,255));
  gridbox->addWidget( _backGroundColor , 0 , 1 );

  auto renderGroup = new QGroupBox( QString( "Render piece selection" ));
  roDockLayout->addWidget( renderGroup );
  auto vbox = new QVBoxLayout;
  renderGroup->setLayout( vbox );

  _neuronRender = new QComboBox( );
  _neuronRender->setSizePolicy( QSizePolicy( QSizePolicy::Fixed ,
                                             QSizePolicy::Fixed ));
  auto hLay = new QHBoxLayout();
  hLay->addWidget(new QLabel( QString( "Neuron" )));
  hLay->addWidget(_neuronRender);
  vbox->addLayout(hLay);
  _neuronRender->addItem( QString( "all" ));
  _neuronRender->addItem( QString( "soma" ));
  _neuronRender->addItem( QString( "neurites" ));

  hLay = new QHBoxLayout();
  hLay->addWidget(new QLabel( QString( "Selected Neuron" )));
  _selectedNeuronRender = new QComboBox( );
  _selectedNeuronRender->setSizePolicy( QSizePolicy( QSizePolicy::Fixed ,
                                                     QSizePolicy::Fixed ));
  hLay->addWidget( _selectedNeuronRender );
  vbox->addLayout(hLay);
  _selectedNeuronRender->addItem( QString( "all" ));
  _selectedNeuronRender->addItem( QString( "soma" ));
  _selectedNeuronRender->addItem( QString( "neurites" ));

  connect( _renderOptionsDock->toggleViewAction( ) , SIGNAL( toggled( bool )) ,
           _ui->actionRenderOptions , SLOT( setChecked( bool )) );

  connect( _ui->actionRenderOptions , SIGNAL( triggered( )) ,
           this , SLOT( updateRenderOptionsDock( )) );

  auto* coloringGroup = new QGroupBox( QString( "Render coloring" ));
  roDockLayout->addWidget( coloringGroup );
  vbox = new QVBoxLayout;
  coloringGroup->setLayout(vbox);

  _renderColoring = new QComboBox(coloringGroup);
  _renderColoring->setSizePolicy( QSizePolicy( QSizePolicy::Fixed ,
                                               QSizePolicy::Fixed ));
  _renderColoring->addItem("Selection");
  _renderColoring->addItem("Morphology");
  _renderColoring->addItem("Layer");
  _renderColoring->addItem("Function");

  hLay = new QHBoxLayout();
  hLay->addWidget( new QLabel( QString( "Color by" )));
  hLay->addWidget( _renderColoring );
  vbox->addLayout(hLay);
  auto line = new QFrame(coloringGroup);
  line->setObjectName(QString::fromUtf8("line"));
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  vbox->addWidget(line);

  _colorLayout = new QGridLayout();
  vbox->addLayout(_colorLayout);
  connect(_renderColoring, SIGNAL(currentIndexChanged(int)),
           this,           SLOT(onColoringChanged(int)));

  onColoringChanged(0);
  coloringGroup->setEnabled(false);
}

void MainWindow::onColoringChanged(int index)
{
  if(!_scene) return;

  QApplication::setOverrideCursor(Qt::WaitCursor);

  _scene->coloringMode(static_cast<neurotessmesh::Scene::TColoringMode>(index));
  const auto usedColors = updateNeuronList();

  // Clear color widgets and refill with current colors.
  std::function<void(QLayout*)> clearLayout = [&](QLayout *layout)
  {
     if (!layout) return;
     while(auto item = layout->takeAt(0))
     {
        delete item->widget();
        clearLayout(item->layout());
     }

     if(layout != dynamic_cast<QLayout*>(_colorLayout))
       delete layout;
  };

  clearLayout(_colorLayout);

  const char *TYPE = "type";

  switch(_renderColoring->currentIndex())
  {
    case 0: // SELECTION
      {
        _colorLayout->addWidget(new QLabel("Unselected"), 0,0);
        auto widget = new ColorSelectionWidget();
        auto col = _scene->color(0);
        widget->color(QColor::fromRgbF(col[0], col[1], col[2]));
        widget->setProperty(TYPE, 0);
        connect(widget, SIGNAL( colorChanged( QColor )) ,
                this, SLOT( changeNeuronColor( QColor )) );

        _colorLayout->addWidget(widget, 0, 1);
        _colorLayout->addWidget(new QLabel("Selected"), 1,0);
        widget = new ColorSelectionWidget();
        col = _scene->color(1);
        widget->color(QColor::fromRgbF(col[0], col[1], col[2]));
        widget->setProperty(TYPE, 1);
        _colorLayout->addWidget(widget, 1, 1);
      }
      break;
    case 1: // MORPHOLOGY
      {
        for(int i = 0, row = 0; i <= nsol::Neuron::TMorphologicalType::DEEP_CEREBELLAR_NUCLEI; ++i)
        {
          if(usedColors.count(i) == 0) continue;
          auto name = nsol::Neuron::typeToString(static_cast<nsol::Neuron::TMorphologicalType>(i));
          _colorLayout->addWidget(new QLabel(QString::fromStdString(name)), row,0);
          auto widget = new ColorSelectionWidget();
          auto col = _scene->color(i);
          widget->color(QColor::fromRgbF(col[0], col[1], col[2]));
          widget->setProperty(TYPE, i);
          connect(widget, SIGNAL( colorChanged( QColor )) ,
                  this, SLOT( changeNeuronColor( QColor )) );
          _colorLayout->addWidget(widget, row++, 1);
        }
      }
      break;
    case 2: // LAYER
      {
        for(int i = 1, row = 0; i <= 6; ++i)
        {
          if(usedColors.count(i) == 0) continue;
          _colorLayout->addWidget(new QLabel(QString("Layer %1").arg(i)), row,0);
          auto widget = new ColorSelectionWidget();
          auto col = _scene->color(i);
          widget->color(QColor::fromRgbF(col[0], col[1], col[2]));
          widget->setProperty(TYPE, i);
          connect(widget, SIGNAL( colorChanged( QColor )) ,
                  this, SLOT( changeNeuronColor( QColor )) );
          _colorLayout->addWidget(widget, row++, 1);
        }
      }
      break;
    case 3: // FUNCTION
      {
        for(int i = 0, row = 0; i <= nsol::Neuron::TFunctionalType::EXCITATORY; ++i)
        {
          if(usedColors.count(i) == 0) continue;
          auto name = nsol::Neuron::functionToString(static_cast<nsol::Neuron::TFunctionalType>(i));
          _colorLayout->addWidget(new QLabel(QString::fromStdString(name)), row,0);
          auto widget = new ColorSelectionWidget();
          auto col = _scene->color(i);
          widget->color(QColor::fromRgbF(col[0], col[1], col[2]));
          widget->setProperty(TYPE, i);
          connect(widget, SIGNAL( colorChanged( QColor )) ,
                  this, SLOT( changeNeuronColor( QColor )) );
          _colorLayout->addWidget(widget, row++, 1);
        }
      }
      break;
    default:
      break;
  }

  _openGLWidget->repaint();
  QApplication::restoreOverrideCursor();
}

void MainWindow::closeEvent( QCloseEvent* e )
{
  if ( _recorder )
  {
    QMessageBox msgBox( this );
    msgBox.setWindowTitle( tr( "Exit NeuroTessMesh" ));
    msgBox.setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));
    msgBox.setText( tr(
      "A recording is being made. Do you really want to exit NeuroTessMesh?" ));
    msgBox.setStandardButtons( QMessageBox::Cancel | QMessageBox::Yes );

    if ( msgBox.exec( ) != QMessageBox::Yes )
    {
      e->ignore( );
      return;
    }

    RecorderUtils::stopAndWait( _recorder , this );
    _recorder = nullptr;
  }

  QMainWindow::closeEvent( e );
}

void MainWindow::_initPlayerDock( )
{
  _playerDock = new QDockWidget( );
  this->addDockWidget( Qt::DockWidgetAreas::enum_type::BottomDockWidgetArea ,
                       _playerDock , Qt::Horizontal );
  _playerDock->setSizePolicy( QSizePolicy::MinimumExpanding ,
                              QSizePolicy::Expanding );

  _playerDock->setFeatures( QDockWidget::DockWidgetClosable |
                            QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetFloatable );
  _playerDock->setAllowedAreas(
    Qt::DockWidgetAreas::enum_type::BottomDockWidgetArea |
    Qt::DockWidgetAreas::enum_type::TopDockWidgetArea );
  _playerDock->setWindowTitle( QString( "Player Options" ));
  _playerDock->show( );
  _playerDock->close( );

#ifdef NEUROTESSMESH_USE_SIMIL
  auto playerControls = new qsimil::QSimControlWidget( _playerDock );
  _playerDock->setWidget( playerControls );

  connect( playerControls , SIGNAL( frame( )) ,
           _openGLWidget , SLOT( update( )) );

  connect( _playerDock->toggleViewAction( ) , SIGNAL( toggled( bool )) ,
           _ui->actionSimulation_player_options , SLOT( setChecked( bool )) );

  connect( _ui->actionSimulation_player_options , SIGNAL( triggered( )) ,
           this , SLOT( updatePlayerOptionsDock( )) );
#endif
}

void MainWindow::changeNeuronColor(QColor color)
{
  auto widget = qobject_cast<ColorSelectionWidget *>(sender());
  if(widget)
  {
    bool ok = false;
    const auto type = widget->property("type").toInt(&ok);
    if(ok && _scene)
    {
      _scene->setColor(type, Eigen::Vector3f(color.redF(), color.greenF(), color.blueF()));
    }
    updateNeuronList();
    _openGLWidget->repaint();
  }
}

void MainWindow::loadData( const std::string& arg1 , const std::string& arg2 ,
                           const neurotessmesh::LoaderThread::DataFileType type )
{
  if ( m_dataLoader ) return; // already loading?

  m_dataLoader = std::make_shared< neurotessmesh::LoaderThread >( arg1 , arg2 ,
                                                                  type );
  auto dialog = new neurotessmesh::LoadingDialog{ this };

  connect( m_dataLoader.get( ) , SIGNAL( finished( )) ,
           this , SLOT( onDataLoaded( )) , Qt::QueuedConnection );

  connect( m_dataLoader.get( ) , SIGNAL( destroyed( QObject * )) ,
           dialog , SLOT( closeDialog( )) );

  connect( m_dataLoader.get( ) ,
           SIGNAL( progress(const QString & , const unsigned int)) ,
           dialog , SLOT( progress(const QString & , const unsigned int)) );

  dialog->show( );

  m_dataLoader->start( );
}

void MainWindow::onDataLoaded( )
{
  this->setWindowTitle("NeuroTessMesh");

  const auto errors = m_dataLoader->errors( );
  if ( !errors.isEmpty( ))
  {
    m_dataLoader = nullptr;

    QMessageBox msgbox{ this };
    msgbox.setWindowTitle( tr( "Error loading dataset" ));
    msgbox.setIcon( QMessageBox::Icon::Critical );
    msgbox.setText( errors );
    msgbox.setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));
    msgbox.setStandardButtons( QMessageBox::Ok );
    msgbox.exec( );
    return;
  }

  const auto fileName = QString::fromStdString(m_dataLoader->filename());
  this->setWindowTitle("NeuroTessMesh - " + fileName);

  try
  {
    _openGLWidget->makeCurrent( );
    _openGLWidget->update( );

    _scene = std::make_shared< neurotessmesh::Scene >(
      _openGLWidget->getCamera( ) , m_dataLoader->getDataset( )
#ifdef NEUROTESSMESH_USE_SIMIL
      , m_dataLoader->getPlayer( )
#endif
    );
    _openGLWidget->setScene( _scene );
  }
  catch ( const std::exception& e )
  {
    m_dataLoader = nullptr;

    QMessageBox msgbox{ this };
    msgbox.setWindowTitle( tr( "Error loading dataset" ));
    msgbox.setIcon( QMessageBox::Icon::Critical );
    msgbox.setText( tr( "Unable to load dataset. Geometry error." ));
    msgbox.setDetailedText(QString::fromLatin1(e.what()));
    msgbox.setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));
    msgbox.setStandardButtons( QMessageBox::Ok );
    msgbox.exec( );
    return;
  }

  _openGLWidget->onLotValueChanged( _lotSlider->value( ));
  _openGLWidget->onDistanceValueChanged( _distanceSlider->value( ));

  _backGroundColor->color(QColor(255,255,255));
  _openGLWidget->changeClearColor(QColor(255,255,255));
  _openGLWidget->changeNeuronColor(1, QColor(250,120,0)); // selected color
  _openGLWidget->changeNeuronColor(0, QColor(0,120,250)); // unselected color
  _renderColoring->parentWidget()->setEnabled(true);
  _renderColoring->setCurrentIndex(0);
  onColoringChanged(0);
  updateNeuronList( );

  // Disable coloring options if not available.
  auto hasMorphology = [](const std::pair<const unsigned int, nsol::NeuronPtr> &p){ return p.second->morphologicalType() != 0; };
  const bool hasMorphoData = std::find_if(_scene->neurons().cbegin(), _scene->neurons().cend(), hasMorphology) != _scene->neurons().cend();

  auto hasLayer = [](const std::pair<const unsigned int, nsol::NeuronPtr> &p){ return p.second->layer() != 0; };
  const bool hasLayerData = std::find_if(_scene->neurons().cbegin(), _scene->neurons().cend(), hasLayer) != _scene->neurons().cend();

  auto hasFunctionality = [](const std::pair<const unsigned int, nsol::NeuronPtr> &p){ return p.second->functionalType() != 0; };
  const bool hasFunctionData = std::find_if(_scene->neurons().cbegin(), _scene->neurons().cend(), hasFunctionality) != _scene->neurons().cend();

  int i = 1;
  for(auto value: {hasMorphoData, hasLayerData, hasFunctionData})
  {
    auto model = dynamic_cast< QStandardItemModel * >( _renderColoring->model() );
    auto item = model->item(i++, 0);
    item->setEnabled( value );
  }

  _openGLWidget->home( );
  _openGLWidget->changeNeuronPiece( _neuronRender->currentIndex( ));

  // @felix Change unselected/selected ? Rethink
  //  _openGLWidget->changeSelectedNeuronPiece(_selectedNeuronRender->currentIndex());

#ifdef NEUROTESSMESH_USE_SIMIL
  auto playerWidget = qobject_cast< qsimil::QSimControlWidget* >(
    _playerDock->widget( ));
  if ( playerWidget )
  {
    playerWidget->init( m_dataLoader->getPlayer( ));
  }
#endif

  m_dataLoader = nullptr;
}
