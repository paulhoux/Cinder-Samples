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

#include "cinder/Unicode.h"

#include "text/TextLabels.h"

#include <boost/algorithm/string.hpp>

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

void TextLabels::renderString( const std::wstring &str, Vec2f *cursor, float stretch )
{
	std::wstring::const_iterator itr;
	for(itr=str.begin();itr!=str.end();++itr) {
		// retrieve character code
		uint16_t id = (uint16_t) *itr;

		if( mFont->contains(id) ) {
			// get metrics for this character to speed up measurements
			Font::Metrics m = mFont->getMetrics(id);

			// skip whitespace characters
			if( ! isWhitespaceUtf16(id) ) {
				size_t index = mVertices.size();

				Rectf bounds = mFont->getBounds(m, mFontSize);
				mVertices.push_back( Vec3f(*cursor + bounds.getUpperLeft()) );
				mVertices.push_back( Vec3f(*cursor + bounds.getUpperRight()) );
				mVertices.push_back( Vec3f(*cursor + bounds.getLowerRight()) );
				mVertices.push_back( Vec3f(*cursor + bounds.getLowerLeft()) );
			
				bounds = mFont->getTexCoords(m);
				mTexcoords.push_back( bounds.getUpperLeft() );
				mTexcoords.push_back( bounds.getUpperRight() );
				mTexcoords.push_back( bounds.getLowerRight() );
				mTexcoords.push_back( bounds.getLowerLeft() );

				mIndices.push_back(index+0); mIndices.push_back(index+3); mIndices.push_back(index+1);
				mIndices.push_back(index+1); mIndices.push_back(index+3); mIndices.push_back(index+2);
			
				mOffsets.insert(mOffsets.end(), 4, mOffset);
			}

			if( id == 32 )
				cursor->x += stretch * mFont->getAdvance(m, mFontSize);
			else
				cursor->x += mFont->getAdvance(m, mFontSize);
		}
	}

	//
	mBoundsInvalid = true;
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
		"uniform vec2 viewport_size;\n"
		"\n"
		"vec3 toClipSpace(vec4 vertex)\n"
		"{\n"
		"	return vec3( vertex.xyz / vertex.w );\n"
		"}\n"
		"\n"
		"vec2 toScreenSpace(vec4 vertex)\n"
		"{\n"
		"	return vec2( vertex.xy / vertex.w ) * viewport_size;\n"
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
		"	//a *= clamp( pow(1.0 - position.w * 0.01, 3.0), 0.0, 1.0 );\n"
		"\n"
		"	gl_FrontColor = vec4( gl_Color.rgb, a );\n"
		"\n"
		"	// calculate vertex position by offsetting it\n"
		"	gl_Position = vec4( (gl_Vertex.xy * vec2(1.0, -1.0)) / viewport_size * 2.0 + offset.xy, 0.0, 1.0 );\n"
		"}";

	return std::string(vs);
}

bool TextLabels::bindShader()
{
	if( Text::bindShader() )
	{
		mShader.uniform( "viewport_size", Vec2f( app::getWindowSize() ) );
		return true;
	}

	return false;
}

} } // namespace ph::text