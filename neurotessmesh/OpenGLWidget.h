/**
 * @file    OpenGLWidget.h
 * @brief
 * @author  Juan José García <juanjose.garcia@urjc.es>,
 * Pablo Toharia <pablo.toharia@urjc.es>
 * @date    2015
 * @remarks Copyright (c) 2015 GMRV/URJC. All rights reserved.
 * Do not distribute without further notice.
 */

#ifndef __NEUROTESSMESH__OPENGLWIDGET__
#define __NEUROTESSMESH__OPENGLWIDGET__

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QLabel>
#include <QTimer>
#include <QColor>
#include <chrono>

#include <reto/reto.h>
#include "Scene.h"

class OpenGLWidget
  : public QOpenGLWidget
  , public QOpenGLFunctions
{

  Q_OBJECT;

public:

  OpenGLWidget( QWidget* parent_ = 0,
                Qt::WindowFlags windowFlags_ = 0 );

  ~OpenGLWidget( void );

  void loadData( const std::string& fileName_,
                 const neurotessmesh::Scene::TDataFileType fileType_ =
                 neurotessmesh::Scene::TDataFileType::BlueConfig,
                 const std::string& target_ = std::string( "" ));

  void idleUpdate( bool idleUpdate_ = true );

  const std::vector< unsigned int > neuronIdList( void ) const;

  void neuronToEdit( const unsigned int id_ );

  unsigned int numNeuritesToEdit( void ) const;

  void regenerateNeuronToEdit( const float alphaRadius_,
                               const std::vector< float >& alphaNeurites_ );

  void home( void );

public slots:

  void changeClearColor( QColor color );
  void changeNeuronColor( QColor color );
  void changeSelectedNeuronColor( QColor color );
  void toggleUpdateOnIdle( void );
  void toggleShowFPS( void );
  void toggleWireframe( void );
  void timerUpdate( void );
  void extractEditNeuronMesh( void );

  void onLotValueChanged( int value_ );
  void onDistanceValueChanged( int value_ );

  void onHomogeneousClicked( void );
  void onLinearClicked( void );

  void changeNeuronPiece( int index_ );
  void changeSelectedNeuronPiece( int index_ );

protected:

  virtual void initializeGL( void );
  virtual void paintGL( void );
  virtual void resizeGL( int width_, int height_ );

  virtual void mousePressEvent( QMouseEvent* event_ );
  virtual void mouseReleaseEvent( QMouseEvent* event_ );
  virtual void wheelEvent( QWheelEvent* event_ );
  virtual void mouseMoveEvent( QMouseEvent* event_ );
  virtual void keyPressEvent( QKeyEvent* event_ );

  reto::Camera* _camera;
  neurotessmesh::Scene* _scene;

  int _mouseX, _mouseY;
  bool _rotation, _translation;
  float _translationScale;
  float _rotationScale;

  bool _wireframe;
  bool _idleUpdate;

  QLabel _fpsLabel;
  bool _showFps;
  unsigned int _frameCount;

  QTimer* _cameraTimer;
  std::chrono::time_point< std::chrono::system_clock > _then;

  QString _lastSavedFileName;

  const static float _colorFactor;
};

#endif // __NEUROTESSMESH__OPENGLWIDGET__
