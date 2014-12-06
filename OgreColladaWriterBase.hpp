// Base class writer with defaults for implementation of Collada to Ogre translation
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2014 Jeffrey E. Trull

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef OGRE_COLLADA_WRITER_BASE_H
#define OGRE_COLLADA_WRITER_BASE_H

#include <OgreLogManager.h>
#include <boost/lexical_cast.hpp>

namespace OgreCollada {

// Base class to supply a default implementation for (almost) all pure virtual methods
// Actual work is delegated to derived classes
template<class Writer>
class WriterBase : public COLLADAFW::IWriter {
public:
  virtual      ~WriterBase() {}

  // define required IWriter methods and delegate to derived via CRTP
  virtual void cancel(const COLLADAFW::String& s) {
    static_cast<Writer*>(this)->cancelImpl(s);
  }
  virtual void start() {
    static_cast<Writer*>(this)->startImpl();
  }
  virtual bool writeGlobalAsset(const COLLADAFW::FileInfo* fi) {
    return static_cast<Writer*>(this)->globalAssetImpl(fi);
  }
  virtual bool writeScene(const COLLADAFW::Scene* s) {
    return static_cast<Writer*>(this)->sceneImpl(s);
  }
  virtual bool writeLibraryNodes(const COLLADAFW::LibraryNodes* ln) {
    return static_cast<Writer*>(this)->libraryNodesImpl(ln);
  }
  virtual bool writeMaterial(const COLLADAFW::Material* m) {
    return static_cast<Writer*>(this)->materialImpl(m);
  }
  virtual bool writeEffect(const COLLADAFW::Effect* e) {
    return static_cast<Writer*>(this)->effectImpl(e);
  }
  virtual bool writeCamera(const COLLADAFW::Camera* c) {
    return static_cast<Writer*>(this)->cameraImpl(c);
  }
  virtual bool writeImage(const COLLADAFW::Image* i) {
    return static_cast<Writer*>(this)->imageImpl(i);
  }
  virtual bool writeLight(const COLLADAFW::Light* l) {
    return static_cast<Writer*>(this)->lightImpl(l);
  }
  virtual bool writeAnimation(const COLLADAFW::Animation* a) {
    return static_cast<Writer*>(this)->animationImpl(a);
  }
  virtual bool writeAnimationList(const COLLADAFW::AnimationList* al) {
    return static_cast<Writer*>(this)->animationListImpl(al);
  }
  virtual bool writeSkinControllerData(const COLLADAFW::SkinControllerData* scd) {
    return static_cast<Writer*>(this)->skinControllerDataImpl(scd);
  }
  virtual bool writeController(const COLLADAFW::Controller* c) {
    return static_cast<Writer*>(this)->controllerImpl(c);
  }
  virtual bool writeFormulas(const COLLADAFW::Formulas* f) {
    return static_cast<Writer*>(this)->formulasImpl(f);
  }
  virtual bool writeKinematicsScene(const COLLADAFW::KinematicsScene* ks) {
    return static_cast<Writer*>(this)->kinematicsSceneImpl(ks);
  }
  virtual bool writeVisualScene(const COLLADAFW::VisualScene* vs) {
    return static_cast<Writer*>(this)->visualSceneImpl(vs);
  }
  virtual bool writeGeometry(const COLLADAFW::Geometry* g) {
    return static_cast<Writer*>(this)->geometryImpl(g);
  }

  virtual void finish() {
    static_cast<Writer*>(this)->finishImpl();
  }

  // Default implementations
  void cancelImpl(const COLLADAFW::String&) {}
  void startImpl() {}

  bool globalAssetImpl(const COLLADAFW::FileInfo*) { return true; }
  bool sceneImpl(const COLLADAFW::Scene*) { return true; }
  bool libraryNodesImpl(const COLLADAFW::LibraryNodes*) { return true; }
  bool materialImpl(const COLLADAFW::Material*) { return true; }
  bool effectImpl(const COLLADAFW::Effect*) { return true; }
  bool cameraImpl(const COLLADAFW::Camera*) { return true; }
  bool imageImpl(const COLLADAFW::Image*) { return true; }
  bool lightImpl(const COLLADAFW::Light*) { return true; }
  bool animationImpl(const COLLADAFW::Animation*) { return true; }
  bool animationListImpl(const COLLADAFW::AnimationList*) { return true; }
  bool skinControllerDataImpl(const COLLADAFW::SkinControllerData*) { return true; }
  bool controllerImpl(const COLLADAFW::Controller*) { return true; }
  bool formulasImpl(const COLLADAFW::Formulas*) { return true; }
  bool kinematicsSceneImpl(const COLLADAFW::KinematicsScene*) { return true; }
  bool visualSceneImpl(const COLLADAFW::VisualScene*) { return true; }
  bool geometryImpl(const COLLADAFW::Geometry* g) { return true; }
  void finishImpl() {}

  // no default impl for finish(); we expect all subclasses to supply

};

}

#endif  // OGRE_COLLADA_WRITER_BASE_H
