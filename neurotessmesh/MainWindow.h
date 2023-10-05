/**
 * @file    MainWindow.h
 * @brief
 * @author  Juan José García <juanjose.garcia@urjc.es>,
 * Pablo Toharia <pablo.toharia@urjc.es>
 * @date    2015
 * @remarks Copyright (c) 2015 VG-Lab/URJC. All rights reserved.
 * Do not distribute without further notice.
 */

#include "ui_mainwindow.h"
#include <QMainWindow>
#include "OpenGLWidget.h"
#include "ColorSelectionWidget.h"
#include "LoaderThread.h"
#include <set>

#include <QDockWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QCheckBox>

constexpr int ID_ROLE = Qt::UserRole +1;
constexpr int COLOR_ROLE = Qt::UserRole +2;
constexpr int TEXT_ROLE = Qt::UserRole +3;

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

public slots:

  /** \brief Updates the neurons list and returns the coloring values used
   * for the current coloring method.
   *
   */
  std::set<int> updateNeuronList( );

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

  void onColoringChanged(int index);

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

  /** \brief Updates the neuron color changed by the user.
   * \param[in] color New neuron color.
   *
   */
  void changeNeuronColor(QColor color);

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

  QComboBox* _renderColoring;
  QCheckBox* _neuronAdditionalText;
  QGridLayout *_colorLayout;

  // Recorder
  Recorder* _recorder;
  std::shared_ptr< neurotessmesh::LoaderThread > m_dataLoader;
};

/** \class NeuronListItem
 * \brief List item for the neuron list widget.
 *
 */
class NeuronListItem
: public QListWidgetItem
{
  public:
    /** \brief NeuronListItem class constructor.
     * \param[in] id Id number of the neuron in the dataset.
     * \param[in] text Text to show in the item.
     * \param[in] color Background color.
     *
     */
    explicit NeuronListItem(uint32_t id, QString text = QString(), QColor color = QColor())
    : QListWidgetItem()
    , m_id{id}
    , m_text{text}
    , m_color{color}
    {}

    virtual QVariant data(int role) const override
    {
      switch(role)
      {
        case Qt::DisplayRole:
          return QString::number(m_id) + " " + m_text;
          break;
        case Qt::BackgroundColorRole:
          if(m_color != QColor()) return m_color;
          break;
        case Qt::TextColorRole:
          if(m_color != QColor())
          { // try to return a color with a lot of contrast with the background
            const auto a = 1 - ( 0.299 * m_color.redF() + 0.587 * m_color.greenF() + 0.114 * m_color.blueF());
            return (a <= 0.3) ? QColor(0,0,0) : QColor(255,255,255);
          }
          break;
        case ID_ROLE:
          return m_id;
          break;
        case COLOR_ROLE:
          return m_color.name();
          break;
        case TEXT_ROLE:
          return m_text;
          break;
        default:
          break;
      }

      return QListWidgetItem::data(role);
    }

    /** \brief NeuronListItem class virtual destructor.
     *
     */
    virtual ~NeuronListItem() {};

    /** \brief To sort the items in the list.
     *
     */
    bool operator <(const QListWidgetItem &other) const override
    {
      return this->m_id < static_cast<const NeuronListItem&>(other).m_id;
    }

    uint32_t m_id;  /** neuron id in the dataset. */
    QString m_text; /** text to show. */
    QColor m_color; /** background color. */
};

