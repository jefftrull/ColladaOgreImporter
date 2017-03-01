// Implementation of Collada to Ogre translation
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2010 Aditazz, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "OgreColladaWriter.h"

#include <OgreMeshManager.h>
#include <OgrePass.h>
#include <OgreTechnique.h>
#include <OgreMaterialManager.h>
#include <OgreTextureManager.h>
#include <OgreSubMesh.h>
#include <OgreMesh.h>
#include <OgreManualObject.h>
#include <OgreLogManager.h>
#include <OgreColourValue.h>
#include <OgreUserObjectBindings.h>

#include <COLLADAFWFileInfo.h>
#include <COLLADAFWColorOrTexture.h>
#include <COLLADAFWSampler.h>
#include <COLLADAFWEffectCommon.h>
#include <COLLADAFWEffect.h>
#include <COLLADAFWLibraryNodes.h>
#include <COLLADAFWMesh.h>
#include <COLLADAFWGeometry.h>
#include <COLLADAFWMaterial.h>
#include <COLLADAFWImage.h>
#include <COLLADAFWVisualScene.h>

#include <iostream>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

typedef COLLADAFW::FileInfo FileInfo;
typedef COLLADAFW::FileInfo::Unit Unit;

// copied out of OgreCollada
#define LOG_DEBUG(msg) { Ogre::LogManager::getSingleton().logMessage( Ogre::String((msg)) ); }

OgreCollada::Writer::Writer(const Ogre::String& dir, const char* dotfn,
				     bool checkNormals, bool calculateGeometryStats) :
  m_dir(dir), m_dotfn(dotfn),
  m_checkNormals(checkNormals), m_calculateGeometryStats(calculateGeometryStats),
  m_transparencyWorkarounds(false) {
  // prepare to load textures from the specified directory
  if (boost::filesystem::exists(m_dir)) {
    LOG_DEBUG("adding directory " + m_dir + " to resources");
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(m_dir, "FileSystem", "General");
  } else {
    LOG_DEBUG("specified directory " + m_dir + " does not exist");
  }
}

OgreCollada::WriterBase::~WriterBase() {}

void OgreCollada::Writer::cancel(const COLLADAFW::String&) {}

void OgreCollada::Writer::start() {
}

// each type of color (ambient, specular, diffuse, etc.) gets handled very similarly
void OgreCollada::Writer::handleColorOrTexture(const COLLADAFW::EffectCommon& ce,
					   const COLLADAFW::ColorOrTexture& ct,
					   Ogre::Pass* pass,
					   ColorSetter setColor,
					   Ogre::TrackVertexColourType ctype) {
  if (ct.isColor()) {
    Ogre::ColourValue color(ct.getColor().getRed(), ct.getColor().getGreen(),
			    ct.getColor().getBlue(), ct.getColor().getAlpha());
    (pass->*setColor)(color);   // call appropriate setter member function for this pass
    pass->setVertexColourTracking(pass->getVertexColourTracking() & ~ctype);
  } else if (ct.isTexture()) {
    // look up in our texture lib and make sure we can find it
    // first we have to find out which sampler it is
    COLLADAFW::SamplerID sampid = ct.getTexture().getSamplerId();
    COLLADAFW::Sampler* samp = ce.getSamplerPointerArray()[sampid];
    COLLADAFW::UniqueId imageid = samp->getSourceImage();
    // make sure this image ID has been stored by us previously (in a writeImage call)
    ImageMapIterator iit = m_images.find(imageid);
    if (iit == m_images.end()) {
      LOG_DEBUG("could not find image " + Ogre::StringConverter::toString(imageid) + " for texture");
      return;
    }
    Ogre::TextureUnitState* tus = pass->createTextureUnitState();
    tus->setTextureName(iit->second);
    tus->setColourOperation(Ogre::LBO_ALPHA_BLEND);
  }
}

void OgreCollada::Writer::createMaterials() {
  // At this point we have both the materials and their referenced effects.  Let's create them in Ogre so we can assign
  // them to submeshes when we instantiate the scene graph
  for (MaterialMapIterator matit = m_materials.begin(); matit != m_materials.end(); ++matit) {
    Ogre::String matname = matit->second.first;
    COLLADAFW::UniqueId effid = matit->second.second;
    EffectMapIterator effit = m_effects.find(effid);
    if (effit == m_effects.end()) {
      LOG_DEBUG("Could not find effect " + Ogre::StringConverter::toString(effid) + " in storage");
      continue;
    } else {
      Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create(matname, "General");      
      // for now, one technique and one pass.  Get the ones supplied automatically upon Material creation:
      Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
      if (std::find(m_unculledEffects.begin(), m_unculledEffects.end(), effid) != m_unculledEffects.end()) {
        // This effect was marked "double_sided" in an <extra> tag within the effect.  Disable backside culling.
        pass->setCullingMode(Ogre::CULL_NONE);
      }
      for (int i = 0, count = effit->second.size(); i < count; ++i) {
	if (i > 0) {
	  pass = mat->getTechnique(0)->createPass();   // need a new pass for each effect
	}
	const COLLADAFW::EffectCommon& ce = effit->second[i];
	if ((ce.getShaderType() == COLLADAFW::EffectCommon::SHADER_BLINN) || // not natively supported in Ogre
	    (ce.getShaderType() == COLLADAFW::EffectCommon::SHADER_PHONG)) {
	  pass->setShadingMode(Ogre::SO_PHONG);
	} else if ((ce.getShaderType() == COLLADAFW::EffectCommon::SHADER_CONSTANT) ||
		   (ce.getShaderType() == COLLADAFW::EffectCommon::SHADER_LAMBERT)) {
	  // FCollada uses Gouraud for this
	  pass->setShadingMode(Ogre::SO_GOURAUD);
	} else {
	  LOG_DEBUG("COLLADA WARNING: Unknown shader type for effect" + Ogre::StringConverter::toString(effid));
	}

	Ogre::ColourValue opacity;
        if (ce.getOpacity().isColor()) {
          opacity = Ogre::ColourValue(
            (Ogre::Real)ce.getOpacity().getColor().getRed(),
            (Ogre::Real)ce.getOpacity().getColor().getGreen(),
            (Ogre::Real)ce.getOpacity().getColor().getBlue(),
            (Ogre::Real)ce.getOpacity().getColor().getAlpha());
          // SketchUp prior to 7.1 inverts transparency, as do many versions of the FBX exporter
          // I'm not certain this is correct in all cases as there is opaque mode to consider
          if (m_transparencyWorkarounds) {
            opacity.r = 1.0 - opacity.r;
            opacity.g = 1.0 - opacity.g;
            opacity.b = 1.0 - opacity.b;
            opacity.a = 1.0 - opacity.a;
          }
        }

	bool transparency = ce.getOpacity().isColor() && ((opacity.r < 1.0) ||
							  (opacity.g < 1.0) ||
							  (opacity.b < 1.0));
	handleColorOrTexture(ce, ce.getAmbient(), pass, &Ogre::Pass::setAmbient, Ogre::TVC_AMBIENT);
	handleColorOrTexture(ce, ce.getDiffuse(), pass, &Ogre::Pass::setDiffuse, Ogre::TVC_DIFFUSE);
	handleColorOrTexture(ce, ce.getSpecular(), pass, &Ogre::Pass::setSpecular, Ogre::TVC_SPECULAR);
	handleColorOrTexture(ce, ce.getEmission(), pass, &Ogre::Pass::setSelfIllumination, Ogre::TVC_EMISSIVE);

	// BOZO why do we create Ambient, Diffuse, Emission, and Specular textures identically?
	// are they used the same in Ogre?

	// handle shininess
	if (ce.getShininess().getType() == COLLADAFW::FloatOrParam::FLOAT)
	  pass->setShininess(ce.getShininess().getFloatValue());

	if (transparency) {
          if(ce.getDiffuse().isColor()) {
            pass->setDiffuse(pass->getDiffuse().r, pass->getDiffuse().g, pass->getDiffuse().b,
                             opacity.a * pass->getDiffuse().a);
          } else {
            // Trying to reverse-engineer the equations in the Collada spec
            // Using "modulate" (the default) with an initial color the same as the opacity appears IMO
            // to match the equations for the case where there is only a transparent texture
            // note that OpenCollada precalculates the channel blending factors for you as part of
            // the returned opacity color, so we don't need to know whether we're RGB_ZERO etc.
            pass->setDiffuse(opacity);
          }

	  // as done by FCollada, IZ, etc.
	  // for constant colors the alpha value of the color will determine transparency
	  // from textures the alpha value comes from the "opacity" (<transparent> tag) color (above)
	  pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
	  pass->setDepthWriteEnabled(false);  // this has plusses and minuses
	} else if (ce.getOpacity().isTexture()) {
	  LOG_DEBUG("effect " + Ogre::StringConverter::toString(effid) + " has an opacity texture, which is presently unsupported");
	}
      }
      m_ogreMaterials.push_back(mat);
    }
  }
}

void OgreCollada::Writer::disableCulling(COLLADAFW::UniqueId const& uid) {
  m_unculledEffects.push_back(uid);
}

bool OgreCollada::Writer::writeGlobalAsset(const FileInfo* fi) {
  enum FileInfo::UpAxisType up = fi->getUpAxisType();
  // set transform of entire imported scene to match Ogre (Y-up)
  if (up == FileInfo::X_UP) {
    // Let Ogre calculate how to fix this
    m_ColladaRotation = Ogre::Vector3(1, 0, 0).getRotationTo(Ogre::Vector3(0, 1, 0));
  } else if (up == FileInfo::Y_UP) {
    // we are good, no change required.  Ogre is Y-up by default and quaternion's default constructor gives "no rotation"
  } else if (up == FileInfo::Z_UP) {
    m_ColladaRotation = Ogre::Vector3(0, 0, 1).getRotationTo(Ogre::Vector3(0, 1, 0));
  }

  double scale = fi->getUnit().getLinearUnitMeter();
  m_ColladaScale = Ogre::Vector3(scale, scale, scale);

  // Look for authoring tool to set up quirks for bugs
  const COLLADAFW::FileInfo::ValuePairPointerArray& values = fi->getValuePairArray();
  for (size_t i = 0; i < values.getCount(); ++i) {
    if (values[i]->first == "authoring_tool") {
      boost::regex sketchup_regex("Google SketchUp (\\d+)\\.(\\d+)(\\.\\d+)?");
      boost::smatch comps;
      if (boost::regex_match(values[i]->second, comps, sketchup_regex)) {
        int maj = std::stoi(comps[1]);
        int min = std::stoi(comps[2]);
        if ((maj < 7) || ((maj == 7) && (min < 1))) {
          // Prior to version 7.1 SketchUp had a number of Collada export bugs
          m_transparencyWorkarounds = true;
        }
      } else if (values[i]->second == "FBX COLLADA exporter") {
        // FBX (no version string, sadly)
        m_transparencyWorkarounds = true;
      }
    }
  }
  return true;
}

bool OgreCollada::Writer::writeScene(const COLLADAFW::Scene* scene) {
  return true;
}

static void do_indent(int level) {
  for (int i = 0; i < level; ++i) {
    LOG_DEBUG("  ");
  }
}

bool OgreCollada::Writer::writeLibraryNodes(const COLLADAFW::LibraryNodes* lnodes) {
  for (int i = 0, count = lnodes->getNodes().getCount(); i < count; ++i) {
    const COLLADAFW::Node* n = lnodes->getNodes()[i];
    // record this name and unique ID
    if (n->getName() == "") {
      // seems strange that a library inst wouldn't have a name
      LOG_DEBUG("WEIRD: library node OID " + n->getOriginalId() + " ID " + Ogre::StringConverter::toString(n->getUniqueId()) + " has no name!");
      continue;
    }
    // record this library node so instance nodes can refer to it later
    m_libNodes.insert(std::make_pair(n->getUniqueId(), n));
  }

  return true;
}
bool OgreCollada::Writer::addGeometry(const COLLADAFW::Geometry* g,       // input geometry from Collada
				    Ogre::ManualObject* manobj,           // object under construction
				    const Ogre::Matrix4& xform,           // transform within the object
				    const COLLADAFW::MaterialBindingArray* mba) {

  const COLLADAFW::Mesh* cmesh = dynamic_cast<const COLLADAFW::Mesh*>(g);
  Ogre::Matrix3 rotscale; xform.extract3x3Matrix(rotscale);   // normals don't get translation

  int triangles = 0, lines = 0;   // for geometry stats

  // iterate over mesh primitives and output
  const COLLADAFW::MeshVertexData& posvdata = cmesh->getPositions();
  const COLLADAFW::MeshVertexData& normvdata = cmesh->getNormals();
  const COLLADAFW::FloatArray& pvals = *(posvdata.getFloatValues());
  const COLLADAFW::FloatArray& nvals = *(normvdata.getFloatValues());
  bool valid_submesh = false;
  if (cmesh->getMeshPrimitives().getCount() == 0) {
    LOG_DEBUG("Mesh primitive count for geometry " + boost::lexical_cast<Ogre::String>(g->getOriginalId()) + " is zero; I won't produce a valid mesh...");
  }
  for (int i = 0, count = cmesh->getMeshPrimitives().getCount(); i < count; ++i) {
    const COLLADAFW::MeshPrimitive& prim = *(cmesh->getMeshPrimitives()[i]);
    if ((prim.getPrimitiveType() != COLLADAFW::MeshPrimitive::TRIANGLES) &&
	(prim.getPrimitiveType() != COLLADAFW::MeshPrimitive::LINES) &&
        (prim.getPrimitiveType() != COLLADAFW::MeshPrimitive::POLYLIST)) {
      // BOZO get proper Ogre error message mechanism here
      LOG_DEBUG("Mesh primitive type " + Ogre::StringConverter::toString(prim.getPrimitiveType()) + " is not currently supported");
      continue;
    }
    if (!cmesh->getPositions().getValuesCount()) {
      LOG_DEBUG("Mesh primitive has no positions; this is strange (and currently unsupported), skipping");
      continue;
    }

    if (prim.getPrimitiveType() == COLLADAFW::MeshPrimitive::POLYLIST) {
      // check vertex counts to ensure they are all actually triangles
      bool all_triangles = true;
      for (size_t i = 0; i < prim.getGroupedVertexElementsCount(); ++i) {
        if (prim.getGroupedVerticesVertexCount(i) != 3) {
          all_triangles = false;
          LOG_DEBUG("a polylist mesh primitive contains a polygon that is not a triangle, which is unsupported - skipping");
          break;
        }
      }
      if (!all_triangles) {
        continue;
      }
    }

    Ogre::String matname("BaseWhiteNoLighting");
    if (mba) {
      // try to use the supplied material binding array to identify the material to apply to this submesh
      COLLADAFW::MaterialId matid = prim.getMaterialId();
      // find any matching entry in the binding array for this instance
      bool found_mat_match = false;
      for (size_t j = 0; j < mba->getCount(); ++j) {
	const COLLADAFW::MaterialBinding mb = (*mba)[j];
	if (mb.getMaterialId() == matid) {
	  MaterialMapIterator matit = m_materials.find(mb.getReferencedMaterial());
	  if (matit == m_materials.end()) {
	    LOG_DEBUG("COLLADA WARNING: geometry " + g->getOriginalId() + " refers to material " +
		      boost::lexical_cast<Ogre::String>(mb.getReferencedMaterial()) + " as material " +
		      boost::lexical_cast<Ogre::String>(mb.getMaterialId()) +
		      " but it cannot be found in the materials map");
	  } else {
	    found_mat_match = true;
	    matname = matit->second.first;
	  }
	}
      }
      if (!found_mat_match) {
	LOG_DEBUG("COLLADA WARNING: geometry  " + g->getOriginalId() + " refers to material Id " +
		  boost::lexical_cast<Ogre::String>(prim.getMaterialId()) +
		  " but it cannot be found in the supplied material bindings.  Using BaseWhiteNoLighting");
      }
    }


    if ((prim.getPrimitiveType() == COLLADAFW::MeshPrimitive::TRIANGLES) ||
        (prim.getPrimitiveType() == COLLADAFW::MeshPrimitive::POLYLIST)) {
      manobj->begin(matname, Ogre::RenderOperation::OT_TRIANGLE_LIST);
    }
    else {
      manobj->begin(matname, Ogre::RenderOperation::OT_LINE_LIST);
    }
 
    // Reorder the vertex buffer for this submesh
    // basically we're going to create a vertex buffer and a set of indices on the fly
    // from the existing sets of values.  The vertex buffer will have only those values used by this
    // particular primitive/submesh
    // we can have any of positions, normals, colors, or UV coordinates for textures
    // FCollada deals with this by calculating a hash of all existing indices
    // I'm paranoid about collisions and prefer to implement this with an exhaustive data structure
    // that's a little slower.  I think a std::map with a vector index (giving the tuple of indices),
    // though slow, is the safest and quickest way to get this going.

    std::vector<const COLLADAFW::UIntValuesArray*> collada_indices;
    typedef std::map<std::vector<unsigned int>, Ogre::uint32> Collada2OgreIndexMap;
    typedef Collada2OgreIndexMap::const_iterator Collada2OgreIndexMapIter;
    Collada2OgreIndexMap collada2ogreidx;

    bool hasNormals = prim.hasNormalIndices();
    bool hasUVs = prim.hasUVCoordIndices();

    collada_indices.push_back(&prim.getPositionIndices());
    if (hasNormals) {
      collada_indices.push_back(&prim.getNormalIndices());
    }
    // TBD stick colors in here
    if (hasUVs) {
      // a single vertex can have multiple texture coordinates.  Each one needs to be part of
      // the generated index and vertex buffer.
      // BOZO just handling one for now
      collada_indices.push_back(&prim.getUVCoordIndices(0)->getIndices());
    }

    // quick check: all of the index arrays should have the same size
    size_t idxsize = collada_indices[0]->getCount();
    for (size_t arridx = 0; arridx < collada_indices.size(); arridx++) {
      if (collada_indices[arridx]->getCount() != idxsize) {
	LOG_DEBUG("size of index array " + Ogre::StringConverter::toString(arridx) + " is " + Ogre::StringConverter::toString(collada_indices[arridx]->getCount()) + " which disagrees with the first index array size of " + Ogre::StringConverter::toString(idxsize));
      }
    }

    // now build vertex data while creating new indices
    std::vector<std::vector<Ogre::Real> > vertices;  // data for resulting vertex buffer
    std::vector<Ogre::uint32> indices;               // resultant indices
    for (size_t ci = 0; ci < idxsize; ++ci) {           // loop over current (N-tuple) indices
      // construct map key
      std::vector<unsigned int> multi_idx_key;
      for (int idxno = 0, idxcount = collada_indices.size(); idxno < idxcount; ++idxno) {
	// strangely collada_indices[idxno]->[ci] does not work here...
	multi_idx_key.push_back((*collada_indices[idxno])[ci]);
      }
      Ogre::uint32 idx;
      Collada2OgreIndexMapIter it = collada2ogreidx.find(multi_idx_key);
      if (it == collada2ogreidx.end()) {
	// create a new index translation
	idx = vertices.size();
	collada2ogreidx.insert(std::make_pair(multi_idx_key, idx));

	// assemble and add new vertex value
	std::vector<Ogre::Real> vval;
	// position (always).  Assuming 3 floats per usual
	int voffset = 0;   // offset of data item within keys. moves as we go through data types
	Ogre::Vector3 pos_xformed = xform * Ogre::Vector3(pvals[3*multi_idx_key[voffset]+0],
							  pvals[3*multi_idx_key[voffset]+1],
							  pvals[3*multi_idx_key[voffset]+2]);
	vval.push_back(pos_xformed.x);
	vval.push_back(pos_xformed.y);
	vval.push_back(pos_xformed.z);
	++voffset;
	if (hasNormals) {
	  Ogre::Vector3 norm_xformed = rotscale * Ogre::Vector3(nvals[3*multi_idx_key[voffset]+0],
								nvals[3*multi_idx_key[voffset]+1],
								nvals[3*multi_idx_key[voffset]+2]);
	  norm_xformed.normalise();
	  vval.push_back(norm_xformed.x);
	  vval.push_back(norm_xformed.y);
	  vval.push_back(norm_xformed.z);
	  ++voffset;
	}
	// colors here
	if (hasUVs) {
	  // BOZO go back and handle more than one texture coord. use voffset on each
	  // BOZO read inputinfos also (for now assume 2 floats)
	  vval.push_back((*cmesh->getUVCoords().getFloatValues())[2*multi_idx_key[voffset]+0]);
	  vval.push_back(1.f - (*cmesh->getUVCoords().getFloatValues())[2*multi_idx_key[voffset]+1]);  // Y needs to be flipped for Ogre
	  ++voffset;
	}
	vertices.push_back(vval);
      } else {
	// we already have this one; just reference it
	idx = it->second;
      }
      indices.push_back(idx);
    }

    // output vertex buffers
    // this will only be the ones used by this submesh, because we accumulated the list of submesh vertices from
    // the master list (mesh-global) list as we built the list of indices

    for (int v = 0, vcount = vertices.size(); v < vcount; ++v) {
      int voffset = 0;
      // position always
      manobj->position(vertices[v][voffset+0], vertices[v][voffset+1], vertices[v][voffset+2]);
      voffset += 3;
      if (hasNormals) {
	manobj->normal(vertices[v][voffset+0], vertices[v][voffset+1], vertices[v][voffset+2]);
	voffset += 3;
      }
      if (hasUVs) {
	manobj->textureCoord(vertices[v][voffset+0], vertices[v][voffset+1]);
	voffset += 2;
      }
    }

    // now the indices
    if (((prim.getPrimitiveType() == COLLADAFW::MeshPrimitive::TRIANGLES) ||
         (prim.getPrimitiveType() == COLLADAFW::MeshPrimitive::POLYLIST))
         && hasNormals) {
      // output indices with a possibility of correcting winding order
      for (int tri = 0, tcount = indices.size() / 3; tri < tcount; ++tri) {
	// get the three vertices defining this triangle
	const std::vector<Ogre::Real>& v1 = vertices[indices[3*tri+0]];
	const std::vector<Ogre::Real>& v2 = vertices[indices[3*tri+1]];
	const std::vector<Ogre::Real>& v3 = vertices[indices[3*tri+2]];
	// extract the vertex normals
	Ogre::Vector3 n1(v1[3], v1[4], v1[5]);    // normals always come right after positions
	Ogre::Vector3 n2(v2[3], v2[4], v2[5]);
	Ogre::Vector3 n3(v3[3], v3[4], v3[5]);

	// check that the vertex normals are all the same (may not be required?)
	if (m_checkNormals && ((n1 == n2) && (n2 == n3))) {
	  // can only check these against the CCW winding normal if they are consistent
	  // calculate the surface normal assuming CCW winding
	  // following the description here: http://www.opengl.org/wiki/Calculating_a_Surface_Normal
	  // however, there is supposed to be some simpler way of performing this check that doesn't require
	  // so much vector math
	  Ogre::Vector3 p1(v1[0], v1[1], v1[2]);    // normals always come right after positions
	  Ogre::Vector3 p2(v2[0], v2[1], v2[2]);
	  Ogre::Vector3 p3(v3[0], v3[1], v3[2]);
	  Ogre::Vector3 U = p2 - p1;
	  Ogre::Vector3 V = p3 - p1;
	  Ogre::Vector3 sn = U.crossProduct(V);
	  // if the surface normal and the vertex normals are more than 90 degrees apart, assume the winding order is wrong
	  if (sn.dotProduct(n1) < 0) {
	    LOG_DEBUG("COLLADA WARNING: surface normal " + Ogre::StringConverter::toString(sn) + " calculated from vertices " +
		      Ogre::StringConverter::toString(p1) + ", " +
		      Ogre::StringConverter::toString(p2) + ", " +
		      Ogre::StringConverter::toString(p3) + ", " +
		      " points in the opposite direction of the Collada-supplied vertex normals " + Ogre::StringConverter::toString(n1));
	  }
	}
	// output triangle vertex indices in the supplied order
	manobj->index(indices[3*tri+0]);
	manobj->index(indices[3*tri+1]);
	manobj->index(indices[3*tri+2]);
      }
    } else {
      // simply output the indices as we see them
      for (int j = 0, icount = indices.size(); j < icount; ++j) {
	manobj->index(indices[j]);
      }
    }
    if (m_calculateGeometryStats) {
      // update stats
      if ((prim.getPrimitiveType() == COLLADAFW::MeshPrimitive::TRIANGLES) ||
          (prim.getPrimitiveType() == COLLADAFW::MeshPrimitive::POLYLIST)) {
	triangles += (indices.size() / 3);
      }
      else {
	lines += (indices.size() / 2);
      }
    }
    manobj->end();
    valid_submesh = true;
  }

  if (m_calculateGeometryStats) {
    // update stats
    m_geometryTriangleCounts[g->getUniqueId()] = triangles;
    m_geometryLineCounts[g->getUniqueId()] = lines;
  }

  if (!valid_submesh)
    LOG_DEBUG("not returning a valid submesh for geometry " + boost::lexical_cast<Ogre::String>(g->getOriginalId()));
  return valid_submesh;
}

bool OgreCollada::Writer::writeMaterial(const COLLADAFW::Material* m) {

  m_materials.insert(std::make_pair(m->getUniqueId(), std::make_pair(m->getName(), m->getInstantiatedEffect())));

  return true;
}

bool OgreCollada::Writer::writeEffect(const COLLADAFW::Effect* e) {
  COLLADAFW::Color c = e->getStandardColor();
  for (int i = 0, count = e->getCommonEffects().getCount(); i < count; ++i) {
    const COLLADAFW::EffectCommon* ce = e->getCommonEffects()[i];
    // record this effect for future lookup when materials (which reference them) are processed
    m_effects[e->getUniqueId()].push_back(*ce);
  }

  return true;
}
bool OgreCollada::Writer::writeCamera(const COLLADAFW::Camera*) {
  return true;
}
bool OgreCollada::Writer::writeImage(const COLLADAFW::Image* i) {
  // these are basically texture jpegs... they contain a file path
  if (i->getSourceType() == COLLADAFW::Image::SOURCE_TYPE_URI) {
    Ogre::String image_rel_path = COLLADABU::URI::uriDecode(i->getImageURI().getURIString());
    LOG_DEBUG("URI: " + image_rel_path);
    // this is the only type we currently support
    // Ogre wants to load base name files (without paths) from directories that have already been registered.
    // We normally see our textures in subdirectories.  Trim off the path
    Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().load(image_rel_path, "General");
    if (texture.isNull()) {
      LOG_DEBUG("COLLADA WARNING: Failed to load texture from file " + image_rel_path);
    } else {
      m_images.insert(std::make_pair(i->getUniqueId(), texture->getName()));
    }
  } else {
    LOG_DEBUG("OgreCollada::Writer::writeImage called on OID " + i->getOriginalId() + " uniqueid " + Ogre::StringConverter::toString(i->getUniqueId()) + " name " + i->getName() + " source type ");
    if (i->getSourceType() == COLLADAFW::Image::SOURCE_TYPE_DATA) {
      LOG_DEBUG("DATA");
    } else {
      LOG_DEBUG("UNKNOWN");
    }
    LOG_DEBUG("which is unsupported");
  }
  return true;
}

bool OgreCollada::Writer::writeVisualScene(const COLLADAFW::VisualScene* vscene) {
  // store those root nodes for later processing
  for (int i = 0, count = vscene->getRootNodes().getCount(); i < count; ++i) {
    m_vsRootNodes.push_back(vscene->getRootNodes()[i]);
  }

  return true;
}

// utility/debug functions

void OgreCollada::Writer::node_dfs_print(const COLLADAFW::Node* n, int level) {
  do_indent(level);
  LOG_DEBUG("OID " + n->getOriginalId() + " UniqueID " + Ogre::StringConverter::toString(n->getUniqueId()) + " Name " + n->getName());
  if (n->getChildNodes().getCount()) {
    do_indent(level);
    LOG_DEBUG(Ogre::String("with ") + Ogre::StringConverter::toString(n->getChildNodes().getCount()) + " child nodes:");
    for (int i = 0, count = n->getChildNodes().getCount(); i < count; ++i) {
      node_dfs_print(n->getChildNodes()[i], level+1);
    }
  }
  if (n->getInstanceNodes().getCount()) {
    do_indent(level);
    LOG_DEBUG(Ogre::String("with ") + Ogre::StringConverter::toString(n->getInstanceNodes().getCount()) + " instance nodes:");
    for (int i = 0, count = n->getInstanceNodes().getCount(); i < count; ++i) {
      const COLLADAFW::InstanceNode* in = n->getInstanceNodes()[i];
      // it seems instance nodes don't have children... I'm thinking (hoping?) they are only ever references to library nodes
      do_indent(level+1);
      // also no "original ID", but an "instantiated object id" instead
      LOG_DEBUG(Ogre::String("ID ") + Ogre::StringConverter::toString(in->getInstanciatedObjectId()) + " Name " + in->getName());
      LibNodesIterator it = m_libNodes.find(in->getInstanciatedObjectId());
      if (it == m_libNodes.end()) {
	LOG_DEBUG(" (NOT FOUND IN LIBRARY)");
      } else {
	LOG_DEBUG(" (library elt " + it->second->getName() + " )");
      }
    }
  }
}

void OgreCollada::Writer::dump_as_dot(std::ostream& os) {
  // dump instance hierarchy as DOT
  os << "digraph OgreScene {\n";
  os << "ratio=0.1\n";   // based on messing with the result

  int nodeid = 0;
  std::map<const COLLADAFW::Node*, int> nodeids;
  // all the library nodes first with their labels
  for (LibNodesIterator mit = m_libNodes.begin();
       mit != m_libNodes.end(); ++mit, ++nodeid) {
    os << "node" << nodeid << " [label=\"" << mit->second->getName() << "\"]\n";
    nodeids.insert(std::make_pair(mit->second, nodeid));
  }

  // having established node numbers for the library nodes, we can now process their children
  for (LibNodesIterator mit = m_libNodes.begin();
       mit != m_libNodes.end(); ++mit) {
    nodeid = nodeids[mit->second];  // this way instead of counter because I'm not sure if iter order repeatable
    os << "node" << nodeid << " [label=\"" << mit->second->getName() << "\"]\n";
    // now its children
    const COLLADAFW::Node* n = mit->second;
    node_dfs_dot(os, n, nodeid, nodeids);
  }

  // output the root nodes with their names as label
  int root_node_id_ctr = m_libNodes.size();
  for (int i = 0, count = m_vsRootNodes.size(); i < count; ++i) {
    os << "node" << root_node_id_ctr << " [label=\"" << m_vsRootNodes[i]->getName() << "\"]\n";
    node_dfs_dot(os, m_vsRootNodes[i], root_node_id_ctr++, nodeids);
  }

  os << "}\n";
}

// just the instance hierarchy for now.
void OgreCollada::Writer::node_dfs_dot(std::ostream& os, const COLLADAFW::Node* n,
				   int parentid, const std::map<const COLLADAFW::Node*, int>& nodeids) {
  // if this node has instance nodes, they are all children of this node's parent
  const COLLADAFW::InstanceNodePointerArray& inodes = n->getInstanceNodes();
  for (int i = 0, count = inodes.getCount(); i < count; ++i) {
    const COLLADAFW::InstanceNode* in = inodes[i];
    // look this thing up in the stored library node list
    LibNodesIterator it = m_libNodes.find(in->getInstanciatedObjectId());
    if (it == m_libNodes.end()) {
      LOG_DEBUG("could not find library ID " + Ogre::StringConverter::toString(in->getUniqueId()) + " in the library list");
      continue;
    }
    int nodeid = nodeids.find(it->second)->second;
    if (parentid != -1) {
      // if a "cell", use a square for the shape
      os << "node" << parentid << " -> node" << nodeid << std::endl;
    }
  }
  // now process child nodes (basically by searching for most instances downstream
  // notice that instance nodes terminate the recursion, but regular nodes don't
  const COLLADAFW::NodePointerArray& cnodes = n->getChildNodes();
  for (int i = 0, count = cnodes.getCount(); i < count; ++i) {
    node_dfs_dot(os, cnodes[i], parentid, nodeids);
  }
}

// traverse node hierarchy from either 1) root node or 2) some instance root
// and make sure we have geometries stored as meshes
void OgreCollada::Writer::node_dfs_geocheck(const COLLADAFW::Node* n) {
  // check instantiated geometries hanging off this node
  const COLLADAFW::InstanceGeometryPointerArray& gnodes = n->getInstanceGeometries();
  for (int i = 0, count = gnodes.getCount(); i < count; ++i) {
    const COLLADAFW::InstanceGeometry* gn = gnodes[i];
    // look this thing up in our geometry uniqueid to mesh ptr map
    std::map<COLLADAFW::UniqueId, Ogre::MeshPtr>::iterator it = m_meshMap.find(gn->getInstanciatedObjectId());
    if (it == m_meshMap.end()) {
      // even if we don't have a mesh constructed (or loaded) for this, we should still have recorded its geometry
      // when it originally appeared in the input.  Use this information to make a nicer error message
      std::map<COLLADAFW::UniqueId, Ogre::String>::const_iterator git = m_geometryNames.find(gn->getInstanciatedObjectId());
      if (git != m_geometryNames.end()) {
	LOG_DEBUG("geometry check: could not find geometry " + git->second + ", a child of OID " + n->getOriginalId() + " name " + n->getName() + " in our geometry map");
      } else {
	LOG_DEBUG("geometry check: could not find geometry ID " + Ogre::StringConverter::toString(gn->getUniqueId()) + " of name " + gn->getName() + ", an instance of unique ID " + Ogre::StringConverter::toString(gn->getInstanciatedObjectId()) + " off node OID " + n->getOriginalId() + " name " + n->getName() + " in the geometry map");
      }
      continue;
    }
  }
  
  // check library instances hanging off this node
  const COLLADAFW::InstanceNodePointerArray& inodes = n->getInstanceNodes();
  for (int i = 0, count = inodes.getCount(); i < count; ++i) {
    const COLLADAFW::InstanceNode* in = inodes[i];
    LibNodesIterator it = m_libNodes.find(inodes[i]->getInstanciatedObjectId());
    if (it == m_libNodes.end()) {
      // this should not happen
      LOG_DEBUG("geometry check: node " + n->getOriginalId() + " refers to instantiated object " +
		Ogre::StringConverter::toString(in->getInstanciatedObjectId()) + " but I cannot find it in the library node directory");
    } else {
      // not a terminal node; proceed
      node_dfs_geocheck(it->second);
    }
  }

  // now process regular child nodes
  const COLLADAFW::NodePointerArray& cnodes = n->getChildNodes();
  for (int i = 0, count = cnodes.getCount(); i < count; ++i) {
    node_dfs_geocheck(cnodes[i]);
  }
}
