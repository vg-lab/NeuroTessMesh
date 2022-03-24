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

#ifdef NEUROTESSMESH_USE_LEXIS
#include <zeroeq/zeroeq.h>
#include <thread>
#include <lexis/lexis.h>
#ifdef NEUROTESSMESH_USE_GMRVLEX
#include <gmrvlex/gmrvlex.h>
#endif
#endif

class OpenGLWidget
  : public QOpenGLWidget
  , public QOpenGLFunctions
{

  Q_OBJECT;

public:

  OpenGLWidget( QWidget* parent_ = nullptr,
                Qt::WindowFlags windowFlags_ = Qt::WindowFlags() );

  ~OpenGLWidget( void );

  /** \brief Loads a dataset with the given parameters.
   * \param[in] fileName_ Dataset filename.
   * \param[in] fileType_ Dataset type.
   * \param[in] target_ BlueConfig target if Blueconfig or empty otherwise.
   *
   */
  bool loadData( const std::string& fileName_,
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

  void setZeqSession( const std::string& session_ );

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

#ifdef NEUROTESSMESH_USE_LEXIS
  void _onSelectionEvent( lexis::data::ConstSelectedIDsPtr selectedIndices_ );

#ifdef NEUROTESSMESH_USE_GMRVLEX
  void _onFocusEvent( zeroeq::gmrv::ConstFocusedIDsPtr focusIndices_ );
#endif

#endif

  virtual void initializeGL( void );
  virtual void paintGL( void );
  virtual void resizeGL( int width_, int height_ );

  virtual void mousePressEvent( QMouseEvent* event_ );
  virtual void mouseReleaseEvent( QMouseEvent* event_ );
  virtual void wheelEvent( QWheelEvent* event_ );
  virtual void mouseMoveEvent( QMouseEvent* event_ );
  virtual void keyPressEvent( QKeyEvent* event_ );

  reto::OrbitalCameraController* _camera;
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

#ifdef NEUROTESSMESH_USE_LEXIS
    zeroeq::Subscriber* _subscriber;

    std::thread* _subscriberThread;
#endif

};

#endif // __NEUROTESSMESH__OPENGLWIDGET__
