// OgreColladaWriter.h, a class definition for (restricted) Collada import into Ogre
// Child classes to build scene hierarchies or a single mesh from a scene inherit from this
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2010, 2011 Aditazz, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef OGRE_COLLADA_WRITER_H
#define OGRE_COLLADA_WRITER_H

#include <OgreString.h>
#include <OgreQuaternion.h>
#include <OgreVector3.h>
#include <OgreMatrix4.h>
#include <OgreCommon.h>

#include <COLLADAFWIWriter.h>
#include <COLLADAFWMaterialBinding.h>

namespace COLLADAFW {
   class Node;
   class EffectCommon;
   class ColorOrTexture;
}

#define LOG_DEBUG(msg) { Ogre::LogManager::getSingleton().logMessage( Ogre::String((msg)) ); }

namespace OgreCollada {

class Writer : public COLLADAFW::IWriter {
 public:
  // no public xtor, child classes to supply their own
  ~Writer();

  // declare implementations for parent class's virtual functions
  virtual void cancel(const COLLADAFW::String&);
  virtual void start();
  virtual bool writeGlobalAsset(const COLLADAFW::FileInfo*);
  virtual bool writeScene(const COLLADAFW::Scene*);
  virtual bool writeLibraryNodes(const COLLADAFW::LibraryNodes*);
  virtual bool writeMaterial(const COLLADAFW::Material*);
  virtual bool writeEffect(const COLLADAFW::Effect*);
  virtual bool writeCamera(const COLLADAFW::Camera*);
  virtual bool writeImage(const COLLADAFW::Image*);
  virtual bool writeLight(const COLLADAFW::Light*);
  virtual bool writeAnimation(const COLLADAFW::Animation*);
  virtual bool writeAnimationList(const COLLADAFW::AnimationList*);
  virtual bool writeSkinControllerData(const COLLADAFW::SkinControllerData*);
  virtual bool writeController(const COLLADAFW::Controller*);
  virtual bool writeFormulas(const COLLADAFW::Formulas*);
  virtual bool writeKinematicsScene(const COLLADAFW::KinematicsScene*);
  virtual bool writeVisualScene(const COLLADAFW::VisualScene*);

  // two IWriter methods we expect child classes to implement
  virtual bool writeGeometry(const COLLADAFW::Geometry*) = 0;
  virtual void finish() = 0;

  void setGraphOutput(const char* filename) { m_dotfn = filename; }

  std::vector<Ogre::MaterialPtr> const& getMaterials() const { return m_ogreMaterials; }

  // a separate method to disable culling for materials marked "double sided"
  // this is out-of-band information supplied by some converters and not an official
  // part of the standard, so it takes this route
  void disableCulling(COLLADAFW::UniqueId const&);

 protected:
  // parent class members aren't accessible to child constructors, so provide this xtor for children to use:
  Writer(const Ogre::String&, const char*, bool, bool);

  // geometry statistics and debug tools are useful to child classes
  const Ogre::String& m_dir;   // directory relative to which we can find texture images etc.
  const char* m_dotfn;   // where (and if) to dump DOT output describing the scene hierarchy
  void dump_as_dot(std::ostream& os);
  bool m_checkNormals;         // whether to do checking of the surface normals against vertex winding order

  // starting points for final processing
  std::vector<const COLLADAFW::Node*> m_vsRootNodes;                 // top-level nodes
  Ogre::Quaternion m_ColladaRotation;        // how to rotate Collada input to match Ogre's Y-up coordinates
  Ogre::Vector3 m_ColladaScale;              // how to scale Collada input into meters
  
  // utility functions used while building scene
  bool addGeometry(const COLLADAFW::Geometry* g,                         // input geometry from Collada
		   Ogre::ManualObject* manobj,                           // object under construction
		   const Ogre::Matrix4& xform = Ogre::Matrix4::IDENTITY, // transform to this point
		   const COLLADAFW::MaterialBindingArray* mba = 0);      // materials to attach

  void createMaterials();

  // stats
  bool m_calculateGeometryStats; // whether to calculate and log statistics on geometries (meshes) and their usages
  std::map<COLLADAFW::UniqueId, Ogre::String> m_geometryNames; // geometries in input
  std::map<COLLADAFW::UniqueId, int> m_geometryInstanceCounts; // how often it gets used
  std::map<COLLADAFW::UniqueId, int> m_geometryTriangleCounts; // how many triangles it contains
  std::map<COLLADAFW::UniqueId, int> m_geometryLineCounts;     // how many lines it contains

  // internal class to do sorting of triangle counts
  class TriangleCountComparator {
  public:
  TriangleCountComparator(const std::map<COLLADAFW::UniqueId, int>& instCount,
			  const std::map<COLLADAFW::UniqueId, int>& triCount) : m_icount(instCount), m_tcount(triCount) {}
    bool operator() (const COLLADAFW::UniqueId& a,
		     const COLLADAFW::UniqueId& b) {
      // biggest first
      return m_icount.find(a)->second * m_tcount.find(a)->second > m_icount.find(b)->second * m_tcount.find(b)->second;
    }
  private:
    const std::map<COLLADAFW::UniqueId, int> &m_icount, &m_tcount;

  };

  // we have to store, for each created mesh, what "material ID" is used by each submesh
  // then we'll be able to bind materials to the created entities properly
  typedef std::map<Ogre::MeshPtr, std::vector<COLLADAFW::MaterialId> > MeshMaterialIdMap;
  typedef MeshMaterialIdMap::const_iterator MeshMaterialIdMapIterator;
  MeshMaterialIdMap m_meshmatids;

  // library geometries
  std::map<COLLADAFW::UniqueId, Ogre::MeshPtr> m_meshMap;  // loaded or generated meshes here

 protected:
  // data storage - stuff collected during callbacks from Collada
  typedef std::map<COLLADAFW::UniqueId, const COLLADAFW::Node*> LibNodesContainer;
  typedef LibNodesContainer::const_iterator LibNodesIterator;
  LibNodesContainer m_libNodes; // tree roots for library nodes

  // names and effect IDs for each material, searchable by material ID (referenced by geometry instances)
  typedef std::map<COLLADAFW::UniqueId, std::pair<Ogre::String, COLLADAFW::UniqueId> > MaterialMap;
  typedef MaterialMap::const_iterator MaterialMapIterator;
  MaterialMap m_materials;

  typedef std::map<COLLADAFW::UniqueId, std::vector<COLLADAFW::EffectCommon> > EffectMap;
  typedef EffectMap::const_iterator EffectMapIterator;
  EffectMap m_effects;
  
  typedef std::map<COLLADAFW::UniqueId, Ogre::String> ImageMap;
  typedef ImageMap::const_iterator ImageMapIterator;
  ImageMap m_images;

 private:
  // hide default xtor and compiler-generated copy and assignment operators
  Writer();
  Writer( const Writer& pre );
  const Writer& operator= ( const Writer& pre );

  typedef void (Ogre::Pass::*ColorSetter)(const Ogre::ColourValue&);
  void handleColorOrTexture(const COLLADAFW::EffectCommon& ce,
			    const COLLADAFW::ColorOrTexture&,
			    Ogre::Pass*, ColorSetter, Ogre::TrackVertexColourType);

  std::vector<Ogre::MaterialPtr> m_ogreMaterials;

  bool m_sketchUpWorkarounds;

  std::vector<COLLADAFW::UniqueId> m_unculledEffects;

  // private debug functions
  void node_dfs_print(const COLLADAFW::Node*, int);
  void node_dfs_dot(std::ostream& os, const COLLADAFW::Node*, int, const std::map<const COLLADAFW::Node*, int>&);
  void node_dfs_geocheck(const COLLADAFW::Node*);

};

} // end namespace OgreCollada

#endif // OGRE_COLLADA_WRITER_H
