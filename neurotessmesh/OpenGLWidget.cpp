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
#include "MainWindow.h"
#include "Scene.h"

#include <QOpenGLContext>
#include <QMouseEvent>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

#include <sstream>
#include <string>
#include <iostream>
#include <utility>

#include <nlrender/nlrender.h>

const QString FPSLABEL_STYLESHEET = "QLabel { background-color : #333;"
                                    "color : white;"
                                    "padding: 3px;"
                                    "margin: 10px;"
                                    "border-radius: 10px;}";

OpenGLWidget::OpenGLWidget( QWidget* parent_ ,
                            Qt::WindowFlags windowsFlags_ )
  : QOpenGLWidget( parent_ , windowsFlags_ )
  , _scene( nullptr )
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
  try
  {
    _camera = new reto::OrbitalCameraController( );
  }
  catch ( ... )
  {
    _camera = new reto::OrbitalCameraController( nullptr ,
                                                 reto::Camera::NO_ZEROEQ );
  }

  _cameraTimer = new QTimer( );
  _cameraTimer->start(( 1.0f / 60.f ) * 100 );
  connect( _cameraTimer , SIGNAL( timeout( )) , this , SLOT( timerUpdate( )));
  _fpsLabel.setStyleSheet( FPSLABEL_STYLESHEET );

  // This is needed to get key events
  this->setFocusPolicy( Qt::WheelFocus );

  _lastSavedFileName = QDir::currentPath( );
}

OpenGLWidget::~OpenGLWidget( )
{
  delete _camera;
  delete _cameraTimer;
}

void OpenGLWidget::setScene( std::shared_ptr< neurotessmesh::Scene > scene )
{
  _scene = std::move( scene );
  makeCurrent( );
}

void OpenGLWidget::idleUpdate( bool idleUpdate_ )
{
  _idleUpdate = idleUpdate_;
}

void OpenGLWidget::home( )
{
  update( );
}

void OpenGLWidget::setZeqSession( const std::string&

#ifdef NEUROTESSMESH_USE_LEXIS
  session_
)
{
  if (!session_.empty())
  {
    if (_camera)
    {
      delete _camera;
    }

    try
    {
      _camera = new reto::OrbitalCameraController(nullptr, session_);
    }
    catch (...)
    {
      _camera = new reto::OrbitalCameraController(nullptr,
          reto::Camera::NO_ZEROEQ);
    }

    if (_subscriberThread)
    {
      _subscriberThread->~thread();
      delete _subscriberThread;
      delete _subscriber;
    }

    try
    {
      _subscriber = new zeroeq::Subscriber(session_);

      _subscriber->subscribe(
          lexis::data::SelectedIDs::ZEROBUF_TYPE_IDENTIFIER(),
          [&](const void *selectedData, size_t selectedSize)
          { _onSelectionEvent( lexis::data::SelectedIDs::create(
                    selectedData, selectedSize ));});

#ifdef NEUROTESSMESH_USE_GMRVLEX
      _subscriber->subscribe(
          zeroeq::gmrv::FocusedIDs::ZEROBUF_TYPE_IDENTIFIER(),
          [&](const void *focusedData, size_t focusedSize)
          { _onFocusEvent( zeroeq::gmrv::FocusedIDs::create(
                    focusedData, focusedSize ));});
#endif

      _subscriberThread = new std::thread([&]()
      {
        try
        {
          while ( true )
          _subscriber->receive( 10000 );
        }
        catch( ... )
        {
          std::cerr << "EXCEPTION: ZeroEQ Conection closed -> "
          << __FILE__ << ":" << __LINE__ << std::endl;
        }
      });

    }
    catch (...)
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

reto::OrbitalCameraController* OpenGLWidget::getCamera( ) const
{
  return _camera;
}

CameraPosition OpenGLWidget::cameraPosition( ) const
{
  CameraPosition pos;
  pos.position = _camera->position( );
  pos.radius = _camera->radius( );
  pos.rotation = _camera->rotation( );

  return pos;
}

void OpenGLWidget::toggleUpdateOnIdle( )
{
  _idleUpdate = !_idleUpdate;
  if ( _idleUpdate )
    update( );
}

void OpenGLWidget::toggleShowFPS( )
{
  _showFps = !_showFps;
  if ( _idleUpdate )
    update( );
}

void OpenGLWidget::toggleWireframe( )
{
  makeCurrent( );
  _wireframe = !_wireframe;

  if ( _wireframe )
  {
    glEnable( GL_POLYGON_OFFSET_LINE );
    glPolygonOffset( -1 , -1 );
    glLineWidth( 1.5 );
    glPolygonMode( GL_FRONT_AND_BACK , GL_LINE );
  }
  else
  {
    glPolygonMode( GL_FRONT_AND_BACK , GL_FILL );
    glDisable( GL_POLYGON_OFFSET_LINE );
  }

  update( );
}

void OpenGLWidget::timerUpdate( )
{
  if ( _camera->isAniming( ))
  {
    _camera->anim( );
    this->update( );
  }
}

void OpenGLWidget::extractEditNeuronMesh( )
{
  if ( _scene->isEditNeuronMeshExtraction( ))
  {
    QString path;
    const QString filter( tr( "OBJ (*.obj);; All files (*)" ));
    QFileDialog fd( this , QString( "Save mesh to OBJ file" ) ,
                    _lastSavedFileName , filter );

    fd.setOption( QFileDialog::DontUseNativeDialog , true );
    fd.setDefaultSuffix( "obj" );
    fd.setFileMode( QFileDialog/*::FileMode*/::AnyFile );
    fd.setAcceptMode( QFileDialog/*::AcceptMode*/::AcceptSave );
    if ( fd.exec( ))
      path = fd.selectedFiles( )[ 0 ];

    if ( !path.isEmpty( ))
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
  if ( _scene )
  {
    _scene->levelOfDetail( static_cast<float>(value_) * 0.1f );
    update( );
  }
}

void OpenGLWidget::onDistanceValueChanged( int value_ )
{
  if ( _scene )
  {
    _scene->maximumDistance( static_cast<float>(value_) / 1000.0f );
    update( );
  }
}

void OpenGLWidget::onHomogeneousClicked( )
{
  if ( _scene )
  {
    _scene->subdivisionCriteria( nlrender::Renderer::HOMOGENEOUS );
    update( );
  }
}

void OpenGLWidget::onLinearClicked( )
{
  if ( _scene )
  {
    _scene->subdivisionCriteria( nlrender::Renderer::LINEAR );
    update( );
  }
}

void OpenGLWidget::changeNeuronPiece( int index_ )
{
  if ( !_scene ) return;
  switch ( index_ )
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
  if ( !_scene ) return;
  switch ( index_ )
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

void OpenGLWidget::initializeGL( )
{
  initializeOpenGLFunctions( );
  glEnable( GL_DEPTH_TEST );
  glClearColor( 1.0f , 1.0f , 1.0f , 1.0f );
  glPolygonMode( GL_FRONT_AND_BACK , GL_FILL );
  glEnable( GL_CULL_FACE );

  glLineWidth( 1.5 );

  QOpenGLWidget::initializeGL( );

  const GLubyte* vendor = glGetString( GL_VENDOR ); // Returns the vendor
  const GLubyte* renderer = glGetString(
    GL_RENDERER ); // Returns a hint to the model
  const GLubyte* version = glGetString( GL_VERSION );
  const GLubyte* shadingVer = glGetString( GL_SHADING_LANGUAGE_VERSION );

  std::cout << "OpenGL Hardware: " << vendor << " (" << renderer << ")"
            << std::endl;
  std::cout << "OpenGL Version: " << version << " (shading ver. " << shadingVer
            << ")" << std::endl;

  nlrender::Config::init( );
}

void OpenGLWidget::paintGL( )
{
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  if ( _scene != nullptr )
  {
    _scene->update();
    _scene->render( );
  }

  glUseProgram( 0 );
  glFlush( );

#define FRAMES_PAINTED_TO_MEASURE_FPS 10
  if ( _frameCount % FRAMES_PAINTED_TO_MEASURE_FPS == 0 )
  {
    const auto now = std::chrono::system_clock::now( );

    const auto duration =
      std::chrono::duration_cast< std::chrono::milliseconds >( now - _then );
    _then = now;

    auto mainWindow = dynamic_cast< MainWindow* >( parent( ));
    if ( mainWindow )
    {
      const unsigned int ellapsedMiliseconds = duration.count( );

      const auto fps = static_cast<unsigned int>(roundf(
        1000.0f *
        static_cast<float>( FRAMES_PAINTED_TO_MEASURE_FPS ) /
        static_cast<float>( ellapsedMiliseconds ))
      );

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
  _camera->windowSize( width_ , height_ );
  glViewport( 0 , 0 , width_ , height_ );
}

void OpenGLWidget::mousePressEvent( QMouseEvent* event_ )
{
  if ( event_->button( ) == Qt::LeftButton )
  {
    _rotation = true;
    _mouseX = event_->x( );
    _mouseY = event_->y( );
  }

  if ( event_->button( ) == Qt::MidButton )
  {
    _translation = true;
    _mouseX = event_->x( );
    _mouseY = event_->y( );
  }
}

void OpenGLWidget::mouseReleaseEvent( QMouseEvent* event_ )
{
  if ( event_->button( ) == Qt::LeftButton )
  {
    _rotation = false;
  }

  if ( event_->button( ) == Qt::MidButton )
  {
    _translation = false;
  }
}

void OpenGLWidget::mouseMoveEvent( QMouseEvent* event_ )
{
  constexpr float TRANSLATION_FACTOR = 0.001f;
  constexpr float ROTATION_FACTOR = 0.01f;

  const auto diffX = static_cast<float>(event_->x( ) - _mouseX);
  const auto diffY = static_cast<float>(event_->y( ) - _mouseY);

  auto updateLastEventCoords = [ this ]( const QMouseEvent* e )
  {
    _mouseX = e->x( );
    _mouseY = e->y( );
  };

  if ( _rotation )
  {
    _camera->rotate(
      Eigen::Vector3f(
        diffX * ROTATION_FACTOR ,
        diffY * ROTATION_FACTOR ,
        0.0f
      )
    );

    updateLastEventCoords( event_ );
  }

  if ( _translation )
  {
    const float xDis = diffX * TRANSLATION_FACTOR * _camera->radius( );
    const float yDis = diffY * TRANSLATION_FACTOR * _camera->radius( );

    _camera->localTranslate( Eigen::Vector3f( -xDis , yDis , 0.0f ));
    updateLastEventCoords( event_ );
  }

  update( );
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
      _camera->position( Eigen::Vector3f( 0.f , 0.f , 0.f ));
      _camera->radius( 1000.0f );
      _camera->rotation( Eigen::Vector3f{ 0.f , 0.f , 0.f } );
      update( );
      break;
  }
}

void OpenGLWidget::changeClearColor( QColor qColor )
{
  makeCurrent( );
  glClearColor( qColor.redF( ) , qColor.greenF( ) , qColor.blueF( ) , 1.0f );
  update( );
}

void OpenGLWidget::changeNeuronColor( QColor qColor )
{
  _scene->unselectedNeuronColor( qColor );
  update( );
}

void OpenGLWidget::changeSelectedNeuronColor( QColor qColor )
{
  _scene->selectedNeuronColor( qColor );
  update( );
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
