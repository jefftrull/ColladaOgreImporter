// OgreMeshWriter.h, a class declaration for Collada import into a single Ogre mesh
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2011 Aditazz, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef OGRE_COLLADA_MESHWRITER
#define OGRE_COLLADA_MESHWRITER

#include <OgreMesh.h>

namespace COLLADABU { namespace Math { class Matrix4; }  }

#include "OgreColladaWriter.hpp"

namespace OgreCollada {

class MeshWriter : public Writer<MeshWriter> {
 public:
  MeshWriter(const Ogre::String& // dir to find materials etc. in
		 );
  ~MeshWriter();

  // user access to generated data
  Ogre::MeshPtr getMesh() { return m_mesh; }

  // ColladaWriter methods we will implement
  bool write(const COLLADAFW::Geometry*);
  void finishImpl();

  // pick up overloads from Writer
  using Writer<MeshWriter>::write;
  // and WriterBase
  using WriterBase<Writer<MeshWriter>>::write;

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

  class OgreMeshDispatchPass1 : public WriterBase<OgreMeshDispatchPass1> {
    MeshWriter* m_converter;   // dispatch target (the "real" methods)
  public: 
    OgreMeshDispatchPass1(MeshWriter* converter) : m_converter(converter) {}

    // forward everything except geometry
    void start() { m_converter->start(); }
    using WriterBase<OgreMeshDispatchPass1>::write;
    bool write(const COLLADAFW::FileInfo* fi) { return m_converter->write(fi); }
    bool write(const COLLADAFW::Scene* s) { return m_converter->write(s); }
    bool write(const COLLADAFW::LibraryNodes* ln) { return m_converter->write(ln); }
    bool write(const COLLADAFW::Material* m) { return m_converter->write(m); }
    bool write(const COLLADAFW::Effect* e) { return m_converter->write(e); }
    bool write(const COLLADAFW::Camera* c) { return m_converter->write(c); }
    bool write(const COLLADAFW::Image* i) { return m_converter->write(i); }
    bool write(const COLLADAFW::Light* l) { return m_converter->write(l); }
    bool write(const COLLADAFW::Animation* a) { return m_converter->write(a); }
    bool write(const COLLADAFW::AnimationList* al) { return m_converter->write(al); }
    bool write(const COLLADAFW::SkinControllerData* d) { return m_converter->write(d); }
    bool write(const COLLADAFW::Controller* c) { return m_converter->write(c); }
    bool write(const COLLADAFW::Formulas* f) { return m_converter->write(f); }
    bool write(const COLLADAFW::KinematicsScene* s) { return m_converter->write(s); }
    bool write(const COLLADAFW::VisualScene* vs) { return m_converter->write(vs); }
    void finishImpl() { m_converter->pass1Finish(); }

  };

  class OgreMeshDispatchPass2 : public WriterBase<OgreMeshDispatchPass2> {
    MeshWriter* m_converter;   // dispatch target (the "real" methods)
  public:
    OgreMeshDispatchPass2(MeshWriter* converter) : m_converter(converter) {}

    // Forward only writeGeometry and finish
    using WriterBase<OgreMeshDispatchPass2>::write;
    bool write(const COLLADAFW::Geometry* g) {
      return m_converter->write(g);
    }
    void finishImpl() { m_converter->finishImpl(); }
  };

  OgreMeshDispatchPass1* m_pass1Writer;
  OgreMeshDispatchPass2* m_pass2Writer;

 public:
  OgreMeshDispatchPass1* getPass1ProxyWriter() { return m_pass1Writer; }
  OgreMeshDispatchPass2* getPass2ProxyWriter() { return m_pass2Writer; }

};

} // end namespace OgreCollada

#endif
