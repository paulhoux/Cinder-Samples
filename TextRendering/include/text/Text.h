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

#include "cinder/Cinder.h"
#include "cinder/Utilities.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/VboMesh.h"
#include "text/Font.h"

namespace ph {
namespace text {

class Text {
  public:
	//!
	typedef enum { LEFT, CENTER, RIGHT } Alignment;
	//! Specifies the boundary used to break up text in word wrapping and other algorithms
	typedef enum { LINE, WORD } Boundary;

  public:
	Text( void )
	    : mInvalid( true )
	    , mBoundsInvalid( true )
	    , mAlignment( LEFT )
	    , mBoundary( WORD )
	    , mFontSize( 14.0f )
	    , mLineSpace( 1.0f ){};
	virtual ~Text( void ){};

	virtual void draw();
	virtual void drawWireframe();

	std::string getFontFamily() const
	{
		if( mFont )
			return mFont->getFamily();
		else
			return std::string();
	}
	void setFont( FontRef font )
	{
		mFont = font;
		mInvalid = true;
	}

	float getFontSize() const { return mFontSize; }
	void setFontSize( float size )
	{
		mFontSize = size;
		mInvalid = true;
	}

	float getLineSpace() const { return mLineSpace; }
	void setLineSpace( float value )
	{
		mLineSpace = value;
		mInvalid = true;
	}

	float getLeading() const { return ( mFont ? std::floorf( mFont->getLeading( mFontSize ) * mLineSpace + 0.5f ) : 0.0f ); }

	Alignment getAlignment() const { return mAlignment; }
	void setAlignment( Alignment alignment )
	{
		mAlignment = alignment;
		mInvalid = true;
	}

	Boundary getBoundary() const { return mBoundary; }
	void setBoundary( Boundary boundary )
	{
		mBoundary = boundary;
		mInvalid = true;
	}

	void setText( const std::string &text ) { setText( ci::toUtf16( text ) ); }
	void setText( const std::u16string &text )
	{
		mText = text;
		mMust.clear();
		mAllow.clear();
		mInvalid = true;
	}

	ci::Rectf getBounds() const;

	//!
	virtual std::string getVertexShader() const;
	//!
	virtual std::string getFragmentShader() const;
	//! Allows you to override the default text shader.
	void setShader( const ci::gl::GlslProgRef &shader ) { mShader = shader; }
  protected:
	//! get the maximum width of the text at the specified vertical position
	virtual float getWidthAt( float y ) { return 0.0f; }
	//! get the maximum height of the text
	virtual float getHeight() { return 0.0f; }
	//! function to move the cursor to the next line
	virtual bool newLine( ci::vec2 *cursor )
	{
		cursor->x = 0.0f;
		cursor->y += getLeading();

		return ( getHeight() == 0.0f || cursor->y < getHeight() );
	}

	//!
	virtual bool bindShader();
	virtual bool unbindShader();

	//! clears the mesh and the buffers
	virtual void clearMesh();
	//! renders the current contents of mText
	virtual void renderMesh();
	//! helper to render a non-word-wrapped string
	virtual void renderString( const std::u16string &str, ci::vec2 *cursor, float stretch = 1.0f );
	//! creates the VBO from the data in the buffers
	virtual void createMesh();

  public:
	// special Unicode functions (requires Cinder v0.8.5)
	void findBreaksUtf8( const std::string &line, std::vector<size_t> *must, std::vector<size_t> *allow );
	void findBreaksUtf16( const std::u16string &line, std::vector<size_t> *must, std::vector<size_t> *allow );
	bool isWhitespaceUtf8( const char ch );
	bool isWhitespaceUtf16( const wchar_t ch );

  protected:
	bool mInvalid;

	mutable bool      mBoundsInvalid;
	mutable ci::Rectf mBounds;

	Alignment mAlignment;
	Boundary  mBoundary;

	std::u16string mText;

	ci::gl::GlslProgRef mShader;
	ci::gl::VboMeshRef  mVboMesh;

	FontRef mFont;
	float   mFontSize;

	float mLineSpace;

	std::vector<size_t>   mMust, mAllow;
	std::vector<ci::vec3> mVertices;
	std::vector<uint16_t> mIndices;
	std::vector<ci::vec2> mTexcoords;
};
}
} // namespace ph::text