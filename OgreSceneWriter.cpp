// Implementation of Collada to Ogre translation
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2010 Aditazz, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <COLLADAFWMatrix.h>
#include <OgreManualObject.h>
#include <OgreMatrix4.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>

#include "OgreSceneWriter.h"

OgreSceneWriter::OgreSceneWriter(Ogre::SceneManager* mgr,
				 Ogre::SceneNode* topnode,
				 const Ogre::String& dir) : OgreColladaWriter(dir, 0, false, false),
							    m_topNode(topnode), m_sceneMgr(mgr) {}

OgreSceneWriter::~OgreSceneWriter() {}

bool OgreSceneWriter::writeGeometry(const COLLADAFW::Geometry* g) {

  if (m_calculateGeometryStats) {
    // remember this for later use
    m_geometryNames.insert(std::make_pair(g->getUniqueId(), g->getOriginalId()));
    m_geometryInstanceCounts.insert(std::make_pair(g->getUniqueId(), 0));
    m_geometryLineCounts.insert(std::make_pair(g->getUniqueId(), 0));
    m_geometryTriangleCounts.insert(std::make_pair(g->getUniqueId(), 0));
  }

  if (g->getType() != COLLADAFW::Geometry::GEO_TYPE_MESH) {
    Ogre::String geotype;
    if (g->getType() == COLLADAFW::Geometry::GEO_TYPE_SPLINE) {
      geotype = "spline";
    } else if (g->getType() == COLLADAFW::Geometry::GEO_TYPE_CONVEX_MESH) {
      geotype = "convex mesh";
    } else {
      geotype = "unknown";
    }
    LOG_DEBUG("COLLADA WARNING: writeGeometry called on type " + geotype + ", which is not supported.  Skipping)");
    return false;
  }

  // create a mesh object out of this Geometry

  // After a lot of experimenting it seems like the "ManualObject" flow is the way to go.
  // I had initially avoided it because it didn't allow vertex sharing among submeshes.
  // However, I've discovered that such sharing may be impossible anyway, because Collada
  // has this mixed-index thing where a pair of indices (e.g., vertex/texture index) can reference
  // an arbitrary vertex position/normal out of one set of (Collada) vertices and a texture coordinate
  // out of another.  Ogre wants a single index and a single vertex buffer, so you have to make a mapping
  // from the Collada pair (triple, quad...) to the Ogre index, and create a single vertex buffer for it
  // to reference with all the possible combinations expanded, which means NO SHARING.  So let's use the
  // simpler interface for clarity. (we still have to flatten the index tuples)

  Ogre::ManualObject* manobj = new Ogre::ManualObject(g->getOriginalId() + "_mobj");
  const COLLADAFW::Mesh* cmesh = dynamic_cast<const COLLADAFW::Mesh*>(g);

  // do some estimating of ultimate sizes, for performance
  manobj->estimateVertexCount(cmesh->getPositions().getValuesCount());
  int index_count = 0;
  for (int prim = 0, count = cmesh->getMeshPrimitives().getCount(); prim < count; ++prim) {
    index_count += cmesh->getMeshPrimitives()[prim]->getPositionIndices().getCount();
  }
  manobj->estimateIndexCount(index_count);   // probably more than this, actually...

  if (!addGeometry(g, manobj)) {
    LOG_DEBUG("Could not find valid submesh to create, so not creating the parent mesh");
    return true;  // make this harmless - for now
  }

  // convert manual object to mesh
  Ogre::MeshPtr mesh = manobj->convertToMesh(g->getOriginalId());

  // record materials information for later reference
  for (int i = 0, count = cmesh->getMeshPrimitives().getCount(); i < count; ++i) {
    const COLLADAFW::MeshPrimitive& prim = *(cmesh->getMeshPrimitives()[i]);
    m_meshmatids[mesh].push_back(prim.getMaterialId());
  }

  if (!mesh->isManuallyLoaded()) {
    LOG_DEBUG("mesh " + mesh->getName() + " is not marked manual, for some reason. It is likely we failed to load it");
  }

  // store this mesh somewhere we can refer to it later (e.g. from a library instance)
  m_meshMap.insert(std::make_pair(g->getUniqueId(), mesh));

  return true;
}

void OgreSceneWriter::finish() {
  // this is the only function we're guaranteed will be called after all the others...
  // so do everything from here

  createMaterials();

  // GraphViz debug output
  if (m_dotfn) {
    std::ofstream os(m_dotfn);
    if (os.is_open()) {
      dump_as_dot(os);
    } else {
      LOG_DEBUG("Could not open dot output file " + Ogre::String(m_dotfn));
    }
  }

  // Create an intervening "shim" scene node to handle the fact that the Collada "up" axis and that of Ogre are likely different
  // This would also be a good place to introduce other transformations for the import, i.e. translation to desired location
  Ogre::SceneNode* transformShimNode = m_topNode->createChildSceneNode();
  transformShimNode->setOrientation(m_ColladaRotation);
  transformShimNode->setScale(m_ColladaScale);

  // next: (recursively) process root node associated with "visual scene" element of input
  for (size_t i = 0; i < m_vsRootNodes.size(); ++i) {
    createSceneDFS(m_vsRootNodes[i],
		       transformShimNode->createChildSceneNode(m_vsRootNodes[i]->getName()),
		       m_vsRootNodes[i]->getName() + ":");
  }

  if (m_calculateGeometryStats) {
    // output geometry stats
    std::vector<COLLADAFW::UniqueId> geometries;
    // BOZO should use some type of function object magic here instead
    for (std::map<COLLADAFW::UniqueId, int>::const_iterator icit = m_geometryInstanceCounts.begin();
	 icit != m_geometryInstanceCounts.end(); ++icit) {
      geometries.push_back(icit->first);
    }
    std::sort(geometries.begin(), geometries.end(),
	      TriangleCountComparator(m_geometryInstanceCounts, m_geometryTriangleCounts));

    LOG_DEBUG("loaded geometry data as follows:");
    for (std::vector<COLLADAFW::UniqueId>::const_iterator git = geometries.begin();
	 git != geometries.end(); ++git) {
      LOG_DEBUG(m_geometryNames[*git] +
		"\t" + Ogre::StringConverter::toString(m_geometryTriangleCounts[*git]) +
		"\t" + Ogre::StringConverter::toString(m_geometryLineCounts[*git]) +
		"\t" + Ogre::StringConverter::toString(m_geometryInstanceCounts[*git]));
    }
  }
}

bool OgreSceneWriter::createSceneDFS(const COLLADAFW::Node* cn, Ogre::SceneNode* sn, const Ogre::String& prefix) {
  // General algorithm (assumes Ogre scene node is already created):
  // set transformation
  // for each instance node, build copy of its subtree recursively, with uniquified name
  // for each attached geometry, create an entity using our name prefix
  // recursively handle child each node

  // handle this node's transformation matrix
  const COLLADAFW::TransformationPointerArray& tarr = cn->getTransformations();
  if (tarr.getCount() > 1) {
    LOG_DEBUG("COLLADA WARNING: Scene node has " + Ogre::StringConverter::toString(tarr.getCount()) + " transformations - we only handle 0 or 1");
  } else if (tarr.getCount() == 1) {
    if (tarr[0]->getTransformationType() != COLLADAFW::Transformation::MATRIX) {
      LOG_DEBUG("COLLADA WARNING: Scene node has non-matrix transformation - ignoring");
    } else {
      const COLLADAFW::Matrix& m = dynamic_cast<const COLLADAFW::Matrix&>(*tarr[0]);
      const COLLADABU::Math::Matrix4& mm = m.getMatrix();
      // create an Ogre matrix from this data
      Ogre::Matrix4 om(mm.getElement(0, 0), mm.getElement(0, 1), mm.getElement(0, 2), mm.getElement(0, 3),
		       mm.getElement(1, 0), mm.getElement(1, 1), mm.getElement(1, 2), mm.getElement(1, 3),
		       mm.getElement(2, 0), mm.getElement(2, 1), mm.getElement(2, 2), mm.getElement(2, 3),
		       mm.getElement(3, 0), mm.getElement(3, 1), mm.getElement(3, 2), mm.getElement(3, 3));
      // have to split this up into components b/c Ogre::SceneNode has no direct way to set 4x4 transform
      Ogre::Vector3 position, scale;
      Ogre::Quaternion orientation;
      om.decomposition(position, scale, orientation);

      if (orientation.isNaN()) {
	LOG_DEBUG("COLLADA WARNING: the orientation appears to be gibberish!");
      } else {
	sn->setOrientation(orientation);
      }
      sn->setPosition(position);
      sn->setScale(scale);
    }
  }

  // collect the different types of child nodes
  const COLLADAFW::InstanceNodePointerArray& inodes = cn->getInstanceNodes();
  const COLLADAFW::InstanceGeometryPointerArray& ginodes = cn->getInstanceGeometries();
  const COLLADAFW::NodePointerArray& cnodes = cn->getChildNodes();

  // optimization: often (in Sketchup output, anyway) a library instance is the only child of a regular scene node
  // which supplies its transformation matrix.  in this case we can simply make build the instance in the current node,
  // rather than an added child node.  This makes the hierarchy clearer and cleaner for users to navigate

  // see if we can instantiate just one library node
  if ((ginodes.getCount() == 0) && (cnodes.getCount() == 0) && (inodes.getCount() == 1)) {
    LibNodesIterator lit = m_libNodes.find(inodes[0]->getInstanciatedObjectId());
    if ((lit != m_libNodes.end()) && (lit->second->getTransformations().getCount() == 0)) {
      // get the name of the referred-to library node
      LibNodesIterator lit = m_libNodes.find(inodes[0]->getInstanciatedObjectId());
      if (lit == m_libNodes.end()) {
	LOG_DEBUG("COLLADA WARNING: could not find library node with unique ID " + Ogre::StringConverter::toString(inodes[0]->getInstanciatedObjectId()));
	return false;
      }
      Ogre::String iname = sn->getName() + ":" + lit->second->getOriginalId();
      return processLibraryInstance(inodes[0], sn, iname + ":");
    }
  }


  // connect up library instances
  for (int i = 0, count = inodes.getCount(); i < count; ++i) {
    Ogre::String iname = prefix + "LibraryInstance_" + boost::lexical_cast<Ogre::String>(inodes[i]->getInstanciatedObjectId());
    Ogre::SceneNode* lsn = sn->createChildSceneNode(iname);
    processLibraryInstance(inodes[i], lsn, iname + ":");
  }

  // implement geometry instances
  for (int i = 0, count = ginodes.getCount(); i < count; ++i) {
    COLLADAFW::InstanceGeometry* gi = ginodes[i];
    std::map<COLLADAFW::UniqueId, Ogre::MeshPtr>::const_iterator mit = m_meshMap.find(gi->getInstanciatedObjectId());
    if (mit != m_meshMap.end()) {
      // so load it
      Ogre::MeshPtr m = mit->second;
      Ogre::String ename = prefix + m->getName();
      Ogre::Entity* e = m_sceneMgr->createEntity(ename, m->getName());
      MeshMaterialIdMapIterator mmapit = m_meshmatids.find(m);
      if (mmapit == m_meshmatids.end()) {
	LOG_DEBUG("Cannot find mesh material ids for mesh " + m->getName());
	continue;
      }
      // attach materials
      const COLLADAFW::MaterialBindingArray& mba = gi->getMaterialBindings();
      for (int i = 0, count = mba.getCount(); i < count; ++i) {
	const COLLADAFW::MaterialBinding mb = mba[i];
	// and each material binding has an array of texture bindings (!)
	const COLLADAFW::TextureCoordinateBindingArray& tba = mb.getTextureCoordinateBindingArray();
	for (int j = 0, tcount = tba.getCount(); j < tcount; ++j) {
	  const COLLADAFW::TextureCoordinateBinding& tcb = tba[j];
	  // BOZO seems like we should be doing something here!
	}
	// look up this material and make sure we can find it
	MaterialMapIterator matit = m_materials.find(mb.getReferencedMaterial());
	if (matit == m_materials.end()) {
	  LOG_DEBUG("material " + Ogre::StringConverter::toString(mb.getReferencedMaterial()) + " is not found in the stored materials");
	} else {
	  // check that this material has been defined - TBD
	  const Ogre::String& matname = matit->second.first;
	  // go through the subentities and see which ones have this material ID
	  bool found_mat_match = false;
	  for (int j = 0, mcount = mmapit->second.size(); j < mcount; ++j) {
	    if (mmapit->second[j] == mb.getMaterialId()) {
	      e->getSubEntity(j)->setMaterialName(matname);
	      found_mat_match = true;
	    }
	  }
	  if (!found_mat_match) {
	    LOG_DEBUG("instance geometry " + gi->getName() + " has no subentities matching material ID " + boost::lexical_cast<Ogre::String>(mb.getMaterialId()) + " for material name " + matname);
	  }
	}
      }

      sn->attachObject(e);

      if (m_calculateGeometryStats) {
	// stats
	if (m_geometryInstanceCounts.find(gi->getInstanciatedObjectId()) != m_geometryInstanceCounts.end()) {
	  m_geometryInstanceCounts[gi->getInstanciatedObjectId()]++;
	} else {
	  LOG_DEBUG("cannot find instanciated object of unique id " + boost::lexical_cast<Ogre::String>(gi->getInstanciatedObjectId()) + " for counting");
	}
      }
    } else {
      LOG_DEBUG("Geometry instance with object id " + boost::lexical_cast<std::string>(gi->getInstanciatedObjectId()) + " is NOT a mesh we know about");
    }
  }

  // for each regular child node:
  //     create the node, call recursively

  for (int i = 0, count = cnodes.getCount(); i < count; ++i) {
    Ogre::String cname = prefix + cnodes[i]->getOriginalId();
    if (!createSceneDFS(cnodes[i], sn->createChildSceneNode(cname), cname + ":"))
      return false;
  }

  return true;
}

// instantiate library node at the given Ogre SceneNode, assuming transformation is set for you
bool OgreSceneWriter::processLibraryInstance(const COLLADAFW::InstanceNode* inode,
					     Ogre::SceneNode* lsn, const Ogre::String& prefix) {
  // an instantiation of an entire subtree
  // follow the hierarchy (the regular node and its subtree) associated with this instance node by looking it up in the library nodes
  LibNodesIterator lit = m_libNodes.find(inode->getInstanciatedObjectId());
  if (lit == m_libNodes.end()) {
    LOG_DEBUG("COLLADA WARNING: could not find library node with unique ID " + Ogre::StringConverter::toString(inode->getInstanciatedObjectId()));
    return false;
  }
  // subtree copying.  Need to prefix downstream names with something to uniquify
  // using parent node ID for this
  // BOZO if we have more than one instantiation of the same library node in the same regular node, we get name duplication = badness
  // JET actually I think when this happens we always have a node that "owns" this instantiation
  // i.e., there is a <node> which has only the transformation matrix and the relevant
  // <instance_node>.  Each <node> has a unique name within its parent node.  (Need to verify
  // this is true)
  if (!lit->second->getName().empty()) {
    // store the type (name of the library node) as a property
    Ogre::UserObjectBindings& lsprops = lsn->getUserObjectBindings();
    lsprops.setUserAny("LibNodeType", Ogre::Any(lit->second->getName()));
  }

  if (!createSceneDFS(lit->second, lsn, prefix))
    return false;
 
 return true;
}
