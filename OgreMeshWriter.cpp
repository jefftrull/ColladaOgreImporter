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
#include <COLLADAFWScale.h>
#include <COLLADAFWRotate.h>
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

bool OgreCollada::MeshWriter::writeGeometry(const COLLADAFW::Geometry* g) {
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

  // determine initial transformation
  Ogre::Matrix4 xform;
  xform.makeTransform(Ogre::Vector3::ZERO,    // no translation
                      Ogre::Vector3(m_ColladaScale.x, m_ColladaScale.y, m_ColladaScale.z),
                      Ogre::Quaternion(m_ColladaRotation.w, m_ColladaRotation.x, m_ColladaRotation.y, m_ColladaRotation.z));

  // recursively find geometry instances and their transforms
  for (size_t i = 0; i < m_vsRootNodes.size(); ++i) {
    createSceneDFS(m_vsRootNodes[i], xform);
  }

  // create manualobject for use by pass2 writeGeometry calls
  m_manobj = new Ogre::ManualObject(m_vsRootNodes[0]->getName() + "_mobj");
}

void OgreCollada::MeshWriter::finish() {
  createMaterials();

  // close manualobject and convert to mesh
  m_mesh = m_manobj->convertToMesh(m_vsRootNodes[0]->getName() + "_mesh");
}

// utility functions

// recursively build a table of geometry instances with ID and transform
// to be accessed when geometries are read in the second pass
bool OgreCollada::MeshWriter::createSceneDFS(const COLLADAFW::Node* cn,  // node to instantiate
				             Ogre::Matrix4 xn)           // accumulated transform
{
  // apply this node's transformation matrix to the one inherited from its parent
  const COLLADAFW::TransformationPointerArray& tarr = cn->getTransformations();
  // COLLADA spec says multiple transformations are "postmultiplied in the order in which
  // they are specified", which I think means like this:
  for (size_t i = 0; i < tarr.getCount(); ++i) {
    xn = xn * computeTransformation(tarr[i]);
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
    m_geometryUsage[gi->getInstanciatedObjectId()].push_back(std::make_pair(&(gi->getMaterialBindings()), xn));
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
