# CMake build control file for ColladaOgreImporter
# by Jeff Trull <jetrull@sbcglobal.net> and released under the MIT License; see LICENSE.txt for details

# Example build procedure (Ubuntu):
# mkdir build;cd build;cmake -DOGRE_HOME=/usr/share/OGRE -DCOLLADA_ROOT=/home/me/opencollada-read-only/ ..;make

# Requires the library OpenCollada;  there doesn't seem to be an Ubuntu package so I do this (from their web page):
# svn checkout http://opencollada.googlecode.com/svn/trunk/ opencollada-read-only
# cd opencollada-read-only;mkdir build;cd build;cmake ..;make

cmake_minimum_required(VERSION 2.8)

# We need the OpenCollada library.  Users can supply COLLADA_ROOT as a hint
# includes
find_path(COLLADAFW_INCLUDE_DIR COLLADAFWRoot.h HINTS ${COLLADA_ROOT}/COLLADAFramework/include)
find_path(COLLADASAX_INCLUDE_DIR COLLADASaxFWLLoader.h HINTS ${COLLADA_ROOT}/COLLADASaxFrameworkLoader/include)
find_path(COLLADABASE_INCLUDE_DIR COLLADABUPlatform.h HINTS ${COLLADA_ROOT}/COLLADABaseUtils/include)
# libraries
find_library(COLLADABU_LIB OpenCOLLADABaseUtils HINTS ${COLLADA_ROOT}/build/lib)
find_library(COLLADAFW_LIB OpenCOLLADAFramework HINTS ${COLLADA_ROOT}/build/lib)
find_library(COLLADASAX_LIB OpenCOLLADASaxFrameworkLoader HINTS ${COLLADA_ROOT}/build/lib)
find_library(COLLADASAXP_LIB GeneratedSaxParser HINTS ${COLLADA_ROOT}/build/lib)
find_library(UTF_LIB UTF HINTS ${COLLADA_ROOT}/build/lib)
find_library(MATHML_LIB MathMLSolver ${COLLADA_ROOT}/build/lib)

# fail if not found
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenCollada REQUIRED_VARS
                                  COLLADAFW_INCLUDE_DIR COLLADASAX_INCLUDE_DIR COLLADABASE_INCLUDE_DIR
				  COLLADABU_LIB COLLADAFW_LIB COLLADASAX_LIB COLLADASAXP_LIB
				  UTF_LIB MATHML_LIB)

# find Ogre.  Unfortunately FindOGRE.cmake is not included as part of CMake, but as part of Ogre instead,
# so you have to know where Ogre is in order to find it...

if ((DEFINED OGRE_HOME) AND (EXISTS ${OGRE_HOME}/cmake/modules))
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${OGRE_HOME}/cmake/modules)
elseif((DEFINED $ENV{OGRE_HOME}) AND (EXISTS $ENV{OGRE_HOME}/cmake/modules))
  # Ogre's cmake script also accepts an environment variable of the same name
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{OGRE_HOME}/cmake/modules)
endif()

find_package(Boost REQUIRED system filesystem)

find_package(OGRE 1.7.3 REQUIRED)

include_directories(SYSTEM ${COLLADAFW_INCLUDE_DIR} ${COLLADASAX_INCLUDE_DIR} ${COLLADABASE_INCLUDE_DIR} ${OGRE_INCLUDE_DIRS})

add_executable(c2mesh collada2ogre.cpp OgreMeshWriter.cpp OgreColladaWriter.cpp)
# assume we have xml2 and pcre (definitely available on Ubuntu) but others come from OpenCollada dir
target_link_libraries(c2mesh ${OGRE_LIBRARIES} ${Boost_LIBRARIES} ${COLLADASAX_LIB} ${COLLADASAXP_LIB} ${COLLADAFW_LIB} ${COLLADABU_LIB} ${UTF_LIB} xml2 pcre ${MATHML_LIB})
# BOZO separate into libs?
