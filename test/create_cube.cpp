// Generate triangulated cube test data for importer testing
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2012 Jeffrey E Trull

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <vector>
#include <iostream>

#include <COLLADASWConstants.h>
#include <COLLADASWAsset.h>
#include <COLLADASWNode.h>
#include <COLLADASWPrimitves.h>
#include <COLLADASWBaseInputElement.h>
#include <COLLADASWInstanceGeometry.h>
#include <COLLADASWLibraryGeometries.h>
#include <COLLADASWLibraryEffects.h>
#include <COLLADASWLibraryMaterials.h>
#include <COLLADASWLibraryVisualScenes.h>
#include <COLLADASWLibrary.h>
#include <COLLADASWStreamWriter.h>
#include <COLLADASWSource.h>
#include <COLLADASWScene.h>
#include <COLLADABUStringUtils.h>

struct CubeExporter : public COLLADASW::LibraryGeometries {
  // use parent ctor/dtor
  CubeExporter(COLLADASW::StreamWriter* sw) : COLLADASW::LibraryGeometries(sw) {}

  void exportCube() {
    openMesh("CubeID", "Cube");

    COLLADASW::FloatSource vertexSource(mSW);   // inherited from COLLADASW::ElementWriter
    vertexSource.setId("Positions");
    vertexSource.setArrayId("positions-list");
    vertexSource.setAccessorStride(3);
    vertexSource.setAccessorCount(8);
    vertexSource.getParameterNameList().push_back("X");
    vertexSource.getParameterNameList().push_back("Y");
    vertexSource.getParameterNameList().push_back("Z");
    vertexSource.prepareToAppendValues();
    vertexSource.appendValues(0.0,   0.0,   0.0);
    vertexSource.appendValues(0.0,   100.0, 0.0);
    vertexSource.appendValues(100.0, 0.0,   0.0);
    vertexSource.appendValues(100.0, 100.0, 0.0);
    vertexSource.appendValues(0.0,   0.0,   100.0);
    vertexSource.appendValues(0.0,   100.0, 100.0);
    vertexSource.appendValues(100.0, 0.0,   100.0);
    vertexSource.appendValues(100.0, 100.0, 100.0);
    vertexSource.finish();

    // vertices, which just seems to be some sort of indirection
    COLLADASW::VerticesElement vertices(mSW);
    vertices.setId("vertices");
    COLLADASW::InputList& inputList = vertices.getInputList();
    inputList.push_back(COLLADASW::Input(COLLADASW::InputSemantic::POSITION,
					 COLLADASW::URI(EMPTY_STRING,
							COLLADABU::NativeString("Positions"))));
    vertices.add();
    
    // the actual faces themselves
    COLLADASW::Triangles tris(mSW);
    tris.setMaterial(COLLADABU::NativeString("CubeColor"));
    tris.setCount(6*2);         // two triangles per side

    tris.getInputList().push_back(COLLADASW::Input(COLLADASW::InputSemantic::VERTEX,
						   COLLADASW::URI(EMPTY_STRING,
								  COLLADABU::NativeString("vertices")),
						   0));  // or you won't get an "offset" attribute
    tris.prepareToAppendValues();
    // actual index data here:
    // back face
    tris.appendValues(0, 1, 2);
    tris.appendValues(1, 3, 2);
    // front face
    tris.appendValues(4, 6, 5);
    tris.appendValues(5, 6, 7);
    // left face
    tris.appendValues(0, 4, 1);
    tris.appendValues(5, 1, 4);
    // right face
    tris.appendValues(2, 3, 6);
    tris.appendValues(7, 6, 3);
    // bottom face
    tris.appendValues(0, 2, 4);
    tris.appendValues(2, 6, 4);
    // top face
    tris.appendValues(1, 5, 3);
    tris.appendValues(3, 5, 7);

    tris.closeElement();

    tris.finish();

    closeMesh();

    closeLibrary();   // closes "library_geometries" tag

  }


};

struct MaterialExporter : public COLLADASW::LibraryMaterials {
  MaterialExporter(COLLADASW::StreamWriter* sw) : COLLADASW::LibraryMaterials(sw) {}

  void exportMaterial() {
    openMaterial(COLLADABU::NativeString("LandlordWhite"));

    // just refer to the effect
    addInstanceEffect(COLLADASW::URI(EMPTY_STRING, COLLADABU::NativeString("LLWEffect")));

    closeMaterial();
    closeLibrary();
  }

};

struct EffectExporter : public COLLADASW::LibraryEffects {
  EffectExporter(COLLADASW::StreamWriter* sw) : COLLADASW::LibraryEffects(sw) {}

  void exportEffect() {
    openEffect(COLLADABU::NativeString("LLWEffect"));
    COLLADASW::EffectProfile ep(mSW);
    ep.setShaderType(COLLADASW::EffectProfile::PHONG);
    // "Landlord White" or beige - everyone's favorite color.  Values from Wikipedia.
    ep.setDiffuse(COLLADASW::ColorOrTexture(COLLADASW::Color(245.0/256.0, 245.0/256.0, 220.0/256.0)));
    ep.openProfile();
    ep.addProfileElements();
    ep.closeProfile();
    closeEffect();
    closeLibrary();
  }
};

struct VisualSceneExporter : public COLLADASW::LibraryVisualScenes {
  VisualSceneExporter(COLLADASW::StreamWriter* sw) : COLLADASW::LibraryVisualScenes(sw) {}

  void exportVisualScene() {
    openVisualScene(COLLADABU::NativeString("VisualScene"));

    COLLADASW::Node csn(mSW);
    csn.setNodeId(COLLADABU::NativeString("Cube"));
    csn.setType(COLLADASW::Node::NODE);
    csn.start();

    COLLADASW::InstanceGeometry ig(mSW);
    ig.setUrl(COLLADASW::URI(EMPTY_STRING, COLLADABU::NativeString("CubeID")));
    COLLADASW::InstanceMaterial im(COLLADABU::NativeString("CubeColor"),
				   COLLADASW::URI(EMPTY_STRING,
						  COLLADABU::NativeString("LandlordWhite")));
    ig.getBindMaterial().getInstanceMaterialList().push_back(im);
    ig.add();

    closeVisualScene();

    closeLibrary();
  }
};

int main(int argc, char **argv) {
  // get name of output from argv
  if (argc != 2) {
    std::cerr << "usage: create_cube /path/to/cube.dae" << std::endl;
    return 1;
  }

  // open output file
  COLLADASW::StreamWriter sw((COLLADABU::NativeString(argv[1])));

  // stream data
  sw.startDocument();

  COLLADASW::Asset a(&sw);
  a.setUpAxisType(COLLADASW::Asset::Y_UP);
  a.setUnit("centimeter", 0.01);
  a.add();

  // material for the cube
  EffectExporter ee(&sw);
  ee.exportEffect();
  MaterialExporter me(&sw);
  me.exportMaterial();
  
  // geometry of the cube
  CubeExporter ce(&sw);
  ce.exportCube();

  // now add a visual scene with a node that refers to the cube
  VisualSceneExporter vse(&sw);
  vse.exportVisualScene();

  // and make the primary scene refer to our visual scene
  COLLADASW::Scene scene(&sw, COLLADASW::URI(COLLADASW::CSWC::EMPTY_STRING,
					     COLLADABU::NativeString("VisualScene")));
  scene.add();

  // and we're done
  sw.endDocument();

  // close file
  // (sw dtor does this)

  return 0;
}
