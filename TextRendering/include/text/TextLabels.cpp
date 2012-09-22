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

#include "text/TextLabels.h"

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

namespace ph { namespace text {

using namespace ci;
using namespace std;

void TextLabels::clear()
{
	mLabels.clear();
	mInvalid = true;
}

void TextLabels::addLabel( const Vec3f &position, const std::wstring &text )
{
	mLabels.insert( pair<Vec3f, std::wstring>( position, text ) );
	mInvalid = true;
}

void TextLabels::clearMesh()
{
	mVboMesh.reset();
	
	mVertices.clear();
	mIndices.clear();
	mTexcoords.clear();
	mOffsets.clear();

	mInvalid = true;
}

void TextLabels::renderMesh()
{
	// parse all labels
	TextLabelListIter labelItr;
	for(labelItr=mLabels.begin();labelItr!=mLabels.end();++labelItr) {
		// render label
		mOffset = labelItr->first;
		setText( labelItr->second );

		Text::renderMesh();
	}
}

void TextLabels::renderString( const std::wstring &str, Vec2f *cursor )
{
	std::wstring::const_iterator itr;
	for(itr=str.begin();itr!=str.end();++itr) {
		// retrieve character code
		uint16_t id = (uint16_t) *itr;

		if( mFont->contains(id) ) {
			size_t index = mVertices.size();

			Rectf bounds = mFont->getBounds(id, mFontSize);
			mVertices.push_back( Vec3f(*cursor + bounds.getUpperLeft()) );
			mVertices.push_back( Vec3f(*cursor + bounds.getUpperRight()) );
			mVertices.push_back( Vec3f(*cursor + bounds.getLowerRight()) );
			mVertices.push_back( Vec3f(*cursor + bounds.getLowerLeft()) );
			
			bounds = mFont->getTexCoords(id);
			mTexcoords.push_back( bounds.getUpperLeft() );
			mTexcoords.push_back( bounds.getUpperRight() );
			mTexcoords.push_back( bounds.getLowerRight() );
			mTexcoords.push_back( bounds.getLowerLeft() );

			mIndices.push_back(index+0); mIndices.push_back(index+3); mIndices.push_back(index+1);
			mIndices.push_back(index+1); mIndices.push_back(index+3); mIndices.push_back(index+2);
			
			mOffsets.insert(mOffsets.end(), 4, mOffset);

			cursor->x += mFont->getAdvance(id, mFontSize);
		}
	}
}

void TextLabels::createMesh()
{
	//
	if( mVertices.empty() || mIndices.empty() )
		return;

	//
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticIndices();
	layout.setStaticTexCoords2d(0);
	//layout.setStaticColorsRGBA();
	layout.setStaticTexCoords3d(1);

	mVboMesh = gl::VboMesh( mVertices.size(), mIndices.size(), layout, GL_TRIANGLES );
	mVboMesh.bufferPositions( &mVertices.front(), mVertices.size() );
	mVboMesh.bufferIndices( mIndices );
	mVboMesh.bufferTexCoords2d( 0, mTexcoords );
	//mVboMesh.bufferColorsRGBA( colors );
	mVboMesh.bufferTexCoords3d( 1, mOffsets );

	mInvalid = false;
}

std::string TextLabels::getVertexShader() const
{
	// vertex shader
	const char *vs = 
		"#version 120\n"
		"\n"
		"uniform vec2 viewportSize;\n"
		"\n"
		"vec3 toClipSpace(vec4 vertex)\n"
		"{\n"
		"	return vec3( vertex.xyz / vertex.w );\n"
		"}\n"
		"\n"
		"vec2 toScreenSpace(vec4 vertex)\n"
		"{\n"
		"	return vec2( vertex.xy / vertex.w ) * viewportSize;\n"
		"}\n"
		"\n"
		"void main()\n"
		"{\n"
		"	// pass font texture coordinate to fragment shader\n"
		"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
		"\n"
		"	// retrieve label position\n"
		"	vec4 position = ( gl_ModelViewProjectionMatrix * gl_MultiTexCoord1 );\n"
		"\n"
		"	// convert position to clip space to find the 2D offset\n"
		"	vec3 offset = toClipSpace( position );\n"
		"\n"
		"	// if the label is behind us, then don't draw the label (set alpha to zero)\n"
		"	float a = (position.w < 0) ? 0.0 : gl_Color.a;\n"
		"	// only draw labels close to the camera\n"
		"	//a *= clamp( 1.0 - position.z * 0.02, 0.0, 1.0 );\n"
		"\n"
		"	gl_FrontColor = vec4( gl_Color.rgb, a );\n"
		"\n"
		"	// calculate vertex position by offsetting it\n"
		"	gl_Position = vec4( (gl_Vertex.xy * vec2(1.0, -1.0)) / viewportSize * 2.0 + offset.xy, 0.0, 1.0 );\n"
		"}";

	return std::string(vs);
}

bool TextLabels::bindShader()
{
	if( Text::bindShader() )
	{
		mShader.uniform( "viewportSize", Vec2f( app::getWindowSize() ) );
		return true;
	}

	return false;
}

} } // namespace ph::text