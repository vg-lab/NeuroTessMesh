/**
 * @file    MainWindow.cpp
 * @brief
 * @author  Juan José García <juanjose.garcia@urjc.es>,
 * Pablo Toharia <pablo.toharia@urjc.es>
 * @date    2015
 * @remarks Copyright (c) 2015 GMRV/URJC. All rights reserved.
 * Do not distribute without further notice.
 */


#include "MainWindow.h"
#include <neurotessmesh/version.h>
#include <nsol/nsol.h>
#ifdef NEUROLOTS_USE_GMRVZEQ
#include <gmrvzeq/version.h>
#endif
#ifdef NEUROLOTS_USE_DEFLECT
#include <deflect/version.h>
#endif

#include <acuterecorder/acuterecorder.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QGridLayout>

MainWindow::MainWindow( QWidget* parent_, bool updateOnIdle_ )
  : QMainWindow( parent_ )
  , _lastOpenedFileName( "" )
  , _ui( new Ui::MainWindow )
  , _openGLWidget( nullptr )
  , _recorder( nullptr )
{
  _ui->setupUi( this );

  _ui->actionUpdateOnIdle->setChecked( updateOnIdle_ );
  _ui->actionShowFPSOnIdleUpdate->setChecked( false );

#ifdef NSOL_USE_BRION
  _ui->actionOpenBlueConfig->setEnabled( true );
#else
  //_ui->actionOpenBlueConfig->setEnabled( false );
  _ui->actionOpenBlueConfig->setVisible(false);
#endif

#ifdef NSOL_USE_QT5CORE
  _ui->actionOpenXMLScene->setEnabled( true );
#else
  _ui->actionOpenXMLScene->setEnabled( false );
#endif


  connect( _ui->actionQuit, SIGNAL( triggered( )),
           QApplication::instance(), SLOT( quit( )));

  connect( _ui->actionAbout, SIGNAL(triggered( )),
           this, SLOT( showAbout( )));

  _initExtractionDock( );
  _initConfigurationDock( );
  _initRenderOptionsDock( );

  _openGLWidget = new OpenGLWidget();
  this->setCentralWidget( _openGLWidget );
  _openGLWidget->setMinimumSize( QSize( 100, 100 ));

  if( _openGLWidget->format( ).version( ).first < 4 )
  {
    std::cerr << "This application requires at least OpenGL 4.0" << std::endl;
    exit( -1 );
  }
}

MainWindow::~MainWindow( void )
{
    delete _ui;
}

void MainWindow::init( const std::string& zeqSession_ )
{
  _openGLWidget->idleUpdate( _ui->actionUpdateOnIdle->isChecked( ));
  if ( !zeqSession_.empty( ))
    _openGLWidget->setZeqSession( zeqSession_ );

  connect( _ui->actionHome, SIGNAL( triggered( )),
           this, SLOT( home( )));

  connect( _ui->actionUpdateOnIdle, SIGNAL( triggered( )),
           _openGLWidget, SLOT( toggleUpdateOnIdle( )));

  connect( _ui->actionShowFPSOnIdleUpdate, SIGNAL( triggered( )),
           _openGLWidget, SLOT( toggleShowFPS( )));

  connect( _ui->actionWireframe, SIGNAL( triggered( )),
           _openGLWidget, SLOT( toggleWireframe( )));

  connect( _ui->actionOpenBlueConfig, SIGNAL( triggered( )),
           this, SLOT( openBlueConfigThroughDialog( )));

  connect( _ui->actionOpenXMLScene, SIGNAL( triggered( )),
           this, SLOT( openXMLSceneThroughDialog( )));

  connect( _ui->actionOpenSWCFile, SIGNAL( triggered( )),
           this, SLOT( openSWCFileThroughDialog( )));

  connect( _radiusSlider, SIGNAL( valueChanged( int )),
           this, SLOT( onActionGenerate( int )));

  connect( _extractButton, SIGNAL( clicked( )),
           _openGLWidget, SLOT( extractEditNeuronMesh( )));

  connect( _lotSlider, SIGNAL( valueChanged( int )),
           _openGLWidget, SLOT( onLotValueChanged( int )));
  _lotSlider->valueChanged( _lotSlider->value( ));

  connect( _distanceSlider, SIGNAL( valueChanged( int )),
           _openGLWidget, SLOT( onDistanceValueChanged( int )));
  _distanceSlider->valueChanged( _distanceSlider->value( ));

  connect( _radioHomogeneous, SIGNAL( clicked( )),
           _openGLWidget, SLOT( onHomogeneousClicked( )));

  connect( _radioLinear, SIGNAL( clicked( )),
           _openGLWidget, SLOT( onLinearClicked( )));
  _radioLinear->clicked( );

  connect( _backGroundColor, SIGNAL( colorChanged( QColor )),
           _openGLWidget, SLOT( changeClearColor( QColor )));
  _backGroundColor->color( QColor( 255, 255, 255 ));

  connect( _neuronColor, SIGNAL( colorChanged( QColor )),
           _openGLWidget, SLOT( changeNeuronColor( QColor )));
  _neuronColor->color( QColor( 0, 120, 250 ));

  connect( _selectedNeuronColor, SIGNAL( colorChanged( QColor )),
           _openGLWidget, SLOT( changeSelectedNeuronColor( QColor )));
  _selectedNeuronColor->color( QColor( 250, 120, 0 ));

  connect( _neuronRender, SIGNAL( currentIndexChanged( int )),
           _openGLWidget, SLOT( changeNeuronPiece( int )));
  _neuronRender->currentIndexChanged( 1 );

  connect( _selectedNeuronRender, SIGNAL( currentIndexChanged( int )),
           _openGLWidget, SLOT( changeSelectedNeuronPiece( int )));
  _selectedNeuronRender->currentIndexChanged( 0 );

  connect( _ui->actionRecorder , SIGNAL( triggered( void )) , this ,
           SLOT( openRecorder( void )));
}

void MainWindow::showStatusBarMessage ( const QString& message )
{
  _ui->statusbar->showMessage( message );
}

void MainWindow::openBlueConfig( const std::string& fileName,
                                 const std::string& targetLabel )
{
  _openGLWidget->loadData( fileName,
                           neurotessmesh::Scene::TDataFileType::BlueConfig,
                           targetLabel );
  updateNeuronList( );
  _openGLWidget->changeNeuronPiece(_neuronRender->currentIndex());
  _openGLWidget->changeSelectedNeuronPiece(_selectedNeuronRender->currentIndex());
}

void MainWindow::openXMLScene( const std::string& fileName )
{
  _openGLWidget->loadData( fileName,
                           neurotessmesh::Scene::TDataFileType::NsolScene );
  updateNeuronList( );
  _openGLWidget->changeNeuronPiece(_neuronRender->currentIndex());
  _openGLWidget->changeSelectedNeuronPiece(_selectedNeuronRender->currentIndex());
}

void MainWindow::openSWCFile( const std::string& fileName )
{
  _openGLWidget->loadData( fileName,
                           neurotessmesh::Scene::TDataFileType::SWC );
  updateNeuronList( );
  _openGLWidget->changeNeuronPiece(_neuronRender->currentIndex());
  _openGLWidget->changeSelectedNeuronPiece(_selectedNeuronRender->currentIndex());
}

void MainWindow::updateNeuronList( void )
{
  _neuronList->clear( );
  const std::vector< unsigned int >& ids = _openGLWidget->neuronIdList( );

  for( const auto& id: ids )
  {
    _neuronList->addItem( QString::number( id ));
  }
}

void MainWindow::home( void )
{
  _openGLWidget->home( );
  _generateNeuritesLayout( );
  _extractButton->setEnabled( false );
  _somaGroup->hide( );
}

void MainWindow::openBlueConfigThroughDialog( void )
{
#ifdef NSOL_USE_BRION

  QString path = QFileDialog::getOpenFileName(
    this, tr( "Open BlueConfig" ), _lastOpenedFileName,
    tr( "BlueConfig ( BlueConfig CircuitConfig);; All files (*)" ),
    nullptr, QFileDialog::DontUseNativeDialog );

  if (path != QString( "" ))
  {
    bool ok;
    QString text = QInputDialog::getText(
      this, tr( "Please select target" ),
      tr( "Cell target:" ), QLineEdit::Normal,
      "Column", &ok );
    if ( ok && !text.isEmpty( ))
    {
      std::string targetLabel = text.toStdString( );
      _lastOpenedFileName = QFileInfo(path).path( );
      std::string fileName = path.toStdString( );
      openBlueConfig( fileName, targetLabel );
    }
  }
#endif
}

void MainWindow::openXMLSceneThroughDialog( void )
{
#ifdef NSOL_USE_QT5CORE
  QString path = QFileDialog::getOpenFileName(
    this, tr( "Open XML Scene" ), _lastOpenedFileName,
    tr( "XML ( *.xml);; All files (*)" ), nullptr,
    QFileDialog::DontUseNativeDialog );

  if ( path != QString( "" ))
  {
    std::string fileName = path.toStdString( );
    openXMLScene( fileName );
  }
#endif

}

void MainWindow::openSWCFileThroughDialog( void )
{
  QString path = QFileDialog::getOpenFileName(
    this, tr( "Open Swc File" ), _lastOpenedFileName,
    tr( "swc ( *.swc);; All files (*)" ), nullptr,
    QFileDialog::DontUseNativeDialog );

  if ( path != QString( "" ))
  {
    std::string fileName = path.toStdString( );
    openSWCFile( fileName );
  }
}

void MainWindow::showAbout( void )
{

  QMessageBox::about(
    this, tr( "About " ) + tr( "NeuroTessMesh" ),
    tr( "<p><BIG><b>" ) + tr( "NeuroTessMesh" ) + tr( "</b></BIG><br><br>" ) +
    tr( "version " ) +
    tr( neurotessmesh::Version::getString( ).c_str( )) +
    tr( " (" ) +
    tr( std::to_string( neurotessmesh::Version::getRevision( )).c_str( )) +
    tr( ")" ) +
    tr( "<br><br>Using: " ) +
    tr( "<ul>") +
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
    tr ( "</li> " ) +
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
    tr( acuterecorder::Version::getString().c_str( )) +
    tr( " (" ) +
    tr( std::to_string( acuterecorder::Version::getRevision( )).c_str( )) +
    tr( ")" ) +
    tr ( "</li> " ) +

    tr ( "</ul>" ) +
    tr( "<br>VG-Lab - Universidad Rey Juan Carlos<br>"
       "<a href=www.vg-lab.es>www.vg-lab.es</a><br>"
       "<a href='mailto:dev@vg-lab.es'>dev@vg-lab.es</a><br><br>"
       "<br>(C) 2015-2022. Universidad Rey Juan Carlos<br><br>"
       "<img src=':/icons/rsc/logoVGLab.png' >&nbsp;&nbsp;&nbsp;&nbsp;"
       "<img src=':/icons/rsc/logoURJC.png' ><br><br> "
       "</p>"
       "")
    );
}

void MainWindow::openRecorder()
{
  // The button stops the recorder if found.
  if( _recorder )
  {
    _ui->actionRecorder->setDisabled( true );
    _recorder->stop();

    // Recorder will be deleted after finishing.
    _recorder = nullptr;
    _ui->actionRecorder->setChecked( false );
    return;
  }

  RSWParameters params;
  params.widgetsToRecord.emplace_back( "Viewport" , _openGLWidget );
  params.widgetsToRecord.emplace_back( "Main Widget" , this );
  params.includeScreens = false;

  if(!_ui->actionAdvancedRecorderOptions->isChecked())
  {
    params.showWorker = false;
    params.showWidgetSourceMode = false;
    params.showSourceParameters = false;
  }

  RecorderDialog dialog( nullptr , params , true );
  dialog.setWindowIcon( QIcon( ":/icons/rsc/neurotessmesh.png" ));
  dialog.setFixedSize( 800 , 600 );
  if ( dialog.exec( ) == QDialog::Accepted)
  {
    _recorder = dialog.getRecorder( );
    connect( _recorder , SIGNAL( finished( )) ,
             _recorder , SLOT( deleteLater( )));
    connect( _recorder , SIGNAL( finished( )) ,
             this , SLOT( finishRecording( )));
    _ui->actionRecorder->setChecked( true );
  } else
  {
    _ui->actionRecorder->setChecked( false );
  }
}

void MainWindow::updateExtractMeshDock( void )
{
  if( _ui->actionEditSave->isChecked( ))
    _extractMeshDock->show( );
  else
    _extractMeshDock->close( );
}

void MainWindow::updateConfigurationDock( void )
{
  if( _ui->actionConfiguration->isChecked( ))
    _configurationDock->show( );
  else
    _configurationDock->close( );
}

void MainWindow::updateRenderOptionsDock( void )
{
  if( _ui->actionRenderOptions->isChecked( ))
    _renderOptionsDock->show( );
  else
    _renderOptionsDock->close( );
}


void MainWindow::onListClicked( QListWidgetItem* item )
{
  int id = item->text( ).toInt( );
  _openGLWidget->neuronToEdit( id );
  _generateNeuritesLayout( );
  _extractButton->setEnabled( true );
  _somaGroup->show( );

}

void MainWindow::onActionGenerate( int /*value_*/ )
{
  float alphaRadius = ( float )_radiusSlider->value( ) / 100.0f;
  std::vector< float > alphaNeurites;

  for ( unsigned int i = 0; i < _neuriteSliders.size( ); i++ )
  {
    alphaNeurites.push_back(( float ) _neuriteSliders[i]->value( ) / 100.0f );
  }

  _openGLWidget->regenerateNeuronToEdit( alphaRadius, alphaNeurites );
}

void MainWindow::finishRecording( )
{
  _ui->actionRecorder->setEnabled( true );
}

void MainWindow::_generateNeuritesLayout( void )
{
  const unsigned int numDendrites = _openGLWidget->numNeuritesToEdit( );

  _neuriteSliders.clear( );

  QLayoutItem* child;
  while(( child = _neuritesLayout->takeAt( 0 )) != 0 )
  {
    delete child->widget( );
  }

  QSlider* _neuriteSlider;
  for( unsigned int i = 0; i < numDendrites; i++ )
  {
    _neuriteSlider = new QSlider( Qt::Horizontal );
    _neuriteSlider->setMinimum(0);
    _neuriteSlider->setMaximum(200);
    _neuriteSlider->setValue(100);
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
    // connect( _neuriteSlider, SIGNAL( sliderReleased( )),
    //        this, SLOT( onActionGenerate( )));
    connect( _neuriteSlider, SIGNAL( valueChanged( int )),
           this, SLOT( onActionGenerate( int )));
  }
  _radiusSlider->setValue(100);
}

void MainWindow::_initExtractionDock( void )
{
  _extractMeshDock = new QDockWidget( );
  this->addDockWidget( Qt::DockWidgetAreas::enum_type::RightDockWidgetArea,
                       _extractMeshDock, Qt::Vertical );
  _extractMeshDock->setSizePolicy(QSizePolicy::MinimumExpanding,
                             QSizePolicy::Expanding);

  _extractMeshDock->setFeatures(QDockWidget::DockWidgetClosable |
                           QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetFloatable);
  _extractMeshDock->setWindowTitle( QString( "Edit And Save" ));
  _extractMeshDock->setMinimumSize( 200, 200 );

  _extractMeshDock->close( );

  QWidget* newWidget = new QWidget( );
  _extractMeshDock->setWidget( newWidget );

  QVBoxLayout* _meshDockLayout = new QVBoxLayout( );
  _meshDockLayout->setAlignment( Qt::AlignTop );
  newWidget->setLayout( _meshDockLayout );

  //Neurons group
  QGroupBox* _neuronsGroup = new QGroupBox( QString( "Select Neuron" ));
  QVBoxLayout* _neuronsLayout = new QVBoxLayout( );
  _neuronsGroup->setLayout( _neuronsLayout );
  _meshDockLayout->addWidget( _neuronsGroup );

  _neuronList = new QListWidget( );
  _neuronsLayout->addWidget( new QLabel( QString( "Neurons" )));
  _neuronsLayout->addWidget( _neuronList );

  // Soma reconstruction group
  _somaGroup = new QGroupBox( QString( "Parameters" ));
  QVBoxLayout* _somaGroupLayout = new QVBoxLayout( );
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

  QScrollArea* _neuritesArea = new QScrollArea( );
  _neuritesArea->setSizePolicy( QSizePolicy::MinimumExpanding,
                                QSizePolicy::Expanding );
  _neuritesArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
  _neuritesArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
  _neuritesArea->setWidgetResizable( true );
  _neuritesArea->setFrameShape( QFrame::NoFrame );
  _somaGroupLayout->addWidget( _neuritesArea );
  QWidget* _neuritesWidget = new QWidget( );
  _neuritesArea->setWidget( _neuritesWidget );
  _neuritesLayout = new QVBoxLayout( );
  _neuritesWidget->setLayout( _neuritesLayout );

  _extractButton = new QPushButton( QString( "Save" ));
  _extractButton->setSizePolicy( QSizePolicy::Fixed,
                                 QSizePolicy::Fixed);
  _extractButton->setEnabled( false );
  _meshDockLayout->addWidget( _extractButton );

  connect( _neuronList, SIGNAL( itemClicked( QListWidgetItem* )),
           this, SLOT( onListClicked( QListWidgetItem* )));

  connect( _extractMeshDock->toggleViewAction( ), SIGNAL( toggled( bool )),
           _ui->actionEditSave, SLOT( setChecked( bool )));
  connect( _ui->actionEditSave, SIGNAL( triggered( )),
           this, SLOT( updateExtractMeshDock( )));

}

void MainWindow::_initConfigurationDock( void )
{
  _configurationDock = new QDockWidget( );
  this->addDockWidget( Qt::DockWidgetAreas::enum_type::LeftDockWidgetArea,
                       _configurationDock, Qt::Vertical );
  _configurationDock->setSizePolicy(QSizePolicy::MinimumExpanding,
                             QSizePolicy::Expanding);

  _configurationDock->setFeatures(QDockWidget::DockWidgetClosable |
                           QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetFloatable);
  _configurationDock->setWindowTitle( QString( "Configuration" ));
  _configurationDock->setMinimumSize( 200, 200 );

  _configurationDock->close( );

  QWidget* newWidget = new QWidget( );
  _configurationDock->setWidget( newWidget );

  QVBoxLayout* _configDockLayout = new QVBoxLayout( );
  _configDockLayout->setAlignment( Qt::AlignTop );
  newWidget->setLayout( _configDockLayout );

  QGroupBox* tessParamsGroup = new QGroupBox( QString( "Tessellation params" ));
  tessParamsGroup->setSizePolicy( QSizePolicy::Fixed,
                                  QSizePolicy::Fixed);
  _configDockLayout->addWidget( tessParamsGroup );
  QVBoxLayout* vbox = new QVBoxLayout;
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

  connect( _radioLinear, SIGNAL( toggled( bool )),
           _distanceSlider, SLOT( setEnabled( bool )));


  connect( _configurationDock->toggleViewAction( ), SIGNAL( toggled( bool )),
           _ui->actionConfiguration, SLOT( setChecked( bool )));
  connect( _ui->actionConfiguration, SIGNAL( triggered( )),
           this, SLOT( updateConfigurationDock( )));
}

void MainWindow::_initRenderOptionsDock( void )
{
  _renderOptionsDock = new QDockWidget( );
  this->addDockWidget( Qt::DockWidgetAreas::enum_type::LeftDockWidgetArea,
                       _renderOptionsDock, Qt::Vertical );
  _renderOptionsDock->setSizePolicy(QSizePolicy::Fixed,
                                    QSizePolicy::Fixed);
  _renderOptionsDock->setFeatures(QDockWidget::DockWidgetClosable |
                           QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetFloatable);
  _renderOptionsDock->setWindowTitle( QString( "Render Options" ));
  _renderOptionsDock->setMinimumSize( 200, 200 );

  _renderOptionsDock->close( );

  QWidget* newWidget = new QWidget( );
  _renderOptionsDock->setWidget( newWidget );

  QVBoxLayout* roDockLayout = new QVBoxLayout( );
  roDockLayout->setAlignment( Qt::AlignTop );
  newWidget->setLayout( roDockLayout );


  QGroupBox* colorGroup = new QGroupBox( QString( "Color" ));
  colorGroup->setSizePolicy( QSizePolicy(QSizePolicy::Fixed,
                                         QSizePolicy::Fixed));
  roDockLayout->addWidget( colorGroup );
  QGridLayout* gridbox = new QGridLayout;
  colorGroup->setLayout( gridbox );

  gridbox->addWidget( new QLabel( QString("Background color")), 0, 0);
  _backGroundColor = new ColorSelectionWidget( this );
  gridbox->addWidget( _backGroundColor, 0, 1 );

  gridbox->addWidget( new QLabel( QString("Neuron color")), 1, 0);
  _neuronColor = new ColorSelectionWidget( this );
  gridbox->addWidget( _neuronColor, 1, 1 );

  gridbox->addWidget( new QLabel( QString("Selected neuron color")), 2, 0);
  _selectedNeuronColor = new ColorSelectionWidget( this );
  gridbox->addWidget( _selectedNeuronColor, 2, 1 );


  QGroupBox* renderGroup = new QGroupBox( QString( "Render piece selection" ));
  roDockLayout->addWidget( renderGroup );
  QVBoxLayout* vbox = new QVBoxLayout;
  renderGroup->setLayout( vbox );

  _neuronRender = new QComboBox( );
  _neuronRender->setSizePolicy( QSizePolicy(QSizePolicy::Fixed,
                                            QSizePolicy::Fixed));
  vbox->addWidget( new QLabel( QString( "Neuron" )));
  vbox->addWidget( _neuronRender );
  _neuronRender->addItem( QString( "all" ));
  _neuronRender->addItem( QString( "soma" ));
  _neuronRender->addItem( QString( "neurites" ));

  _selectedNeuronRender = new QComboBox( );
  _selectedNeuronRender->setSizePolicy( QSizePolicy(QSizePolicy::Fixed,
                                                    QSizePolicy::Fixed));
  vbox->addWidget( new QLabel( QString( "Selected neuron" )));
  vbox->addWidget( _selectedNeuronRender );
  _selectedNeuronRender->addItem( QString( "all" ));
  _selectedNeuronRender->addItem( QString( "soma" ));
  _selectedNeuronRender->addItem( QString( "neurites" ));

  connect( _renderOptionsDock->toggleViewAction( ), SIGNAL( toggled( bool )),
           _ui->actionRenderOptions, SLOT( setChecked( bool )));
  connect( _ui->actionRenderOptions, SIGNAL( triggered( )),
           this, SLOT( updateRenderOptionsDock( )));
}
