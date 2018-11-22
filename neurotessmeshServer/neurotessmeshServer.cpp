#include <iostream>
#include <boost/filesystem.hpp>

#include <nlgeometry/nlgeometry.h>
#include <nlgenerator/nlgenerator.h>
#include <nlrender/nlrender.h>
#include <reto/reto.h>
#include <nsol/nsol.h>

#include <neurotessmeshServer/version.h>

//OpenGL
#ifndef NEUROLOTS_SKIP_GLEW_INCLUDE
  #include <GL/glew.h>
#endif
#ifdef Darwin
  #define __gl_h_
  #define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
  #include <GL/freeglut.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
  #include <GL/freeglut.h>
#endif

void helpMessage( const std::string& appName_ )
{
  std::cerr << "Usage:\n\n" << appName_ << "[options] morphology_files[.swc]\n"
            << "  Options:\n\n    -l [float] sets the level of subdivisiones "
            << "per unit of measure for the output mesh.\n    -f [obj|off] sets"
            << " the output format file: obj or off file format" << std::endl;
}

void errorMessage( const std::string& appName_ )
{
  std::cerr << "Error ";
  helpMessage( appName_ );
}

void initContext( int argc, char* argv[ ])
{
  glutInit( &argc, argv );
  glutInitContextVersion( 4, 0 );

  glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );
  glutInitWindowSize( 600, 600 );
  glutInitWindowPosition( 0, 0 );
  glutCreateWindow( "" );

  glewExperimental = GL_TRUE;
  glewInit( );
}

int main( int argc, char** argv )
{
  int filesStart = 1;
  float lod = 1.0f;
  unsigned int outFormat = 0;
  std::string appName( argv[0] );
  for ( int i = 1; i < argc; i++ )
  {
    std::string option( argv[i]);
    try
    {
      if ( option.compare( "-h" ) == 0 || option.compare( "--help" ) == 0 )
      {
        helpMessage( appName );
        return 0;
      }
      else if ( option.compare( "-l" ) == 0 )
      {
        lod = std::atof( argv[i+1] );
        ++i;
        filesStart += 2;
      }
      else if ( option.compare( "-f" ) == 0 )
      {
        std::string outFormatOption( argv[i+1] );
        ++i;
        filesStart += 2;
        if ( outFormatOption.compare( "obj" ) == 0 )
        {
          outFormat = 0;
        }
        else if ( outFormatOption.compare( "off" ) == 0 )
        {
          outFormat = 1;
        }
      }
    }
    catch( ... )
    {
      errorMessage( appName );
      return 1;
    }
  }

  if ( filesStart >= argc )
  {
    errorMessage( appName );
    return 1;
  }

  initContext( argc, argv );

  nlrender::Renderer renderer;
  renderer.lod( ) = lod;
  nlgeometry::MeshPtr mesh;
  nsol::NeuronMorphologyPtr morphology;
  nlgeometry::AttribsFormat format( 3 );
  format[0] = nlgeometry::TAttribType::POSITION;
  format[1] = nlgeometry::TAttribType::CENTER;
  format[2] = nlgeometry::TAttribType::TANGENT;

  nsol::SwcReader swcr;

  for ( int i = filesStart; i < argc; i++ )
  {
    std::string inFile( argv[i] );

    try{
      morphology = swcr.readMorphology( inFile );
      mesh = nlgenerator::MeshGenerator::generateMesh( morphology );
      mesh->uploadGPU( format, nlgeometry::Facet::PATCHES );
      std::string originalFile =
        boost::filesystem::path( inFile ).filename( ).string( );
      std::string header(
        "#Mesh generated with neurotessmeshServer " +
        std::to_string( neurotessmeshServer::Version::getMajor( )) + "." +
        std::to_string( neurotessmeshServer::Version::getMinor( )) + "." +
        std::to_string( neurotessmeshServer::Version::getPatch( )) +
        " application from the GMRV/URJC \n"
        "#Contact: juanjose.garcia@urjc.es\n"
        "#Generated from: " + originalFile + "\n"
        "#Level of subdivision applied: " + std::to_string( lod ) );
      if ( outFormat == 0 )
      {
        std::string outFile = boost::filesystem::path( inFile
          ).replace_extension( "obj" ).string( );
        nlgeometry::ObjWriter::writeMesh(
          renderer.extract( mesh, mesh->modelMatrix( )), outFile, header );
      }
      else if ( outFormat == 1 )
      {
        std::string outFile = boost::filesystem::path( inFile
          ).replace_extension( "off" ).string( );
        nlgeometry::OffWriter::writeMesh(
          renderer.extract( mesh, mesh->modelMatrix( )), outFile, header );
      }
      delete mesh;
      delete morphology;
    }
    catch( ... )
    {
      std::cerr << "Error loading " << inFile << std::endl;
    }
  }


  return 0;
}
