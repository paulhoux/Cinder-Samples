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

#include "cinder/Rand.h"
#include "cinder/Unicode.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/draw.h"

#include "text/Text.h"

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

namespace ph {
namespace text {

using namespace ci;
using namespace std;

void Text::draw()
{
	if( mInvalid ) {
		clearMesh();
		renderMesh();
		createMesh();
	}

	if( mVboMesh && mFont && bindShader() ) {
		mFont->enableAndBind();
		gl::draw( mVboMesh );
		mFont->unbind();

		unbindShader();
	}
}

void Text::drawWireframe()
{
	if( mInvalid ) {
		clearMesh();
		renderMesh();
		createMesh();
	}

	if( !mVboMesh )
		return;

	gl::enableWireframe();
	gl::disable( GL_TEXTURE_2D );

	gl::draw( mVboMesh );
}

void Text::clearMesh()
{
	mVboMesh.reset();

	mVertices.clear();
	mIndices.clear();
	mTexcoords.clear();

	mInvalid = true;
}

void Text::renderMesh()
{
	// prevent errors
	if( !mInvalid )
		return;
	if( !mFont )
		return;
	if( mText.empty() )
		return;

	// initialize variables
	const float    space = mFont->getAdvance( 32, mFontSize );
	const float    height = getHeight() > 0.0f ? ( getHeight() - mFont->getDescent( mFontSize ) ) : 0.0f;
	float          width, linewidth;
	size_t         index = 0;
	std::u16string trimmed, chunk;

	// initialize cursor position
	vec2 cursor( 0.0f, std::floorf( mFont->getAscent( mFontSize ) + 0.5f ) );

	// get word/line break information from Cinder's Unicode class if not available
	if( mMust.empty() || mAllow.empty() )
		findBreaksUtf16( mText, &mMust, &mAllow );

	// double t = app::getElapsedSeconds();

	// reserve some room in the buffers, to prevent excessive resizing. Do not use the full string length,
	// because the text may contain white space characters that don't need to be rendered.
	size_t sz = mText.length() / 2;
	mVertices.reserve( 4 * sz );
	mTexcoords.reserve( 4 * sz );
	mIndices.reserve( 6 * sz );

	// process text in chunks
	std::vector<size_t>::iterator mitr = mMust.begin();
	std::vector<size_t>::iterator aitr = mAllow.begin();
	while( aitr != mAllow.end() && mitr != mMust.end() && ( height == 0.0f || cursor.y <= height ) ) {
		// calculate the maximum allowed width for this line
		linewidth = getWidthAt( cursor.y );

		switch( mBoundary ) {
		case LINE:
			// render the whole paragraph
			trimmed = boost::trim_copy( mText.substr( index, *mitr - index + 1 ) );
			width = mFont->measureWidth( trimmed, mFontSize, true );

			// advance iterator
			index = *mitr;
			++mitr;

			break;
		case WORD:
			// measure the first chunk on this line
			chunk = ( mText.substr( index, *aitr - index + 1 ) );
			width = mFont->measureWidth( chunk, mFontSize, false );

			// if it fits, add the next chunk until no more chunks fit or are available
			while( linewidth > 0.0f && width < linewidth && *aitr != *mitr ) {
				++aitr;

				if( aitr == mAllow.end() )
					break;

				chunk = ( mText.substr( *( aitr - 1 ) + 1, *aitr - *( aitr - 1 ) ) );
				width += mFont->measureWidth( chunk, mFontSize, false );
			}

			// end of line encountered
			if( aitr == mAllow.begin() || *( aitr - 1 ) <= index ) { // not a single chunk fits on this line, just render what we have
			}
			else if( linewidth > 0.0f && width > linewidth ) { // remove the last chunk
				--aitr;
			}

			if( aitr != mAllow.end() ) {
				//
				trimmed = boost::trim_copy( mText.substr( index, *aitr - index + 1 ) );
				width = mFont->measureWidth( trimmed, mFontSize );

				// end of paragraph encountered, move to next
				if( *aitr == *mitr )
					++mitr;
				/*else if( mAlignment == JUSTIFIED )
				{
				// count spaces
				uint32_t c = std::count( trimmed.begin(), trimmed.end(), 32 );
				if( c == 0 ) break;
				// remaining whitespace
				float remaining = getWidthAt( cursor.y ) - width;
				float space = mFont->getAdvance( 32, mFontSize );
				//
				stretch = (remaining / c + space) / space;
				if( stretch > 3.0f ) stretch = 1.0f;
				}*/

				// advance iterator
				index = *aitr;
				++aitr;
			}

			break;
		}

		// adjust alignment
		switch( mAlignment ) {
		case CENTER:
			cursor.x = 0.5f * ( linewidth - width );
			break;
		case RIGHT:
			cursor.x = ( linewidth - width );
			break;
			break;
		}

		// add this fitting part of the text to the mesh
		renderString( trimmed, &cursor );

		// advance cursor to new line
		if( !newLine( &cursor ) )
			break;
	}

	// app::console() << ( app::getElapsedSeconds() - t ) << std::endl;
}

void Text::renderString( const std::u16string &str, vec2 *cursor, float stretch )
{
	std::u16string::const_iterator itr;
	for( itr = str.begin(); itr != str.end(); ++itr ) {
		// retrieve character code
		uint16_t id = (uint16_t)*itr;

		if( mFont->contains( id ) ) {
			// get metrics for this character to speed up measurements
			Font::Metrics m = mFont->getMetrics( id );

			// skip whitespace characters
			if( !isWhitespaceUtf16( id ) ) {
				size_t index = mVertices.size();

				Rectf bounds = mFont->getBounds( m, mFontSize );
				mVertices.push_back( vec3( *cursor + bounds.getUpperLeft(), 0 ) );
				mVertices.push_back( vec3( *cursor + bounds.getUpperRight(), 0 ) );
				mVertices.push_back( vec3( *cursor + bounds.getLowerRight(), 0 ) );
				mVertices.push_back( vec3( *cursor + bounds.getLowerLeft(), 0 ) );

				bounds = mFont->getTexCoords( m );
				mTexcoords.push_back( bounds.getUpperLeft() );
				mTexcoords.push_back( bounds.getUpperRight() );
				mTexcoords.push_back( bounds.getLowerRight() );
				mTexcoords.push_back( bounds.getLowerLeft() );

				mIndices.push_back( index + 0 );
				mIndices.push_back( index + 3 );
				mIndices.push_back( index + 1 );
				mIndices.push_back( index + 1 );
				mIndices.push_back( index + 3 );
				mIndices.push_back( index + 2 );
			}

			if( id == 32 )
				cursor->x += stretch * mFont->getAdvance( m, mFontSize );
			else
				cursor->x += mFont->getAdvance( m, mFontSize );
		}
	}

	//
	mBoundsInvalid = true;
}

void Text::createMesh()
{
	//
	if( mVertices.empty() || mIndices.empty() )
		return;

	//
	gl::VboMesh::Layout layout;
	layout.attrib( geom::POSITION, 3 );
	layout.attrib( geom::TEX_COORD_0, 2 );

	mVboMesh = gl::VboMesh::create( mVertices.size(), GL_TRIANGLES, { layout }, mIndices.size(), GL_UNSIGNED_SHORT );
	mVboMesh->bufferAttrib( geom::POSITION, mVertices.size() * sizeof( vec3 ), mVertices.data() );
	mVboMesh->bufferAttrib( geom::TEX_COORD_0, mTexcoords.size() * sizeof( vec2 ), mTexcoords.data() );
	mVboMesh->bufferIndices( mIndices.size() * sizeof( uint16_t ), mIndices.data() );

	mInvalid = false;
}

Rectf Text::getBounds() const
{
	if( mBoundsInvalid ) {
		mBounds = Rectf( 0.0f, 0.0f, 0.0f, 0.0f );

		vector<vec3>::const_iterator itr = mVertices.begin();
		while( itr != mVertices.end() ) {
			mBounds.x1 = ci::math<float>::min( itr->x, mBounds.x1 );
			mBounds.y1 = ci::math<float>::min( itr->y, mBounds.y1 );
			mBounds.x2 = ci::math<float>::max( itr->x, mBounds.x2 );
			mBounds.y2 = ci::math<float>::max( itr->y, mBounds.y2 );
			++itr;
		}

		mBoundsInvalid = false;
	}

	return mBounds;
}

std::string Text::getVertexShader() const
{
	// vertex shader
	const char *vs
	    = "#version 150\n"
	      ""
	      "uniform mat4 ciModelViewProjection;\n"
	      ""
	      "in vec4 ciPosition;\n"
	      "in vec2 ciTexCoord0;\n"
	      "in vec4 ciColor;\n"
	      ""
	      "out vec2 vTexCoord0;\n"
	      "out vec4 vColor;\n"
	      ""
	      "void main()\n"
	      "{\n"
	      "	vColor = ciColor;\n"
	      "	vTexCoord0 = ciTexCoord0;\n"
	      ""
	      "	gl_Position = ciModelViewProjection * ciPosition;\n"
	      "}\n";

	return std::string( vs );
}

std::string Text::getFragmentShader() const
{
	// fragment shader
	const char *fs
	    = "#version 150\n"
	      ""
	      "uniform sampler2D	font_map;\n"
	      "uniform float      smoothness;\n"
	      ""
	      "const float kGamma = 2.2;\n"
	      ""
	      "in vec2 vTexCoord0;\n"
	      "in vec4 vColor;\n"
	      ""
	      "out vec4 oColor;\n"
	      ""
	      "void main()\n"
	      "{\n"
	      "	// retrieve signed distance\n"
	      "	float sdf = texture( font_map, vTexCoord0.xy ).r;\n"
	      "\n"
	      "	// perform adaptive anti-aliasing of the edges\n"
	      "	float w = clamp( smoothness * (abs(dFdx(vTexCoord0.x)) + abs(dFdy(vTexCoord0.y))), 0.0, 0.5);\n"
	      "	float a = smoothstep(0.5-w, 0.5+w, sdf);\n"
	      "\n"
	      "	// gamma correction for linear attenuation\n"
	      "	a = pow(a, 1.0/kGamma);\n"
	      "\n"
	      "	// final color\n"
	      "	oColor.rgb = vColor.rgb;\n"
	      "	oColor.a = vColor.a * a;\n"
	      "}\n";

	return std::string( fs );
}

bool Text::bindShader()
{
	if( !mShader ) {
		try {
			mShader = gl::GlslProg::create( getVertexShader().c_str(), getFragmentShader().c_str() );
		}
		catch( const std::exception &e ) {
			app::console() << "Could not load&compile shader: " << e.what() << std::endl;
			mShader = gl::GlslProgRef();
			return false;
		}
	}

	mShader->bind();
	mShader->uniform( "font_map", 0 );
	mShader->uniform( "smoothness", 64.0f );

	return true;
}

bool Text::unbindShader()
{
	// if( mShader )
	//	mShader.unbind();

	return true;
}

void Text::findBreaksUtf8( const std::string &line, std::vector<size_t> *must, std::vector<size_t> *allow )
{
	std::vector<uint8_t> resultBreaks;
	calcLinebreaksUtf8( line.c_str(), &resultBreaks );

	//
	must->clear();
	allow->clear();

	//
	for( size_t i = 0; i < resultBreaks.size(); ++i ) {
		if( resultBreaks[i] == ci::UNICODE_ALLOW_BREAK )
			allow->push_back( i );
		else if( resultBreaks[i] == ci::UNICODE_MUST_BREAK ) {
			must->push_back( i );
			allow->push_back( i );
		}
	}
}

void Text::findBreaksUtf16( const std::u16string &line, std::vector<size_t> *must, std::vector<size_t> *allow )
{
	std::vector<uint8_t> resultBreaks;
	calcLinebreaksUtf16( (uint16_t *)line.c_str(), &resultBreaks );

	//
	must->clear();
	allow->clear();

	//
	for( size_t i = 0; i < resultBreaks.size(); ++i ) {
		if( resultBreaks[i] == ci::UNICODE_ALLOW_BREAK )
			allow->push_back( i );
		else if( resultBreaks[i] == ci::UNICODE_MUST_BREAK ) {
			must->push_back( i );
			allow->push_back( i );
		}
	}
}

bool Text::isWhitespaceUtf8( const char ch )
{
	return isWhitespaceUtf16( (short)ch );
}

bool Text::isWhitespaceUtf16( const wchar_t ch )
{
	// see: http://en.wikipedia.org/wiki/Whitespace_character,
	// make sure the values are in ascending order,
	// otherwise the binary search won't work
	static const wchar_t arr[]
	    = { 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x0020, 0x0085, 0x00A0, 0x1680, 0x180E, 0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200A, 0x2028, 0x2029, 0x202F, 0x205F, 0x3000 };
	static const vector<wchar_t> whitespace( arr, arr + sizeof( arr ) / sizeof( arr[0] ) );

	return std::binary_search( whitespace.begin(), whitespace.end(), ch );
}
}
} // namespace ph::text