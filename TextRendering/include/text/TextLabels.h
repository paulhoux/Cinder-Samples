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

class TextLabelCompare
{
public:
	TextLabelCompare() {};

	// always append at the end
	bool operator() (const ci::Vec3f &a, const ci::Vec3f &b) const {
        return false;
    }
};

typedef std::multimap< ci::Vec3f, std::wstring, TextLabelCompare >	TextLabelList;
typedef TextLabelList::iterator			TextLabelListIter;
typedef TextLabelList::const_iterator	TextLabelListConstIter;
	
class TextLabels
	: public ph::text::Text
{
public:
	TextLabels(void)
		: mOffset( ci::Vec2f::zero() ) {};
	virtual ~TextLabels(void) {};

	//! clears all labels
	void	clear();
	//! returns the number of labels 
	size_t	size() const { return mLabels.size(); }
	//! returns a const iterator to the labels
	TextLabelListConstIter	begin() const { return mLabels.begin(); }
	//! returns a const iterator to the labels
	TextLabelListConstIter	end() const { return mLabels.end(); }

	//!
	ci::Vec2f	getOffset() const { return mOffset; }
	void		setOffset( float x, float y ) { setOffset( ci::Vec2f(x, y) ); }
	void		setOffset( const ci::Vec2f &offset ) { mOffset = offset; mInvalid = true; }

	//!	add label
	void	addLabel( const ci::Vec3f &position, const std::string &text ) { addLabel(position, ci::toUtf16(text)); }
	void	addLabel( const ci::Vec3f &position, const std::wstring &text );

	//! override the render() method
	void	render();
protected:
	//! get the maximum width of the text at the specified vertical position
	virtual float getWidthAt(float y) const { return 1000.0f; }
	
	//! override vertex shader and bind method
	virtual std::string	getVertexShader() const;
	virtual bool		bindShader();
private:
	ci::Vec2f		mOffset;
	TextLabelList	mLabels;
};

} } // namespace ph::text