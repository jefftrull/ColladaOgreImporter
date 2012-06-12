// Test load of simple cube into Ogre scene via OgreSceneWriter
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2012 Jeffrey E Trull

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>

#include <boost/test/included/unit_test.hpp>

#include <fstream>
#include <COLLADAFWRoot.h>
#include <COLLADASaxFWLLoader.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>

#include "OgreSceneWriter.h"

// globals the test cases need
Ogre::SceneManager* sceneMgr;     // Ogre scene manager to use
std::string         fname;        // input (Collada .dae) filename

// This is the pattern for Boost test with a custom main
// See http://www.boost.org/doc/libs/1_49_0/libs/test/doc/html/utf/user-guide/initialization.html
boost::unit_test::test_suite* init_unit_test_suite(int argc, char *argv[]) {

  // set test suite name.  This would normally be the BOOST_TEST_MODULE parameter value
  boost::unit_test::framework::master_test_suite().p_name.value = "basic datamodel wrapper tests";

  // Expects just one argument: path to .dae file
  if (argc != 2) {
    std::string errstr("usage: cube_test /path/to/model.dae");
    throw boost::unit_test::framework::setup_error(errstr);
  }

  // make sure it's there
  fname = argv[1];
  if (!boost::filesystem::exists(fname) ||
      !boost::filesystem::is_regular_file(fname)) {
    std::string errstr("cannot access file: "); errstr += fname;
    throw boost::unit_test::framework::setup_error(errstr);
  }

  // create an Ogre root
  Ogre::Root* root = new Ogre::Root;
  root->setRenderSystem(root->getRenderSystemByName("OpenGL Rendering Subsystem"));
  root->initialise(false);      // we specify our own window
  root->getRenderSystem()->setConfigOption("RTT Preferred Mode", "PBuffer");  // bug workaround in nVidia drivers
  root->createRenderWindow("ignore me", 80, 80, false);
  sceneMgr = root->createSceneManager(Ogre::ST_GENERIC);

  return 0;
}

BOOST_AUTO_TEST_CASE( basic_contents ) {

  Ogre::SceneNode* topnode = sceneMgr->getRootSceneNode()->createChildSceneNode("Top");

  OgreSceneWriter writer(sceneMgr, topnode, ".");

  COLLADASaxFWL::Loader loader;
  COLLADAFW::Root colladaRoot(&loader, &writer);
  BOOST_REQUIRE(colladaRoot.loadDocument(fname));

  // verify that scene looks as expected
  BOOST_REQUIRE(sceneMgr->hasEntity("Cube:CubeID"));  // there should be a Cube entity

  Ogre::Entity* ent = sceneMgr->getEntity("Cube:CubeID");
  BOOST_REQUIRE(ent);
  Ogre::MeshPtr mesh = ent->getMesh();
  BOOST_REQUIRE(!mesh.isNull());
  BOOST_CHECK_EQUAL(1, mesh->getNumSubMeshes());
  Ogre::SubEntity* subent = ent->getSubEntity(0);
  BOOST_REQUIRE(subent);
  BOOST_CHECK_EQUAL("LandlordWhite", subent->getMaterialName());

  Ogre::SubMesh* submesh = mesh->getSubMesh(0);
  BOOST_CHECK_EQUAL(Ogre::RenderOperation::OT_TRIANGLE_LIST, submesh->operationType);  // shared buffer with triangles
  BOOST_CHECK_EQUAL(6*2*3, submesh->indexData->indexCount);   // should be 2 triangles per face

  // TODO check structure of scene and transform of the entity

}
