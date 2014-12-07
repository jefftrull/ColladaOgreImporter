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
    return static_cast<Writer*>(this)->write(fi);
  }
  virtual bool writeScene(const COLLADAFW::Scene* s) {
    return static_cast<Writer*>(this)->write(s);
  }
  virtual bool writeLibraryNodes(const COLLADAFW::LibraryNodes* ln) {
    return static_cast<Writer*>(this)->write(ln);
  }
  virtual bool writeMaterial(const COLLADAFW::Material* m) {
    return static_cast<Writer*>(this)->write(m);
  }
  virtual bool writeEffect(const COLLADAFW::Effect* e) {
    return static_cast<Writer*>(this)->write(e);
  }
  virtual bool writeCamera(const COLLADAFW::Camera* c) {
    return static_cast<Writer*>(this)->write(c);
  }
  virtual bool writeImage(const COLLADAFW::Image* i) {
    return static_cast<Writer*>(this)->write(i);
  }
  virtual bool writeLight(const COLLADAFW::Light* l) {
    return static_cast<Writer*>(this)->write(l);
  }
  virtual bool writeAnimation(const COLLADAFW::Animation* a) {
    return static_cast<Writer*>(this)->write(a);
  }
  virtual bool writeAnimationList(const COLLADAFW::AnimationList* al) {
    return static_cast<Writer*>(this)->write(al);
  }
  virtual bool writeSkinControllerData(const COLLADAFW::SkinControllerData* scd) {
    return static_cast<Writer*>(this)->write(scd);
  }
  virtual bool writeController(const COLLADAFW::Controller* c) {
    return static_cast<Writer*>(this)->write(c);
  }
  virtual bool writeFormulas(const COLLADAFW::Formulas* f) {
    return static_cast<Writer*>(this)->write(f);
  }
  virtual bool writeKinematicsScene(const COLLADAFW::KinematicsScene* ks) {
    return static_cast<Writer*>(this)->write(ks);
  }
  virtual bool writeVisualScene(const COLLADAFW::VisualScene* vs) {
    return static_cast<Writer*>(this)->write(vs);
  }
  virtual bool writeGeometry(const COLLADAFW::Geometry* g) {
    return static_cast<Writer*>(this)->write(g);
  }

  virtual void finish() {
    static_cast<Writer*>(this)->finishImpl();
  }

  // Default implementations
  void cancelImpl(const COLLADAFW::String&) {}
  void startImpl() {}

  bool write(const COLLADAFW::FileInfo*) { return true; }
  bool write(const COLLADAFW::Scene*) { return true; }
  bool write(const COLLADAFW::LibraryNodes*) { return true; }
  bool write(const COLLADAFW::Material*) { return true; }
  bool write(const COLLADAFW::Effect*) { return true; }
  bool write(const COLLADAFW::Camera*) { return true; }
  bool write(const COLLADAFW::Image*) { return true; }
  bool write(const COLLADAFW::Light*) { return true; }
  bool write(const COLLADAFW::Animation*) { return true; }
  bool write(const COLLADAFW::AnimationList*) { return true; }
  bool write(const COLLADAFW::SkinControllerData*) { return true; }
  bool write(const COLLADAFW::Controller*) { return true; }
  bool write(const COLLADAFW::Formulas*) { return true; }
  bool write(const COLLADAFW::KinematicsScene*) { return true; }
  bool write(const COLLADAFW::VisualScene*) { return true; }
  bool write(const COLLADAFW::Geometry* g) { return true; }
  void finishImpl() {}

  // no default impl for finish(); we expect all subclasses to supply

};

}

#endif  // OGRE_COLLADA_WRITER_BASE_H
