/**
 * @file    OpenGLWidget.cpp
 * @brief
 * @author  Juan José García <juanjose.garcia@urjc.es>,
 * Pablo Toharia <pablo.toharia@urjc.es>
 * @date    2015
 * @remarks Copyright (c) 2015 GMRV/URJC. All rights reserved.
 * Do not distribute without further notice.
 */

#include "OpenGLWidget.h"
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <sstream>
#include <string>
#include <iostream>

#include "MainWindow.h"
#include <nlrender/nlrender.h>

const float OpenGLWidget::_colorFactor = 1 / 255.0f;

OpenGLWidget::OpenGLWidget( QWidget* parent_,
                            Qt::WindowFlags windowsFlags_ )
  : QOpenGLWidget( parent_, windowsFlags_ )
  , _scene{nullptr}
  , _mouseX( 0 )
  , _mouseY( 0 )
  , _rotation( false )
  , _translation( false )
  , _translationScale( 0.1f )
  , _rotationScale( 0.01f )
  , _wireframe( false )
  , _idleUpdate( false )
  , _fpsLabel( this )
  , _showFps( false )
  , _frameCount( 0 )
#ifdef NEUROTESSMESH_USE_LEXIS
  , _subscriber( nullptr )
  , _subscriberThread( nullptr )
#endif
{
  _camera = new reto::OrbitalCameraController( );
  _cameraTimer = new QTimer( );
  _cameraTimer->start(( 1.0f / 60.f ) * 100 );
  connect( _cameraTimer, SIGNAL( timeout( )), this, SLOT( timerUpdate( )));
  _fpsLabel.setStyleSheet(
    "QLabel { background-color : #333;"
    "color : white;"
    "padding: 3px;"
    "margin: 10px;"
    " border-radius: 10px;}" );

  // This is needed to get key evends
  this->setFocusPolicy( Qt::WheelFocus );

  _lastSavedFileName = QDir::currentPath( );
}

OpenGLWidget::~OpenGLWidget( void )
{
  delete _camera;
  delete _scene;
  delete _cameraTimer;
}

bool OpenGLWidget::loadData(
  const std::string& fileName_,
  const neurotessmesh::Scene::TDataFileType fileType_,
  const std::string& target_ )
{
  makeCurrent( );
  const auto errorString = _scene->loadData( fileName_, fileType_, target_ );

  if(!errorString.empty())
  {
    const QString message = QString("Error loading file '%1'.\n%2")
                             .arg(QString::fromStdString(fileName_))
                             .arg(QString::fromStdString(errorString));

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Error Loading Data"));
    msgBox.setWindowIcon(QIcon(":/icons/rsc/neurotessmesh.png"));
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.exec();
  }

  return errorString.empty();
}

void OpenGLWidget::idleUpdate( bool idleUpdate_ )
{
  _idleUpdate = idleUpdate_;
}

const std::vector< unsigned int > OpenGLWidget::neuronIdList( void ) const
{
  return _scene->neuronIndices( );
}

void OpenGLWidget::neuronToEdit( const unsigned int id_ )
{
  _scene->setNeuronToEdit( id_ );
  update( );
}

unsigned int OpenGLWidget::numNeuritesToEdit( void ) const
{
  return _scene->numEditMorphologyNeurites( );
}

void OpenGLWidget::regenerateNeuronToEdit(
  const float alphaRadius_,
  const std::vector< float >& alphaNeurites_ )
{
  makeCurrent( );
  _scene->regenerateEditNeuronMesh( alphaRadius_, alphaNeurites_ );
  update( );
}

void OpenGLWidget::home( void )
{
  _scene->home( );
  update( );
}

void OpenGLWidget::setZeqSession( const std::string&

#ifdef NEUROTESSMESH_USE_LEXIS
                                  session_
  )
{
  if ( !session_.empty( ))
  {
    if ( _camera )
    {
      delete _camera;
    }

    _camera = new reto::OrbitalCameraController( nullptr, session_ );

    if ( _subscriberThread )
    {
      _subscriberThread->~thread();
      delete _subscriberThread;
      delete _subscriber;
    }

    try
    {
    _subscriber = new zeroeq::Subscriber( session_ );

    _subscriber->subscribe(
      lexis::data::SelectedIDs::ZEROBUF_TYPE_IDENTIFIER( ),
      [&]( const void* selectedData, size_t selectedSize )
      { _onSelectionEvent( lexis::data::SelectedIDs::create(
                             selectedData, selectedSize ));});

#ifdef NEUROTESSMESH_USE_GMRVLEX
    _subscriber->subscribe(
      zeroeq::gmrv::FocusedIDs::ZEROBUF_TYPE_IDENTIFIER( ),
      [&]( const void* focusedData, size_t focusedSize )
      { _onFocusEvent( zeroeq::gmrv::FocusedIDs::create(
                             focusedData, focusedSize ));});
#endif

    _subscriberThread =
      new std::thread( [&](){
          try
          {
            while ( true )
              _subscriber->receive( 10000 );
          }
          catch( ... )
          {
            std::cerr << "Connexion closed" << std::endl;
          }
        });

    }
    catch ( ... )
    {
      std::cerr << "Zeroeq Session Error: could not connect to " << session_
                << " session" << std::endl;
    }
  }
}

#else

  )
{
  std::cerr << "Zeq not supported " << std::endl;
}
#endif

CameraPosition OpenGLWidget::cameraPosition() const
{
  CameraPosition pos;
  pos.position = _camera->position();
  pos.radius   = _camera->radius();
  pos.rotation = _camera->rotation();

  return pos;
}

void OpenGLWidget::setCameraPosition(const CameraPosition &pos)
{
  if(_camera)
  {
    _camera->position(pos.position);
    _camera->radius(pos.radius);
    _camera->rotation(pos.rotation);
    update();
  }
}

void OpenGLWidget::changeClearColor( QColor color )
{
    makeCurrent( );
    glClearColor( float( color.red( )) * _colorFactor,
                  float( color.green( )) * _colorFactor,
                  float( color.blue( )) * _colorFactor,
                  float( color.alpha( )) * _colorFactor);
    update( );
}

void OpenGLWidget::changeNeuronColor( QColor color )
{
  makeCurrent( );
  _scene->unselectedNeuronColor(
    Eigen::Vector3f( float( color.red( )) * _colorFactor,
                     float( color.green( )) * _colorFactor,
                     float( color.blue( )) * _colorFactor ));
  update( );
}

void OpenGLWidget::changeSelectedNeuronColor(  QColor color )
{
  makeCurrent( );
  _scene->selectedNeuronColor(
    Eigen::Vector3f( float( color.red( )) * _colorFactor,
                     float( color.green( )) * _colorFactor,
                     float( color.blue( )) * _colorFactor ));
  update( );
}

void OpenGLWidget::toggleUpdateOnIdle( void )
{
  _idleUpdate = !_idleUpdate;
  if ( _idleUpdate )
    update( );
}

void OpenGLWidget::toggleShowFPS( void )
{
  _showFps = !_showFps;
  if ( _idleUpdate )
    update( );
}

void OpenGLWidget::toggleWireframe( void )
{
  makeCurrent( );
  _wireframe = !_wireframe;

  if ( _wireframe )
  {
    glEnable( GL_POLYGON_OFFSET_LINE );
    glPolygonOffset( -1, -1 );
    glLineWidth( 1.5 );
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
  }
  else
  {
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glDisable( GL_POLYGON_OFFSET_LINE );
  }

  update( );
}

void OpenGLWidget::timerUpdate( void )
{
  if( _camera->isAniming() )
  {
    _camera->anim();
    this->update( );
  }
}

void OpenGLWidget::extractEditNeuronMesh( void )
{
  if( _scene->isEditNeuronMeshExtraction( ))
  {
    QString path;
    const QString filter( tr( "OBJ (*.obj);; All files (*)" ));
    QFileDialog fd(this, QString( "Save mesh to OBJ file" ),
                   _lastSavedFileName, filter );

    fd.setOption( QFileDialog::DontUseNativeDialog, true );
    fd.setDefaultSuffix( "obj") ;
    fd.setFileMode( QFileDialog/*::FileMode*/::AnyFile );
    fd.setAcceptMode( QFileDialog/*::AcceptMode*/::AcceptSave );
    if ( fd.exec( ))
      path = fd.selectedFiles( )[0];

    if ( !path.isEmpty() )
    {
      _lastSavedFileName = QFileInfo( path ).path( );
      this->makeCurrent( );
      _scene->extractEditNeuronMesh( path.toStdString( ));
      glUseProgram( 0 );
    }
  }
}

void OpenGLWidget::onLotValueChanged( int value_ )
{
  _scene->levelOfDetail( (( float )value_) * 0.1  );
  update( );
}

void OpenGLWidget::onDistanceValueChanged( int value_ )
{
  _scene->maximumDistance(( float ) value_ / 1000.0f );
  update( );
}

void OpenGLWidget::onHomogeneousClicked( void )
{
  _scene->subdivisionCriteria( nlrender::Renderer::HOMOGENEOUS );
  update( );
}

void OpenGLWidget::onLinearClicked( void )
{
  _scene->subdivisionCriteria( nlrender::Renderer::LINEAR );
  update( );
}
void OpenGLWidget::changeNeuronPiece( int index_ )
{
  switch( index_ )
  {
  case 0:
    _scene->paintUnselectedSoma( true );
    _scene->paintUnselectedNeurites( true );
    break;
  case 1:
    _scene->paintUnselectedSoma( true );
    _scene->paintUnselectedNeurites( false );
    break;
  case 2:
    _scene->paintUnselectedSoma( false );
    _scene->paintUnselectedNeurites( true );
    break;
  }
  update( );
}

void OpenGLWidget::changeSelectedNeuronPiece( int index_ )
{
  switch( index_ )
  {
  case 0:
    _scene->paintSelectedSoma( true );
    _scene->paintSelectedNeurites( true );
    break;
  case 1:
    _scene->paintSelectedSoma( true );
    _scene->paintSelectedNeurites( false );
    break;
  case 2:
    _scene->paintSelectedSoma( false );
    _scene->paintSelectedNeurites( true );
    break;
  }
  update( );
}

void OpenGLWidget::initializeGL( void )
{
  initializeOpenGLFunctions( );

  glEnable( GL_DEPTH_TEST );
  glClearColor(  1.0f, 1.0f, 1.0f, 1.0f );
  glPolygonMode( GL_FRONT_AND_BACK , GL_FILL );
  glEnable( GL_CULL_FACE );

  glLineWidth( 1.5 );

  QOpenGLWidget::initializeGL( );

  const GLubyte* vendor = glGetString(GL_VENDOR); // Returns the vendor
  const GLubyte* renderer = glGetString(GL_RENDERER); // Returns a hint to the model
  const GLubyte* version = glGetString(GL_VERSION);
  const GLubyte* shadingVer = glGetString(GL_SHADING_LANGUAGE_VERSION);

  std::cout << "OpenGL Hardware: " << vendor << " (" << renderer << ")" << std::endl;
  std::cout << "OpenGL Version: " << version << " (shading ver. " << shadingVer << ")" << std::endl;

  nlrender::Config::init( );
  _scene = new neurotessmesh::Scene( _camera );
}

void OpenGLWidget::paintGL( void )
{
  makeCurrent( );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  _scene->render( );

  glUseProgram( 0 );
  glFlush( );

#define FRAMES_PAINTED_TO_MEASURE_FPS 10
  if ( _frameCount % FRAMES_PAINTED_TO_MEASURE_FPS  == 0 )
  {
    std::chrono::time_point< std::chrono::system_clock > now =
      std::chrono::system_clock::now( );

    const auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>( now - _then );
    _then = now;

    MainWindow* mainWindow = dynamic_cast< MainWindow* >( parent( ));
    if ( mainWindow )
    {
      const unsigned int ellapsedMiliseconds = duration.count( );

      const unsigned int fps = roundf( 1000.0f *
                                 float( FRAMES_PAINTED_TO_MEASURE_FPS ) /
                                 float( ellapsedMiliseconds ));

      // mainWindow->showStatusBarMessage(
      //   QString::number( fps ) + QString( " FPS" ));
      if ( _showFps )
      {
        _fpsLabel.setVisible( true );
        _fpsLabel.setText( QString::number( fps ) + QString( " FPS" ));
        _fpsLabel.adjustSize( );
      }
      else
        _fpsLabel.setVisible( false );
    }
  }

  if ( _idleUpdate )
  {
    update( );
  }
  else
  {
    _fpsLabel.setVisible( false );
  }
}

void OpenGLWidget::resizeGL( int width_ , int height_ )
{
  _camera->windowSize(width_, height_);
  glViewport( 0, 0, width_, height_ );
}

void OpenGLWidget::mousePressEvent( QMouseEvent* event_ )
{

  if ( event_->button( ) == Qt::LeftButton )
  {
    _rotation = true;
    _mouseX = event_->x( );
    _mouseY = event_->y( );
  }

  if ( event_->button( ) ==  Qt::MidButton )
  {
    _translation = true;
    _mouseX = event_->x( );
    _mouseY = event_->y( );
  }

  update( );

}

void OpenGLWidget::mouseReleaseEvent( QMouseEvent* event_ )
{
  if ( event_->button( ) == Qt::LeftButton)
  {
    _rotation = false;
  }

  if ( event_->button( ) ==  Qt::MidButton )
  {
    _translation = false;
  }

  update( );

}

void OpenGLWidget::mouseMoveEvent( QMouseEvent* event_ )
{
  if( _rotation )
  {
      _camera->rotate( Eigen::Vector3f{ -( _mouseX - event_->x( )) * _rotationScale,
                                        -( _mouseY - event_->y( )) * _rotationScale,
                                        0.f } );

      _mouseX = event_->x( );
      _mouseY = event_->y( );  }
  if( _translation )
  {
      float xDis = ( event_->x() - _mouseX ) * _translationScale;
      float yDis = ( event_->y() - _mouseY ) * _translationScale;

      _camera->translate( Eigen::Vector3f( -xDis, yDis, 0.0f ));
      _mouseX = event_->x( );
      _mouseY = event_->y( );
  }

  this->update( );
}

void OpenGLWidget::wheelEvent( QWheelEvent* event_ )
{

  int delta = event_->angleDelta( ).y( );

  if ( delta > 0 )
    _camera->radius( _camera->radius( ) / 1.1f );
  else
    _camera->radius( _camera->radius( ) * 1.1f );

  update( );

}

void OpenGLWidget::keyPressEvent( QKeyEvent* event_ )
{
  makeCurrent( );

  switch ( event_->key( ))
  {
  case Qt::Key_C:
    _camera->position( Eigen::Vector3f( 0.f, 0.f, 0.f ));
    _camera->radius( 1000.0f );
    _camera->rotation( Eigen::Vector3f{0.f, 0.f,0.f} );
    update( );
    break;
  }
}

#ifdef NEUROTESSMESH_USE_LEXIS
void OpenGLWidget::_onSelectionEvent(
  lexis::data::ConstSelectedIDsPtr selectedIndices_ )
{
  std::vector<unsigned int> selectedIndices = selectedIndices_->getIdsVector( );
  _scene->changeSelectedIndices( selectedIndices );
  update( );
}
#endif

#ifdef NEUROTESSMESH_USE_GMRVLEX
void OpenGLWidget::_onFocusEvent(
  zeroeq::gmrv::ConstFocusedIDsPtr focusIndices_ )
{
  std::vector<unsigned int> focusedIndices = focusIndices_->getIdsVector( );
  _scene->focusOnIndices( focusedIndices );
  update( );
}
#endif
