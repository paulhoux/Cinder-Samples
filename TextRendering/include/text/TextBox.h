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

class TextBox : public ph::text::Text {
  public:
	TextBox( void )
	    : mSize( ci::vec2( 0 ) ){};
	TextBox( float width, float height )
	    : mSize( ci::vec2( width, height ) ){};
	TextBox( const ci::vec2 &size )
	    : mSize( size ){};
	virtual ~TextBox( void ){};

	//!
	void drawBounds( const ci::vec2 &offset = ci::vec2( 0 ) );

	//!
	ci::vec2 getSize() const { return mSize; }
	//!
	void setSize( float width, float height )
	{
		mSize = ci::vec2( width, height );
		mInvalid = true;
		mBoundsInvalid = true;
	}
	void setSize( const ci::vec2 &size )
	{
		mSize = size;
		mInvalid = true;
		mBoundsInvalid = true;
	}

  protected:
	//! get the maximum width of the text at the specified vertical position
	virtual float getWidthAt( float y ) { return mSize.x; }
	//! get the maximum height of the text
	virtual float getHeight() { return mSize.y; }
	//! function to move the cursor to the next line
	virtual bool newLine( ci::vec2 *cursor )
	{
		cursor->x = 0.0f;
		cursor->y += std::floorf( getLeading() + 0.5f );

		float h = getHeight();
		return ( h == 0.0f || cursor->y < h );
	}

  protected:
	ci::vec2 mSize;
};
}
} // namespace ph::text