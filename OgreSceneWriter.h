// OgreSceneWriter.h, a class definition for Collada import into an Ogre scene graph
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2010, 2011 Aditazz, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <OgreSceneManager.h>
#include <COLLADAFWInstanceNode.h>

#include "OgreColladaWriter.h"

namespace COLLADAFW {
   class Geometry;
}

class OgreSceneWriter : public OgreColladaWriter {
 public:
  OgreSceneWriter(Ogre::SceneManager*,  // the SceneManager in which to create SceneNodes
		  Ogre::SceneNode*,     // the scene node under which we instantiate the loaded data
		  const Ogre::String&); // dir to find materials in
  ~OgreSceneWriter();
  virtual bool writeCamera(const COLLADAFW::Camera*);
  virtual bool writeGeometry(const COLLADAFW::Geometry*);
  virtual void finish();

  Ogre::Camera* getCamera();            // If Collada file defined and instantiated one (returns first)

 private:
  // hide default xtor and compiler-generated copy and assignment operators
  OgreSceneWriter();
  OgreSceneWriter( const OgreSceneWriter& pre );
  const OgreSceneWriter& operator= ( const OgreSceneWriter& pre );

  std::map<COLLADAFW::UniqueId, COLLADAFW::Camera> m_cameras;

  // utility functions
  bool createSceneDFS(const COLLADAFW::Node*, Ogre::SceneNode*, const Ogre::String& prefix = "");
  bool processLibraryInstance(const COLLADAFW::InstanceNode*, Ogre::SceneNode*, const Ogre::String& prefix);
  Ogre::Matrix4 computeTransformation(const COLLADAFW::Transformation*);

  Ogre::SceneNode* m_topNode;
  Ogre::SceneManager* m_sceneMgr;

  std::vector<Ogre::Camera*> m_instantiatedCameras;
};
