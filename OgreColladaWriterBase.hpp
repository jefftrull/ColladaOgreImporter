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

namespace OgreCollada {

namespace detail {

// some syntactic sugar for providing default implementations of write() visitors
template<class Attribute, class... Attributes>
struct DefaultImpl : public DefaultImpl<Attributes...> {
  using DefaultImpl<Attributes...>::write;  // pick up other versions of "write"
  bool write(Attribute const*) { return true; }
protected:
  DefaultImpl() = default;     // this class is for base class use only
};

template<class Attribute>
struct DefaultImpl<Attribute> {
  bool write(Attribute const*) { return true; }
};

typedef DefaultImpl<COLLADAFW::Camera, COLLADAFW::FileInfo, COLLADAFW::Scene,
                    COLLADAFW::LibraryNodes, COLLADAFW::Material, COLLADAFW::Effect,
                    COLLADAFW::Image, COLLADAFW::Light, COLLADAFW::Animation,
                    COLLADAFW::AnimationList, COLLADAFW::SkinControllerData,
                    COLLADAFW::Controller, COLLADAFW::Formulas,
                    COLLADAFW::KinematicsScene, COLLADAFW::VisualScene,
                    COLLADAFW::Geometry> AllMethodsDefault;

}

// Base class to supply a default implementation for (almost) all pure virtual methods
// Actual work is delegated to derived classes
template<class Writer>
class WriterBase : public COLLADAFW::IWriter,
                   public detail::AllMethodsDefault {

protected:
  WriterBase() = default;

public:
  virtual      ~WriterBase() {}

  // define required IWriter methods and delegate to derived via CRTP
  virtual void cancel(const COLLADAFW::String& s) override {
    static_cast<Writer*>(this)->cancelImpl(s);
  }
  virtual void start() override {
    static_cast<Writer*>(this)->startImpl();
  }
  virtual bool writeGlobalAsset(const COLLADAFW::FileInfo* fi) override {
    return static_cast<Writer*>(this)->write(fi);
  }
  virtual bool writeScene(const COLLADAFW::Scene* s) override {
    return static_cast<Writer*>(this)->write(s);
  }
  virtual bool writeLibraryNodes(const COLLADAFW::LibraryNodes* ln) override {
    return static_cast<Writer*>(this)->write(ln);
  }
  virtual bool writeMaterial(const COLLADAFW::Material* m) override {
    return static_cast<Writer*>(this)->write(m);
  }
  virtual bool writeEffect(const COLLADAFW::Effect* e) override {
    return static_cast<Writer*>(this)->write(e);
  }
  virtual bool writeCamera(const COLLADAFW::Camera* c) override {
    return static_cast<Writer*>(this)->write(c);
  }
  virtual bool writeImage(const COLLADAFW::Image* i) override {
    return static_cast<Writer*>(this)->write(i);
  }
  virtual bool writeLight(const COLLADAFW::Light* l) override {
    return static_cast<Writer*>(this)->write(l);
  }
  virtual bool writeAnimation(const COLLADAFW::Animation* a) override {
    return static_cast<Writer*>(this)->write(a);
  }
  virtual bool writeAnimationList(const COLLADAFW::AnimationList* al) override {
    return static_cast<Writer*>(this)->write(al);
  }
  virtual bool writeSkinControllerData(const COLLADAFW::SkinControllerData* scd) override {
    return static_cast<Writer*>(this)->write(scd);
  }
  virtual bool writeController(const COLLADAFW::Controller* c) override {
    return static_cast<Writer*>(this)->write(c);
  }
  virtual bool writeFormulas(const COLLADAFW::Formulas* f) override {
    return static_cast<Writer*>(this)->write(f);
  }
  virtual bool writeKinematicsScene(const COLLADAFW::KinematicsScene* ks) override {
    return static_cast<Writer*>(this)->write(ks);
  }
  virtual bool writeVisualScene(const COLLADAFW::VisualScene* vs) override {
    return static_cast<Writer*>(this)->write(vs);
  }
  virtual bool writeGeometry(const COLLADAFW::Geometry* g) override {
    return static_cast<Writer*>(this)->write(g);
  }

  virtual void finish() override {
    static_cast<Writer*>(this)->finishImpl();
  }

  // Default implementations
  void cancelImpl(const COLLADAFW::String&) {}
  void startImpl() {}

  using detail::AllMethodsDefault::write;

  // no default impl for finish(); we expect all subclasses to supply

};

}

#endif  // OGRE_COLLADA_WRITER_BASE_H
