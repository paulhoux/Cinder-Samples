/*
Copyright (C) 2011-2012 Paul Houx

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "cinder/Cinder.h"
#include "cinder/Utilities.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Vbo.h"
#include "text/Font.h"

namespace ph { namespace text {

class Text
{
public:
	//!
	typedef enum { LEFT, CENTER, RIGHT } Alignment;
	//! Specifies the boundary used to break up text in word wrapping and other algorithms
	typedef enum { NONE, LINE, WORD } Boundary;
public:
	Text(void) : mInvalid(true), mBoundsInvalid(true),
		mAlignment(LEFT), mBoundary(WORD), 
		mFontSize(14.0f), mLineSpace(1.0f) {};
	virtual ~Text(void) {};

	virtual void draw();
	virtual void drawWireframe();

	std::string	getFontFamily() const { if(mFont) return mFont->getFamily(); else return std::string(); }
	void		setFont( FontRef font ) { mFont = font; mInvalid = true; }

	float		getFontSize() const { return mFontSize; }
	void		setFontSize( float size ) { mFontSize = size; mInvalid = true; }

	float		getLineSpace() const { return mLineSpace; }
	void		setLineSpace( float value ) { mLineSpace = value; mInvalid = true; }

	float		getLeading() const { return (mFont ? mFont->getLeading(mFontSize) * mLineSpace : 0.0f); }

	Alignment	getAlignment() const { return mAlignment; }
	void		setAlignment( Alignment alignment ) { mAlignment = alignment; mInvalid = true; }

	Boundary	getBoundary() const { return mBoundary; }
	void		setBoundary( Boundary boundary ) { mBoundary = boundary; mInvalid = true; }

	void		setText(const std::string &text) { setText( ci::toUtf16(text) ); }
	void		setText(const std::wstring &text) { mText = text; mMust.clear(); mAllow.clear(); mInvalid = true; }

	ci::Rectf	getBounds() const;		
protected:
	//! get the maximum width of the text at the specified vertical position 
	virtual float	getWidthAt(float y) { return 0.0f; }
	//! get the maximum height of the text
	virtual float	getHeight() { return 0.0f; }
	//! function to move the cursor to the next line
	virtual	bool	newLine( ci::Vec2f *cursor ) { 
		cursor->x = 0.0f; 
		cursor->y += std::floorf(getLeading() + 0.5f); 
		
		return ( getHeight() == 0.0f || cursor->y < getHeight() );
	}		

	//!
	virtual std::string	getVertexShader() const;
	virtual std::string getFragmentShader() const;
	virtual bool		bindShader();
	virtual bool		unbindShader();
	
	//! clears the mesh and the buffers
	virtual void		clearMesh();
	//! renders the current contents of mText
	virtual void		renderMesh();
	//! helper to render a non-word-wrapped string
	virtual void		renderString( const std::wstring &str, ci::Vec2f *cursor );
	//! creates the VBO from the data in the buffers
	virtual void		createMesh();
protected:
	bool					mInvalid;
	mutable bool			mBoundsInvalid;

	ci::Rectf				mBounds;

	Alignment				mAlignment;
	Boundary				mBoundary;

	std::wstring			mText;

	ci::gl::GlslProg		mShader;
	ci::gl::VboMesh			mVboMesh;

	FontRef					mFont;
	float					mFontSize;

	float					mLineSpace;

	std::vector<size_t>		mMust, mAllow;
	std::vector<ci::Vec3f>	mVertices;
	std::vector<uint32_t>	mIndices;
	std::vector<ci::Vec2f>	mTexcoords;
};

} } // namespace ph::text