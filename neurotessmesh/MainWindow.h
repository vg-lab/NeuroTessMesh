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

class MainWindow
  : public QMainWindow
{
  Q_OBJECT

public:

  explicit MainWindow( QWidget* parent_ = 0,
                       bool updateOnIdle_ = false );
  ~MainWindow( void );

  void init( const std::string& zeqSession_ );

  void showStatusBarMessage ( const QString& message );

  void openBlueConfig( const std::string& fileName,
                       const std::string& target);
  void openXMLScene( const std::string& fileName );
  void openSWCFile( const std::string& fileName );

  void updateNeuronList( void );

public slots:

  void home( void );
  void openBlueConfigThroughDialog( void );
  void openXMLSceneThroughDialog( void );
  void openSWCFileThroughDialog( void );
  void showAbout( void );
  void openRecorder( void );

  void updateExtractMeshDock( void );
  void updateConfigurationDock( void );
  void updateRenderOptionsDock( void );
  void onListClicked( QListWidgetItem *item );
  void onActionGenerate( int value_ );

protected slots:

  void finishRecording( );

protected:

  QString _lastOpenedFileName;

private:

  void _generateNeuritesLayout( void );
  void _initExtractionDock( void );
  void _initConfigurationDock( void );
  void _initRenderOptionsDock( void );

  Ui::MainWindow* _ui;
  OpenGLWidget* _openGLWidget;

  QDockWidget* _extractMeshDock;
  QDockWidget* _configurationDock;
  QDockWidget* _renderOptionsDock;

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
};
