/**
 * Copyright (c) 2015-2017 GMRV/URJC.
 *
 * Authors: Juan Jose Garcia Cantero <juanjose.garcia@urjc.es>
 *
 * This file is part of neurolots <https://github.com/gmrvvis/neurolots>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#ifndef __NEUROTESSMESH_SCENE__
#define __NEUROTESSMESH_SCENE__

#include <nsol/nsol.h>
#include <reto/reto.h>
#include <nlgeometry/nlgeometry.h>
#include <nlrender/nlrender.h>

#include <neurotessmesh/api.h>

namespace neurotessmesh
{

  /* \class Scene */
  class Scene
  {

  public:

    typedef enum
    {
      VISUALIZATION = 0,
      EDITION
    }TSceneMode;

    typedef enum
    {
      BlueConfig,
      SWC,
      NsolScene
    } TDataFileType;

    typedef std::tuple< nlgeometry::Meshes, std::vector< Eigen::Matrix4f >>
      NeuronMeshes;

    /**
     * Default constructor
     */
    NEUROTESSMESH_API
    Scene( reto::OrbitalCameraController* camera_ );

    /**
     * Default destructor
     */
    NEUROTESSMESH_API
    ~Scene( void );

    /**
     * Method to set the scene mode
     * @param mode_ new scnene mode
     */
    NEUROTESSMESH_API
    void mode( const TSceneMode mode_ );

    /**
     * Method to get the scene mode
     * @return the current scene mode
     */
    NEUROTESSMESH_API
    TSceneMode mode( void ) const;

    /**
     * Method to renderize the scene based on the current mode
     */
    NEUROTESSMESH_API
    void render( void );

    /**
     * Method to close and deleted data from dataSet
     */
    NEUROTESSMESH_API
    void close( void );

    /**
     * Method to set the scene params to default
     */
    NEUROTESSMESH_API
    void home( void );

    /**
     * Method to animate the camera to the given position, radius and rotation
     */
    NEUROTESSMESH_API
    void cameraPosition(const Eigen::Vector3f &position, const float radius, const Eigen::Matrix3f &rotation);

    /**
     * Method to compute axis align bounding box
     * @param indices_ vector of indices to compute the bounding box
     * @return axis align bounding box
     */
    NEUROTESSMESH_API
    nlgeometry::AxisAlignedBoundingBox
    computeBoundingBox( std::vector< unsigned int > indices_ );

    /**
     * Method to compute axis align bounding box
     * @return axis align bounding box for all the current neurons
     */
    NEUROTESSMESH_API
    nlgeometry::AxisAlignedBoundingBox computeBoundingBox( void );

    /**
     * Method to generate the meshes associated to the loaded neurons
     */
    NEUROTESSMESH_API
    void generateMeshes( void );

    /**
     * Method to load neurons data returns an empty string on success
     * or an error string otherwise.
     * @param fileName path to the file
     * @param fileType type of file
     * @param target to load, specific param to BlueConfig data
     */
    NEUROTESSMESH_API
    std::string loadData( const std::string& fileName_,
                   const TDataFileType fileType_,
                   const std::string& target_ = std::string( "" ));

    /**
     * Method to set the render options of unseletected and selected neurons
     * @param paint_ option of neuron render
     */
    NEUROTESSMESH_API
    void paintUnselectedSoma( bool paint_ );
    NEUROTESSMESH_API
    void paintUnselectedNeurites( bool paint_ );
    NEUROTESSMESH_API
    void paintSelectedSoma( bool paint_ );
    NEUROTESSMESH_API
    void paintSelectedNeurites( bool paint_ );

    /**
     * Method to change the unselected neuron color
     * @param color_ color of the unselected neuron
     */
    NEUROTESSMESH_API
    void unselectedNeuronColor( Eigen::Vector3f color_ );

    /**
     * Method to change the selected neuron color
     * @param color_ color of the selected neuron
     */
    NEUROTESSMESH_API
    void selectedNeuronColor( Eigen::Vector3f color_ );

    /**
     * Method to set the scene level of subdivision
     * @param lod_ scene level of detail
     */
    NEUROTESSMESH_API
    void levelOfDetail( float lod_ );

    /**
     * Method to set the scene maximum subdivision distance
     * @param maximumDistance_ scene maximum subdivision distance
     */
    NEUROTESSMESH_API
    void maximumDistance( float maximumDistance_ );

    /**
     * Method to set the scene subidivision criteria
     * @param subidivisionCriteria_ scene subdivision criteria
     */
    NEUROTESSMESH_API
    void subdivisionCriteria( nlrender::Renderer::TTessCriteria
                              subdivisionCriteria_ );

    NEUROTESSMESH_API
    std::vector< unsigned int > neuronIndices( void );

    NEUROTESSMESH_API
    void setNeuronToEdit( unsigned int id_ );

    NEUROTESSMESH_API
    unsigned int numEditMorphologyNeurites( void ) const;

    NEUROTESSMESH_API
    void regenerateEditNeuronMesh( const float alphaRadius,
                                   const std::vector< float >& alphaNeurites_ );

    NEUROTESSMESH_API
    bool isEditNeuronMeshExtraction( void );

    NEUROTESSMESH_API
    void extractEditNeuronMesh( const std::string& path_ );

    NEUROTESSMESH_API
    void conformRenderTuples( void );

    NEUROTESSMESH_API
    void changeSelectedIndices( const std::vector< unsigned int >& indices_ );

    NEUROTESSMESH_API
    void focusOnIndices( const std::vector< unsigned int >& indices_ );
  protected:
    /** \brief Animates the camera to the given position and radius.
     * \param[in] position Focus position.
     * \param[in] radius Aperture radius.
     *
     */
    void animate(const Eigen::Vector3f &position, const float radius);

    /** \brief Animates the camera to the given position, radius and rotation.
     * \param[in] position Focus position.
     * \param[in] radius Aperture radius.
     * \param[in] rotation Camera rotation matrix.
     */
    void animatePosition(const Eigen::Vector3f &position, const float radius, const Eigen::Matrix3f &rotation, bool rotAnimation);

    //! Scene mode
    TSceneMode _mode;

    //! Scene camera
    reto::OrbitalCameraController* _camera;
    reto::CameraAnimation *_animation;

    //! Neurolots engine to render morphological data
    nlrender::Renderer* _renderer;

    //! Meshes attribs format
    nlgeometry::AttribsFormat _attribsFormat;

    //! Unselected neuron color
    Eigen::Vector3f _unselectedColor;

    //! Selected neuron color
    Eigen::Vector3f _selectedColor;

    //! Nsol DataSet, contains neurons
    nsol::DataSet* _dataSet;

    //! Neuron Meshes
    std::unordered_map< nsol::MorphologyPtr, nlgeometry::MeshPtr >
    _neuronMeshes;

    //! Unselected neuron meshes
    NeuronMeshes _unselectedNeurons;

    //! Selected neuron meshes
    NeuronMeshes _selectedNeurons;

    //! List of selected indices
    std::set< unsigned int > _selectedIndices;

    //! Render opttions to unselected and selected neurons
    bool _paintUnselectedSoma;
    bool _paintUnselectedNeurites;
    bool _paintSelectedSoma;
    bool _paintSelectedNeurites;

    //! Neuron to be edited
    nsol::NeuronPtr _editNeuron;

    //! Neuron mesh to be edited
    nlgeometry::MeshPtr _editMesh;

    //! Scene bonunding box
    nlgeometry::AxisAlignedBoundingBox _boundingBox;
  };

}

#endif // __NEUROTESSMESH_SCENE__
