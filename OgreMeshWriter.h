// OgreMeshWriter.h, a class declaration for Collada import into a single Ogre mesh
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2011 Aditazz, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <OgreMesh.h>

namespace COLLADABU { namespace Math { class Matrix4; }  }

#include "OgreColladaWriter.h"

namespace OgreCollada {

class MeshWriter : public Writer {
 public:
  MeshWriter(const Ogre::String& // dir to find materials etc. in
		 );
  ~MeshWriter();

  // user access to generated data
  Ogre::MeshPtr getMesh() { return m_mesh; }

  // ColladaWriter methods we will implement
  virtual bool writeGeometry(const COLLADAFW::Geometry*);
  virtual void finish();

  // a replacement for finish(), for the the first pass only
  void pass1Finish();

 private:
  // hide default xtor and compiler-generated copy and assignment operators
  MeshWriter();
  MeshWriter( const MeshWriter& pre );
  const MeshWriter& operator= ( const MeshWriter& pre );

  // record, for every library geometry, all the places where it's used, and their transforms
  typedef std::vector<std::pair<const COLLADAFW::MaterialBindingArray*, Ogre::Matrix4> > GeoInstUsageList;
  typedef GeoInstUsageList::const_iterator GeoInstUsageListIter;
  typedef std::map<COLLADAFW::UniqueId, GeoInstUsageList> GeoUsageMap;
  typedef GeoUsageMap::const_iterator GeoUsageMapIter;
  GeoUsageMap m_geometryUsage;

  Ogre::ManualObject* m_manobj;
  Ogre::MeshPtr m_mesh;

  // scene graph traversal function
  bool createSceneDFS(const COLLADAFW::Node*,             // node to instantiate
		      const COLLADABU::Math::Matrix4&);   // accumulated transform

  // dispatch classes.  Instead of defining a single writer that checks to see what mode it's in,
  // define two proxy writers, one for each pass, that either forward the write method to its
  // parent or do nothing.
  // Using this pattern we can invoke the converter methods in an arbitrary order, in fact, as long
  // as we are willing to run the loader N times...  Thanks to Michael Caisse for this idea.

  // base class supplies all methods as NOP.  Passes define only methods that do something
  class OgreMeshDispatchBase : public COLLADAFW::IWriter {
  public:

  OgreMeshDispatchBase(MeshWriter* converter) : m_converter(converter) {}
    // by default methods do nothing
    virtual void cancel(const COLLADAFW::String&) {}
    virtual void start() {}
    virtual bool writeGlobalAsset(const COLLADAFW::FileInfo*) { return true; }
    virtual bool writeScene(const COLLADAFW::Scene*) { return true; }
    virtual bool writeLibraryNodes(const COLLADAFW::LibraryNodes*) { return true; }
    virtual bool writeMaterial(const COLLADAFW::Material*) { return true; }
    virtual bool writeEffect(const COLLADAFW::Effect*) { return true; }
    virtual bool writeCamera(const COLLADAFW::Camera*) { return true; }
    virtual bool writeImage(const COLLADAFW::Image*) { return true; }
    virtual bool writeLight(const COLLADAFW::Light*) { return true; }
    virtual bool writeAnimation(const COLLADAFW::Animation*) { return true; }
    virtual bool writeAnimationList(const COLLADAFW::AnimationList*) { return true; }
    virtual bool writeSkinControllerData(const COLLADAFW::SkinControllerData*) { return true; }
    virtual bool writeController(const COLLADAFW::Controller*) { return true; }
    virtual bool writeFormulas(const COLLADAFW::Formulas*) { return true; }
    virtual bool writeKinematicsScene(const COLLADAFW::KinematicsScene*) { return true; }
    virtual bool writeVisualScene(const COLLADAFW::VisualScene*) { return true; }

    virtual bool writeGeometry(const COLLADAFW::Geometry*) { return true; }
    virtual void finish() = 0;  // we expect all subclasses to override

  protected:
    MeshWriter* m_converter;   // dispatch target (the "real" methods)
  };

  class OgreMeshDispatchPass1 : public OgreMeshDispatchBase {
  public: 
  OgreMeshDispatchPass1(MeshWriter* converter) : OgreMeshDispatchBase(converter) {}

    // forward everything except geometry
    virtual void start() { m_converter->start(); }
    virtual bool writeGlobalAsset(const COLLADAFW::FileInfo* fi) { return m_converter->writeGlobalAsset(fi); }
    virtual bool writeScene(const COLLADAFW::Scene* s) { return m_converter->writeScene(s); }
    virtual bool writeLibraryNodes(const COLLADAFW::LibraryNodes* ln) { return m_converter->writeLibraryNodes(ln); }
    virtual bool writeMaterial(const COLLADAFW::Material* m) { return m_converter->writeMaterial(m); }
    virtual bool writeEffect(const COLLADAFW::Effect* e) { return m_converter->writeEffect(e); }
    virtual bool writeCamera(const COLLADAFW::Camera* c) { return m_converter->writeCamera(c); }
    virtual bool writeImage(const COLLADAFW::Image* i) { return m_converter->writeImage(i); }
    virtual bool writeLight(const COLLADAFW::Light* l) { return m_converter->writeLight(l); }
    virtual bool writeAnimation(const COLLADAFW::Animation* a) { return m_converter->writeAnimation(a); }
    virtual bool writeAnimationList(const COLLADAFW::AnimationList* al) { return m_converter->writeAnimationList(al); }
    virtual bool writeSkinControllerData(const COLLADAFW::SkinControllerData* d) { return m_converter->writeSkinControllerData(d); }
    virtual bool writeController(const COLLADAFW::Controller* c) { return m_converter->writeController(c); }
    virtual bool writeFormulas(const COLLADAFW::Formulas* f) { return m_converter->writeFormulas(f); }
    virtual bool writeKinematicsScene(const COLLADAFW::KinematicsScene* s) { return m_converter->writeKinematicsScene(s); }
    virtual bool writeVisualScene(const COLLADAFW::VisualScene* vs) { return m_converter->writeVisualScene(vs); }
    virtual void finish() { m_converter->pass1Finish(); }

  };

  class OgreMeshDispatchPass2 : public OgreMeshDispatchBase {
  public:
    OgreMeshDispatchPass2(MeshWriter* converter) : OgreMeshDispatchBase(converter) {}

    // Forward only writeGeometry and finish
    virtual bool writeGeometry(const COLLADAFW::Geometry* g) { return m_converter->writeGeometry(g); }
    virtual void finish() { m_converter->finish(); }
  };

  OgreMeshDispatchPass1* m_pass1Writer;
  OgreMeshDispatchPass2* m_pass2Writer;

 public:
  OgreMeshDispatchPass1* getPass1ProxyWriter() { return m_pass1Writer; }
  OgreMeshDispatchPass2* getPass2ProxyWriter() { return m_pass2Writer; }

};

} // end namespace OgreCollada
