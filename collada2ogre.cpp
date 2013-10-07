// Main program of Collada to Ogre mesh/material converter
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2012 Aditazz, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// specify new version of filesystem API, as the old one is about to be removed
#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>

#include <boost/lexical_cast.hpp>
#include <fstream>
#include <COLLADAFWRoot.h>
#include <COLLADASaxFWLLoader.h>

#include <OgreRoot.h>
#include <OgreLogManager.h>
#include <OgreMaterial.h>
#include <OgreMaterialSerializer.h>
#include <OgreMeshSerializer.h>
#include <OgreRenderWindow.h>

#include "OgreMeshWriter.h"

#define LOG_DEBUG(msg) { Ogre::LogManager::getSingleton().logMessage( Ogre::String((msg)) ); }

// bug fix: subclass material serializer listener to override writing texture unit filename in cases where
// the filename contains embedded spaces, and thus needs to be quoted
// newer versions of Ogre do this already...
class MWMatSerListener : public Ogre::MaterialSerializer::Listener {
public:
  virtual void textureUnitStateEventRaised ( Ogre::MaterialSerializer* ser,
					     Ogre::MaterialSerializer::SerializeEvent      event,
					     bool&                     skip,
					     const Ogre::TextureUnitState* textureUnit ) {
    if (event == Ogre::MaterialSerializer::MSE_WRITE_BEGIN) {
      Ogre::String tname = textureUnit->getTextureName();
      // does the name contain unquoted embedded spaces?
      if ((tname.find_first_of("\"") == Ogre::String::npos) &&  // not already quoted
	  (tname.find_first_of(" ") != Ogre::String::npos)) {   // and contains a space
	  Ogre::String quotedName = ("\"" + tname + "\"");
	  // BOZO hack.  Override constness:
	  (const_cast<Ogre::TextureUnitState*>(textureUnit))->setTextureName(quotedName, textureUnit->getTextureType());
      }
    }
  }
};

// Ogre boilerplate for cross-platform main program

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

int main(int argc, char *argv[])
{
  Ogre::Root root;
  Ogre::RenderSystemList rlist = root.getAvailableRenderers();
  for (size_t i = 0; i < rlist.size(); ++i) {
    LOG_DEBUG("renderer: " + rlist[i]->getName());
  }

  // It looks like my "manualobject" approach requires the availability of actual hardware buffers
  // So unfortunately we have to have a window etc.
  // perhaps there's a way to cleanly avoid the hardware buffer approach...

  // set a bunch of config options to avoid having to have Ogre config files in the current directory
  root.setRenderSystem(root.getRenderSystemByName("OpenGL Rendering Subsystem"));
  root.initialise(false);      // we specify our own window
  root.getRenderSystem()->setConfigOption("RTT Preferred Mode", "PBuffer");  // bug workaround in nVidia drivers
  root.createRenderWindow("ignore me", 80, 80, false);

  // open logger
  Ogre::LogManager::getSingleton().createLog("collada2ogre.log", true);

  // parse args.  A single arg gives the name of the input, and outputs will be placed
  // in the same directory.  Two args give the name of the input dae and output mesh files;
  // other outputs (e.g., materials) will be placed in the same directory as the output file
  if ((argc < 2) || (argc > 3)) {  // argv[0] is program name...
    std::cerr << "usage: collada2ogre input.dae [output.mesh]\n";
    return 1;
  }

  boost::filesystem::path daepath(argv[1]);
  boost::filesystem::path meshpath;
  if (argc == 2) {
    // no output file supplied; base path and file on input
    meshpath = daepath;
    meshpath.replace_extension(".mesh");
  } else {
    meshpath = boost::filesystem::path(argv[2]);
  }
  boost::filesystem::path matpath = meshpath;
  matpath.replace_extension(".material");
  // we will let Ogre access textures using a relative path, which is what we will want for export
  boost::filesystem::path texturedir = meshpath.parent_path();
  //  boost::filesystem::path texturedir = meshpath.parent_path() / meshpath.stem();  // slash operator concatenates path components

  OgreCollada::MeshWriter writer(texturedir.string());
  COLLADASaxFWL::Loader loader;
  COLLADAFW::Root pass1Root(&loader, writer.getPass1ProxyWriter());
  if (!pass1Root.loadDocument(daepath.string())) {
    std::cerr << "load document failed in pass 1\n";
    return 1;
  }
  COLLADAFW::Root pass2Root(&loader, writer.getPass2ProxyWriter());
  if (!pass2Root.loadDocument(daepath.string())) {
    std::cerr << "load document failed in pass 2\n";
    return 1;
  }

  // access mesh, materials list and report statistics
  const std::list<Ogre::MaterialPtr>& materials = writer.getMaterials();
  LOG_DEBUG("mesh conversion produced " + boost::lexical_cast<Ogre::String>(materials.size()) + " materials:");
  Ogre::MaterialSerializer matser;
  matser.addListener(new MWMatSerListener());
  for (std::list<Ogre::MaterialPtr>::const_iterator matit = materials.begin();
       matit != materials.end(); ++matit) {
    LOG_DEBUG((*matit)->getName());
    matser.queueForExport(*matit);
  }
  matser.exportQueued(matpath.string());

  Ogre::MeshPtr mesh = writer.getMesh();
  if (mesh.isNull()) {
    LOG_DEBUG("no mesh created.  exiting.");
    return 1;
  }
  LOG_DEBUG("Created a mesh with " + boost::lexical_cast<Ogre::String>(mesh->getNumSubMeshes()) + " submeshes");
  Ogre::MeshSerializer meshser;
  meshser.exportMesh(mesh.get(), meshpath.string());

  return 0;
}
