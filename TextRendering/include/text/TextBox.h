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

#include "cinder/DataSource.h"
#include "cinder/TriMesh.h"
#include "cinder/Utilities.h"
#include "cinder/gl/Vbo.h"
#include "text/Text.h"

namespace ph { namespace text {

class TextBox
	: public ph::text::Text
{
public:
	TextBox(void)
		: mSize( ci::Vec2f::zero() ) {};
	TextBox(float width, float height) 
		: mSize( ci::Vec2f(width, height) ) {};
	TextBox(const ci::Vec2f &size) 
		: mSize(size) {};
	virtual ~TextBox(void) {};

	//!
	void		drawBounds( const ci::Vec2f &offset = ci::Vec2f::zero() );

	//!
	ci::Vec2f	getSize() const { return mSize; }
	//!
	void		setSize(float width, float height) { mSize = ci::Vec2f(width, height); mInvalid = true; }
	void		setSize(const ci::Vec2f &size) { mSize = size; mInvalid = true; }
protected:
	//! get the maximum width of the text at the specified vertical position
	virtual float getWidthAt(float y) { return mSize.x; }
	//! get the maximum height of the text
	virtual float	getHeight() { return mSize.y; }
	//! function to move the cursor to the next line
	virtual	bool	newLine( ci::Vec2f *cursor ) {
		cursor->x = 0.0f; 
		cursor->y += std::floorf(getLeading() + 0.5f); 

		float h = getHeight();
		return (h == 0.0f || cursor->y < h); 
	}
protected:
	ci::Vec2f		mSize;
};

} } // namespace ph::text