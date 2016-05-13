/*
 Copyright (c) 2010-2012, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
    the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
    the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "cinder/DataSource.h"
#include "cinder/TriMesh.h"
#include "cinder/Utilities.h"
#include "cinder/gl/Vbo.h"
#include "text/Text.h"

namespace ph {
namespace text {

class TextLabelCompare {
  public:
	TextLabelCompare(){};

	// always append at the end
	bool operator()( const ci::vec4 &a, const ci::vec4 &b ) const { return false; }
};

typedef std::multimap<ci::vec4, std::u16string, TextLabelCompare> TextLabelList;
typedef TextLabelList::iterator       TextLabelListIter;
typedef TextLabelList::const_iterator TextLabelListConstIter;

class TextLabels : public ph::text::Text {
  public:
	TextLabels( void )
	    : mOffset( 0 ){};
	virtual ~TextLabels( void ){};

	//! clears all labels
	void clear();
	//! returns the number of labels
	size_t size() const { return mLabels.size(); }
	//! returns a const iterator to the labels
	TextLabelListConstIter begin() const { return mLabels.begin(); }
	//! returns a const iterator to the labels
	TextLabelListConstIter end() const { return mLabels.end(); }

	//!
	// ci::vec2	getOffset() const { return mOffset; }
	// void		setOffset( float x, float y ) { setOffset( ci::vec2(x, y) ); }
	// void		setOffset( const ci::vec2 &offset ) { mOffset = offset; mInvalid = true; }

	//!	add label
	void addLabel( const ci::vec3 &position, const std::string &text, float data = 0.0f ) { addLabel( position, ci::toUtf16( text ), data ); }
	void addLabel( const ci::vec3 &position, const std::u16string &text, float data = 0.0f );

	//! override vertex shader
	virtual std::string getVertexShader() const;

  protected:
	//! get the maximum width of the text at the specified vertical position
	virtual float getWidthAt( float y ) const { return 1000.0f; }

	//! override bind method
	virtual bool bindShader();

	//! clears the mesh and the buffers
	virtual void clearMesh();
	//! renders the current contents of mText
	virtual void renderMesh();
	//! helper to render a non-word-wrapped string
	virtual void renderString( const std::u16string &str, ci::vec2 *cursor, float stretch = 1.0f );
	//! creates the VBO from the data in the buffers
	virtual void createMesh();

  private:
	TextLabelList mLabels;

	ci::vec4              mOffset;
	std::vector<ci::vec4> mOffsets;
};
}
} // namespace ph::text