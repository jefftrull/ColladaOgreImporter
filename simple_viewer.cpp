// A simple viewer for Collada .DAE files using the OgreColladaImporter in "scene" (hierarchy preservation) mode
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2012 Jeffrey E Trull

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <utility>
#include <iostream>

// my idea of a really really minimal viewer app

#include <OgreCamera.h>
#include <OgreEntity.h>
#include <OgreLogManager.h>
#include <OgreRoot.h>
#include <OgreViewport.h>
#include <OgreSceneManager.h>
#include <OgreRenderWindow.h>
#include <OgreConfigFile.h>
#include <OgreFrameListener.h>
#include <OgreWindowEventUtilities.h>

#include "OgreColladaSaxLoader.h"

struct SimpleViewer : public Ogre::FrameListener, public Ogre::WindowEventListener {
  SimpleViewer() : root_(new Ogre::Root("plugins.cfg")), shutdown_(false)
 {
   if (root_->showConfigDialog()) {
     window_   = root_->initialise(true, "Collada Viewer");
     scenemgr_ = root_->createSceneManager(Ogre::ST_GENERIC);

     camera_   = scenemgr_->createCamera("ViewerCam");

     viewport_ = window_->addViewport(camera_);
     viewport_->setBackgroundColour(Ogre::ColourValue(0,0,0));
 
     // Alter the camera aspect ratio to match the viewport
     camera_->setAspectRatio(Ogre::Real(viewport_->getActualWidth()) /
			     Ogre::Real(viewport_->getActualHeight()));

     // listen so we can know when the window is destroyed
     Ogre::WindowEventUtilities::addWindowEventListener(window_, this);

     root_->addFrameListener(this);
   }
 }

  ~SimpleViewer() {
    root_->removeFrameListener(this);

    Ogre::WindowEventUtilities::removeWindowEventListener(window_, this);
    windowClosed(window_);
  }

  void go() {
    root_->startRendering();
  }

  void windowClosed(Ogre::RenderWindow*) {
    shutdown_ = true;
  }

  bool frameRenderingQueued(const Ogre::FrameEvent&) {
    return !shutdown_;
  }

  Ogre::SceneManager*                 getSceneManager() { return scenemgr_; }
  Ogre::Camera*                       getCamera()       { return camera_; }

private:
  std::unique_ptr<Ogre::Root>         root_;
  // in tutorial app only root gets deleted in destructor, so use raw pointers for these:
  Ogre::RenderWindow*                 window_;
  Ogre::SceneManager*                 scenemgr_;
  Ogre::Camera*                       camera_;
  Ogre::Viewport*                     viewport_;
  // application state
  bool                                shutdown_;
};

#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>

#include <COLLADAFWRoot.h>

#include "OgreSceneWriter.h"

int main(int argc, char **argv) {
  // Expects just one argument: path to .dae file
  if (argc != 2) {
    std::cerr << "usage: cview /path/to/model.dae" << std::endl;
    return 1;
  }

  // make sure it's there
  std::string fname(argv[1]);
  if (!boost::filesystem::exists(fname) ||
      !boost::filesystem::is_regular_file(fname)) {
    std::cerr << "cannot access file " << fname << std::endl;
    return 1;
  }

  // determine name of parent directory
  std::string dir = boost::filesystem::path(fname).parent_path().string();

  // create the viewer application
  SimpleViewer viewer;

  // configure it via scene manager and camera getters

  // set camera position and orientation
  // viewer position assumed to be +Z
  viewer.getCamera()->setPosition(Ogre::Vector3(0,0,100));
  // camera looks back along -Z into screen
  viewer.getCamera()->lookAt(Ogre::Vector3(0,0,-100));
  viewer.getCamera()->setNearClipDistance(5);

  // set up some lights to illuminate loaded object
  Ogre::ColourValue dim(0.25, 0.25, 0.25);
  viewer.getSceneManager()->setAmbientLight(dim);

  Ogre::Light* cam_light = viewer.getSceneManager()->createLight("cameraLight");
  cam_light->setType(Ogre::Light::LT_DIRECTIONAL);
  cam_light->setDiffuseColour(dim);
  cam_light->setSpecularColour(dim);
  cam_light->setDirection(Ogre::Vector3(0, 0, -1));

  Ogre::Light* overhead_light = viewer.getSceneManager()->createLight("overheadLight");
  overhead_light->setType(Ogre::Light::LT_DIRECTIONAL);
  overhead_light->setDiffuseColour(dim);
  overhead_light->setSpecularColour(dim);
  overhead_light->setDirection(Ogre::Vector3(0, -1, 0));

  OgreSceneWriter writer(viewer.getSceneManager(),
			 viewer.getSceneManager()->getRootSceneNode()->createChildSceneNode("Top"),
			 dir);

  OgreColladaSaxLoader loader;
  COLLADAFW::Root root(&loader, &writer);
  if (!root.loadDocument(fname)) {
    std::cerr << "load document failed\n";
    return 1;
  }

  viewer.go();

  return 0;
}
