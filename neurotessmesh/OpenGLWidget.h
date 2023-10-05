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

#include <Eigen/Eigen>

#include <chrono>
#include <memory>
#include <iostream>

#ifdef NEUROTESSMESH_USE_LEXIS
#include <zeroeq/zeroeq.h>
#include <thread>
#include <lexis/lexis.h>
#ifdef NEUROTESSMESH_USE_GMRVLEX
#include <gmrvlex/gmrvlex.h>
#endif
#endif

namespace neurotessmesh
{
  class Scene;
}

namespace reto
{
  class OrbitalCameraController;
}

namespace nsol
{
  class DataSet;
}

struct streamDotSeparator : std::numpunct< char >
{
  char do_decimal_point( ) const override
  { return '.'; }
};

/** \class CameraPosition
 * \brief Implements a structure to serialize and store camera positions.
 *
 */
class CameraPosition
{
public:
  Eigen::Vector3f position; /** position point.  */
  Eigen::Matrix3f rotation; /** rotation matrix. */
  float radius;             /** aperture.        */

  /** \brief CameraPosition class constructor.
   *
   */
  CameraPosition( )
    : position{ Eigen::Vector3f( ) }
    , rotation{ Eigen::Matrix3f::Zero( ) }
    , radius{ 0 }
  { };

  /** \brief CameraPosition class constructor.
   * \param[in] data Camera position serialized data.
   *
   */
  explicit CameraPosition( const QString& data )
  {
    const auto separator = std::use_facet< std::numpunct< char>>(
      std::cout.getloc( )).decimal_point( );
    const bool needSubst = ( separator == ',' );

    auto parts = data.split( ";" );
    Q_ASSERT( parts.size( ) == 3 );
    const auto posData = parts.first( );
    const auto rotData = parts.last( );
    auto radiusData = parts.at( 1 );

    auto posParts = posData.split( "," );
    Q_ASSERT( posParts.size( ) == 3 );
    auto rotParts = rotData.split( "," );
    Q_ASSERT( rotParts.size( ) == 9 );

    if ( needSubst )
    {
      for ( auto& part: posParts ) part.replace( '.' , ',' );
      for ( auto& part: rotParts ) part.replace( '.' , ',' );
      radiusData.replace( '.' , ',' );
    }

    position = Eigen::Vector3f( posParts[ 0 ].toFloat( ) ,
                                posParts[ 1 ].toFloat( ) ,
                                posParts[ 2 ].toFloat( ));
    radius = radiusData.toFloat( );
    rotation.block< 1 , 3 >( 0 , 0 ) = Eigen::Vector3f{
      rotParts[ 0 ].toFloat( ) , rotParts[ 1 ].toFloat( ) ,
      rotParts[ 2 ].toFloat( ) };
    rotation.block< 1 , 3 >( 1 , 0 ) = Eigen::Vector3f{
      rotParts[ 3 ].toFloat( ) , rotParts[ 4 ].toFloat( ) ,
      rotParts[ 5 ].toFloat( ) };
    rotation.block< 1 , 3 >( 2 , 0 ) = Eigen::Vector3f{
      rotParts[ 6 ].toFloat( ) , rotParts[ 7 ].toFloat( ) ,
      rotParts[ 8 ].toFloat( ) };
  }

  /** \brief Returns the serialized camera position.
   *
   */
  QString toString( ) const
  {
    std::stringstream stream;
    stream.imbue( std::locale( stream.getloc( ) , new streamDotSeparator( )));
    stream << position << ";" << radius << ";"
           << rotation( 0 , 0 ) << "," << rotation( 0 , 1 ) << ","
           << rotation( 0 , 2 ) << ","
           << rotation( 1 , 0 ) << "," << rotation( 1 , 1 ) << ","
           << rotation( 1 , 2 ) << ","
           << rotation( 2 , 0 ) << "," << rotation( 2 , 1 ) << ","
           << rotation( 2 , 2 );

    auto serialization = QString::fromStdString( stream.str( ));
    serialization.replace( '\n' , ',' ).remove( ' ' );

    return serialization;
  }
};

class OpenGLWidget
  : public QOpenGLWidget , public QOpenGLFunctions
{

Q_OBJECT;

public:

  explicit OpenGLWidget( QWidget* parent_ = nullptr ,
                         Qt::WindowFlags windowFlags_ = Qt::WindowFlags( ));

  ~OpenGLWidget( ) override;

  void setScene( std::shared_ptr< neurotessmesh::Scene > scene );

  void idleUpdate( bool idleUpdate_ = true );

  void home( );

  void setZeqSession( const std::string& session_ );

  reto::OrbitalCameraController* getCamera( ) const;

  /** \brief Returns the current camera position.
   *
   */
  CameraPosition cameraPosition( ) const;

public slots:

  void toggleUpdateOnIdle( );

  void toggleShowFPS( );

  void toggleWireframe( );

  void timerUpdate( );

  void extractEditNeuronMesh( );

  void onLotValueChanged( int value_ );

  void onDistanceValueChanged( int value_ );

  void onHomogeneousClicked( );

  void onLinearClicked( );

  void changeNeuronPiece( int index_ );

  void changeSelectedNeuronPiece( int index_ );

  void changeClearColor( QColor );

  void changeNeuronColor(const int type, const QColor &color);

protected:

#ifdef NEUROTESSMESH_USE_LEXIS
  void _onSelectionEvent( lexis::data::ConstSelectedIDsPtr selectedIndices_ );

#ifdef NEUROTESSMESH_USE_GMRVLEX
  void _onFocusEvent( zeroeq::gmrv::ConstFocusedIDsPtr focusIndices_ );
#endif

#endif

  void initializeGL( ) override;

  void paintGL( ) override;

  void resizeGL( int width_ , int height_ ) override;

  void mousePressEvent( QMouseEvent* event_ ) override;

  void mouseReleaseEvent( QMouseEvent* event_ ) override;

  void wheelEvent( QWheelEvent* event_ ) override;

  void mouseMoveEvent( QMouseEvent* event_ ) override;

  void keyPressEvent( QKeyEvent* event_ ) override;

  std::shared_ptr< neurotessmesh::Scene > _scene;
  reto::OrbitalCameraController* _camera;

  int _mouseX , _mouseY;
  bool _rotation , _translation;
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

#ifdef NEUROTESSMESH_USE_LEXIS
  zeroeq::Subscriber* _subscriber;

  std::thread* _subscriberThread;
#endif

};

#endif // __NEUROTESSMESH__OPENGLWIDGET__
