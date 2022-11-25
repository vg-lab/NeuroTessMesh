/**
 * @file    MainWindow.h
 * @brief
 * @author  Juan José García <juanjose.garcia@urjc.es>,
 * Pablo Toharia <pablo.toharia@urjc.es>
 * @date    2015
 * @remarks Copyright (c) 2015 GMRV/URJC. All rights reserved.
 * Do not distribute without further notice.
 */

#include "ui_mainwindow.h"
#include <QMainWindow>
#include "OpenGLWidget.h"
#include "ColorSelectionWidget.h"
#include "LoaderThread.h"

#include <QDockWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QRadioButton>
#include <QGroupBox>

namespace Ui
{
  class MainWindow;
}

class Recorder;

class QCloseEvent;

namespace neurotessmesh
{
  class LoaderThread;
}

class MainWindow
  : public QMainWindow
{
Q_OBJECT

public:

  explicit MainWindow( QWidget* parent_ = nullptr ,
                       bool updateOnIdle_ = false );

  ~MainWindow( );

  void init( const std::string& zeqSession_ );

  void showStatusBarMessage( const QString& message );

  void openBlueConfig( const std::string& fileName ,
                       const std::string& target );

  void openXMLScene( const std::string& fileName );

  void openSWCFile( const std::string& fileName );

  void openHDF5File( const std::string& fileName );

  void updateNeuronList( );

public slots:

  void home( );

  void openBlueConfigThroughDialog( );

  void openXMLSceneThroughDialog( );

  void openSWCFileThroughDialog( );

  void openHDF5FileThroughDialog( );

  void showAbout( );

  void openRecorder( );

  void updateExtractMeshDock( );

  void updateConfigurationDock( );

  void updateRenderOptionsDock( );

  void updatePlayerOptionsDock( );

  void onListClicked( QListWidgetItem* item );

  void onActionGenerate( int value_ );

protected slots:

  void finishRecording( );

  /** \brief Loads camera positions from a file.
   *
   */
  void loadCameraPositions( );

  /** \brief Saves camera positions to a file on disk.
   *
   */
  void saveCameraPositions( );

  /** \brief Stores current camera position in the positions list.
   *
   */
  void addCameraPosition( );

  /** \brief Lets the user select a position to remove from the positions list.
   *
   */
  void removeCameraPosition( );

  /** \brief Changes the camera position to the one specified by the user.
   *
   */
  void applyCameraPosition( );

  /** \brief Gets the dataset and player from the loader thread and initializes
   * openGL widget and scene.
   *
   */
  void onDataLoaded( );

protected:
  virtual void closeEvent( QCloseEvent* e ) override;

  QString _lastOpenedFileName;

private:

  void _generateNeuritesLayout( );

  void _initExtractionDock( );

  void _initConfigurationDock( );

  void _initRenderOptionsDock( );

  void _initPlayerDock( );

  /** \brief Launches a LoaderThread with the gicen arguments.
   * \param[in] arg1 First argument,required: dataset filename.
   * \param[in] arg2 Second argument, only required for BlueConfig (target).
   * \param[in] type Dataset type.
   *
   */
  void loadData( const std::string& arg1 ,
                 const std::string& arg2 ,
                 neurotessmesh::LoaderThread::DataFileType type );

  Ui::MainWindow* _ui;
  OpenGLWidget* _openGLWidget;

  std::shared_ptr< neurotessmesh::Scene > _scene;

  QDockWidget* _extractMeshDock;
  QDockWidget* _configurationDock;
  QDockWidget* _renderOptionsDock;
  QDockWidget* _playerDock;

  QListWidget* _neuronList;
  QSlider* _radiusSlider;
  QVBoxLayout* _neuritesLayout;
  std::vector< QSlider* > _neuriteSliders;
  QGroupBox* _somaGroup;
  QPushButton* _extractButton;

  QSlider* _lotSlider;
  QSlider* _distanceSlider;

  QRadioButton* _radioHomogeneous;
  QRadioButton* _radioLinear;

  ColorSelectionWidget* _backGroundColor;
  ColorSelectionWidget* _neuronColor;
  ColorSelectionWidget* _selectedNeuronColor;

  QComboBox* _neuronRender;
  QComboBox* _selectedNeuronRender;

  // Recorder
  Recorder* _recorder;
  std::shared_ptr< neurotessmesh::LoaderThread > m_dataLoader;
};
