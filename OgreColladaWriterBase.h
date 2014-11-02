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

// Base class to supply a default implementation for (almost) all pure virtual methods
class WriterBase : public COLLADAFW::IWriter {
public:
  virtual      ~WriterBase() = 0;

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
};

}

#endif  // OGRE_COLLADA_WRITER_BASE_H
