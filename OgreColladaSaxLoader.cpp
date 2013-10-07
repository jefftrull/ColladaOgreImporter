// Implementation of Collada element dispatcher
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2013 Jeffrey E. Trull

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <vector>

#include <COLLADAFWEffect.h>

#include "OgreColladaSaxLoader.h"
#include "OgreColladaWriter.h"

OgreCollada::SaxLoader::ExtraDataHandler::ExtraDataHandler()
  : COLLADASaxFWL::IExtraDataCallbackHandler(), m_latestEffect(0) {}

OgreCollada::SaxLoader::ExtraDataHandler::~ExtraDataHandler() {}

bool OgreCollada::SaxLoader::ExtraDataHandler::elementBegin(
  const ParserChar* elementName, const GeneratedSaxParser::xmlChar** attributes) {
  
  return (std::string(elementName) == "double_sided");  // signal our interest
}

bool OgreCollada::SaxLoader::ExtraDataHandler::elementEnd(const ParserChar* elementName ) {
  return true;
}

bool OgreCollada::SaxLoader::ExtraDataHandler::textData(const ParserChar* text, size_t textLength) {
   if (std::string(text, textLength) == "1") {
     // a single value of 1 indicates we should disable backside culling (so the material is visible from both sides)
     m_writer->disableCulling(m_latestEffect->getUniqueId());
   }
   return true;
}
bool OgreCollada::SaxLoader::ExtraDataHandler::parseElement (
  const ParserChar* profileName, const StringHash& elementHash,
  const COLLADAFW::UniqueId& uniqueId, COLLADAFW::Object* object ) {

  if (std::string(profileName) == "GOOGLEEARTH") {
    m_latestEffect = dynamic_cast<COLLADAFW::Effect*>(object);
    return true;
  }
  return false;   // <extra> tag pertains to some other exporter
}

void OgreCollada::SaxLoader::ExtraDataHandler::setWriter(OgreCollada::Writer* w) {
  m_writer = w;
}

OgreCollada::SaxLoader::SaxLoader()
  : COLLADASaxFWL::Loader() {
  // add our private callback for <extra> tags
  registerExtraDataCallbackHandler(&m_extraDataHandler);
}

OgreCollada::SaxLoader::~SaxLoader() {}

bool OgreCollada::SaxLoader::loadDocument(const COLLADAFW::String& fileName,
                                        COLLADAFW::IWriter* writer) {
  // give the <extra> handler access to the writer to communicate special
  // information (initially, which materials should be double-sided)
  m_extraDataHandler.setWriter(dynamic_cast<Writer*>(writer));
  return COLLADASaxFWL::Loader::loadDocument(fileName, writer);
}

bool OgreCollada::SaxLoader::loadDocument(const COLLADAFW::String& uri,
                                        const char* buffer, int length,
                                        COLLADAFW::IWriter* writer) {
  m_extraDataHandler.setWriter(dynamic_cast<Writer*>(writer));
  return COLLADASaxFWL::Loader::loadDocument(uri, buffer, length, writer);
}
