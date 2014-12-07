// OgreMeshWriter.h, a class definition for Collada import into a single Ogre mesh
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2011 Aditazz, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <boost/lexical_cast.hpp>
#include <COLLADABUURI.h>
#include <COLLADAFWMatrix.h>
#include <COLLADAFWGeometry.h>
#include <COLLADAFWNode.h>
#include <COLLADAFWEffectCommon.h>
#include <OgreManualObject.h>
#include <OgreLogManager.h>
#include "OgreMeshWriter.h"

OgreCollada::MeshWriter::MeshWriter(const Ogre::String& dir) : Writer(dir, 0, false, false),
                                                               m_manobj(0)
{
  // create proxy writer objects we will supply to the Collada loader
  m_pass1Writer = new OgreMeshDispatchPass1(this);
  m_pass2Writer = new OgreMeshDispatchPass2(this);
}

OgreCollada::MeshWriter::~MeshWriter() {}

bool OgreCollada::MeshWriter::write(const COLLADAFW::Geometry* g) {
  // find where this geometry gets instantiated
  GeoUsageMapIter mit = m_geometryUsage.find(g->getUniqueId());
  if (mit == m_geometryUsage.end()) {
    LOG_DEBUG("the geometry with unique ID " + boost::lexical_cast<Ogre::String>(g->getUniqueId()) + " and original ID " + boost::lexical_cast<Ogre::String>(g->getOriginalId()) + " and name " + g->getName() + " has no recorded usage");
    return true;
  }
  for (GeoInstUsageListIter git = mit->second.begin(); git != mit->second.end(); ++git) {
    // add an instance of this geometry to the ManualObject with the specified transform
    if (!addGeometry(g, m_manobj, git->second, git->first))
      return false;
  }
  return true;
}

void OgreCollada::MeshWriter::pass1Finish() {
  // build scene graph and record transformations for each geometry instantiation

  // determine initial transformation.  Use Collada matrices etc. to avoid repeated reconversions to Ogre
  COLLADABU::Math::Quaternion orient(m_ColladaRotation.w, m_ColladaRotation.x, m_ColladaRotation.y, m_ColladaRotation.z);
  COLLADABU::Math::Vector3 scale(m_ColladaScale.x, m_ColladaScale.y, m_ColladaScale.z);
  COLLADABU::Math::Matrix4 xform;
  xform.makeTransform(COLLADABU::Math::Vector3::ZERO, scale, orient);

  // recursively find geometry instances and their transforms
  for (size_t i = 0; i < m_vsRootNodes.size(); ++i) {
    createSceneDFS(m_vsRootNodes[i], xform);
  }

  // create manualobject for use by pass2 writeGeometry calls
  m_manobj = new Ogre::ManualObject(m_vsRootNodes[0]->getName() + "_mobj");
}

void OgreCollada::MeshWriter::finishImpl() {
  createMaterials();

  // close manualobject and convert to mesh
  m_mesh = m_manobj->convertToMesh(m_vsRootNodes[0]->getName() + "_mesh");
}

// utility functions

// recursively build a table of geometry instances with ID and transform
// to be accessed when geometries are read in the second pass
bool OgreCollada::MeshWriter::createSceneDFS(const COLLADAFW::Node* cn,             // node to instantiate
				    const COLLADABU::Math::Matrix4& xform) // accumulated transform
{
  // apply this node's transformation matrix to the one inherited from its parent
  COLLADABU::Math::Matrix4 xn = xform;
  const COLLADAFW::TransformationPointerArray& tarr = cn->getTransformations();
  if (tarr.getCount() > 1) {
    LOG_DEBUG("COLLADA WARNING: Scene node has " + boost::lexical_cast<Ogre::String>(tarr.getCount()) + " transformations - we only handle 0 or 1");
  } else if (tarr.getCount() == 1) {
    if (tarr[0]->getTransformationType() != COLLADAFW::Transformation::MATRIX) {
      LOG_DEBUG("COLLADA WARNING: Scene node has non-matrix transformation - ignoring");
    } else {
      const COLLADAFW::Matrix& m = dynamic_cast<const COLLADAFW::Matrix&>(*tarr[0]);
      const COLLADABU::Math::Matrix4& mm = m.getMatrix();
      xn = xform * mm;
    }
  }

  // record any geometry instances present in this node, along with their attached materials
  // and cumulative transform
  const COLLADAFW::InstanceGeometryPointerArray& ginodes = cn->getInstanceGeometries();
  for (int i = 0, count = ginodes.getCount(); i < count; ++i) {
    COLLADAFW::InstanceGeometry* gi = ginodes[i];
    if (m_geometryUsage.find(gi->getInstanciatedObjectId()) == m_geometryUsage.end()) {
      // create empty usage list for this geometry
      m_geometryUsage.insert(std::make_pair(gi->getInstanciatedObjectId(), GeoInstUsageList()));
    }
    Ogre::Matrix4 xform_ogre(xn.getElement(0, 0), xn.getElement(0, 1), xn.getElement(0, 2), xn.getElement(0, 3),
			     xn.getElement(1, 0), xn.getElement(1, 1), xn.getElement(1, 2), xn.getElement(1, 3),
			     xn.getElement(2, 0), xn.getElement(2, 1), xn.getElement(2, 2), xn.getElement(2, 3),
			     xn.getElement(3, 0), xn.getElement(3, 1), xn.getElement(3, 2), xn.getElement(3, 3));

    m_geometryUsage[gi->getInstanciatedObjectId()].push_back(std::make_pair(&(gi->getMaterialBindings()), xform_ogre));
  }

  // recursively follow child nodes and library instances
  const COLLADAFW::NodePointerArray& cnodes = cn->getChildNodes();
  for (int i = 0, count = cnodes.getCount(); i < count; ++i) {
    if (!createSceneDFS(cnodes[i], xn))
      return false;
  }

  const COLLADAFW::InstanceNodePointerArray& inodes = cn->getInstanceNodes();
  for (int i = 0, count = inodes.getCount(); i < count; ++i) {
    LibNodesIterator lit = m_libNodes.find(inodes[i]->getInstanciatedObjectId());
    if (lit == m_libNodes.end()) {
      LOG_DEBUG("COLLADA WARNING: could not find library node with unique ID " +
		boost::lexical_cast<Ogre::String>(inodes[i]->getInstanciatedObjectId()));
      continue;
    }
    if (!createSceneDFS(lit->second, xn))
      return false;
  }

  return true;
}
