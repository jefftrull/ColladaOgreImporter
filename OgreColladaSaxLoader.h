// OgreColladaSaxLoader.h, a class definition for dispatching Collada elements as they are encountered
// This class extends COLLADASaxFWL::Loader to add a special handler for <extra> elements
// Author: Jeff Trull <jetrull@sbcglobal.net>

/*
Copyright (c) 2013 Jeffrey E. Trull

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "COLLADASaxFWLIExtraDataCallbackHandler.h"
#include "COLLADASaxFWLLoader.h"

namespace COLLADAFW {
   class Effect;
}
class OgreColladaWriter;

class OgreColladaSaxLoader : public COLLADASaxFWL::Loader {
  // private class implementing the callback handler interface
  class OgreColladaExtraDataHandler: public COLLADASaxFWL::IExtraDataCallbackHandler {
    typedef GeneratedSaxParser::ParserChar ParserChar;
    typedef GeneratedSaxParser::StringHash StringHash;
  public:
    OgreColladaExtraDataHandler();
    ~OgreColladaExtraDataHandler();
    virtual bool elementBegin( const ParserChar* elementName, const GeneratedSaxParser::xmlChar** attributes);
    virtual bool elementEnd(const ParserChar* elementName );
    virtual bool textData(const ParserChar* text, size_t textLength);

    /** Method to ask, if the current callback handler want to read the data of the given extra element. */
    virtual bool parseElement ( 
      const ParserChar* profileName, 
      const StringHash& elementHash, 
      const COLLADAFW::UniqueId& uniqueId,
      COLLADAFW::Object* object );

    void setWriter(OgreColladaWriter* writer);

  private:
    COLLADAFW::Effect* m_latestEffect;
    OgreColladaWriter* m_writer;
  };

  OgreColladaExtraDataHandler m_extraDataHandler;

public:
  OgreColladaSaxLoader();
  ~OgreColladaSaxLoader();
  // override loadDocument methods so we can access "extra" params from the writer
  virtual bool loadDocument(const COLLADAFW::String& fileName, COLLADAFW::IWriter* writer);
  virtual bool loadDocument(const COLLADAFW::String& uri, const char* buffer, int length,
                            COLLADAFW::IWriter* writer);
};
