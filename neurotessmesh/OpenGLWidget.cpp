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
#include <sstream>
#include <string>
#include <iostream>

#include "MainWindow.h"
#include <nlrender/nlrender.h>

const float OpenGLWidget::_colorFactor = 1 / 255.0f;


OpenGLWidget::OpenGLWidget( QWidget* parent_,
                            Qt::WindowFlags windowsFlags_ )
  : QOpenGLWidget( parent_, windowsFlags_ )
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
{
  _camera = new reto::Camera( );
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
}

void OpenGLWidget::loadData(
  const std::string& fileName_,
  const neurotessmesh::Scene::TDataFileType fileType_,
  const std::string& target_ )
{
  makeCurrent( );
  _scene->loadData( fileName_, fileType_, target_ );
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
  if( _camera->anim( ))
    this->update( );
}

void OpenGLWidget::extractEditNeuronMesh( void )
{
  if( _scene->isEditNeuronMeshExtraction( ))
  {

    QString path;
    QString filter( tr( "OBJ (*.obj);; All files (*)" ));
    QFileDialog* fd = new QFileDialog( this,
                                       QString( "Save mesh to OBJ file" ),
                                       _lastSavedFileName,
                                       filter );

    fd->setOption( QFileDialog::DontUseNativeDialog, true );
    fd->setDefaultSuffix( "obj") ;
    fd->setFileMode( QFileDialog/*::FileMode*/::AnyFile );
    fd->setAcceptMode( QFileDialog/*::AcceptMode*/::AcceptSave );
    if ( fd->exec( ))
      path = fd->selectedFiles( )[0];

    if ( path != QString( "" ))
    {
      std::cout << "file " << path.toStdString( ) << std::endl;

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

    auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>( now - _then );
    _then = now;

    MainWindow* mainWindow = dynamic_cast< MainWindow* >( parent( ));
    if ( mainWindow )
    {

      unsigned int ellapsedMiliseconds = duration.count( );

      unsigned int fps = roundf( 1000.0f *
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
    // std::cout << _frameCount << std::endl;
    update( );
  }
  else
  {
    _fpsLabel.setVisible( false );
  }
}

void OpenGLWidget::resizeGL( int width_ , int height_ )
{
  _camera->ratio((( double ) width_ ) / height_ );
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
      _camera->localRotation( -( _mouseX - event_->x( )) * _rotationScale,
                              -( _mouseY - event_->y( )) * _rotationScale );

      _mouseX = event_->x( );
      _mouseY = event_->y( );
  }
  if( _translation )
  {
      float xDis = ( event_->x() - _mouseX ) * _translationScale;
      float yDis = ( event_->y() - _mouseY ) * _translationScale;

      _camera->localTranslation( Eigen::Vector3f( -xDis, yDis, 0.0f ));
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
    _camera->pivot( Eigen::Vector3f( 0.0f, 0.0f, 0.0f ));
    _camera->radius( 1000.0f );
    _camera->rotation( 0.0f, 0.0f );
    update( );
    break;
  }
}
