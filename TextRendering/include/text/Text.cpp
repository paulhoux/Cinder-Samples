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
#if defined( CINDER_MSW )
#include "cinder/Unicode.h"
#endif

#include "text/Text.h"

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

namespace ph { namespace text {

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
		glPushAttrib( GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT );

		mFont->enableAndBind();
		gl::draw(mVboMesh);
		mFont->unbind();

		glPopAttrib();

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
 
	if(!mVboMesh) return;

	glPushAttrib( GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT );

	gl::enableWireframe();
	gl::disable( GL_TEXTURE_2D );

	gl::draw(mVboMesh);

	glPopAttrib();
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
	if(!mInvalid) return;
	if(!mFont) return;
	if( mText.empty() )	return;

	// initialize variables
	const float		space = mFont->getAdvance(32, mFontSize);
	const float		height = getHeight() > 0.0f ? (getHeight() - mFont->getDescent(mFontSize)) : 0.0f;
	float			width, linewidth;
	size_t			index = 0;
	std::wstring	trimmed;
	
	// initialize cursor position
	Vec2f cursor(0.0f, std::floorf(mFont->getAscent(mFontSize) + 0.5f));

	// get word/line break information from Cinder's Unicode class if not available
	if( mMust.empty() || mAllow.empty() )
		findBreaksUtf16( mText, &mMust, &mAllow );

	// process text in chunks
	std::vector<size_t>::iterator	mitr = mMust.begin();
	std::vector<size_t>::iterator	aitr = std::find( mAllow.begin(), mAllow.end(), *mitr );
	while( aitr != mAllow.end() && mitr != mMust.end() && (height == 0.0f || cursor.y <= height) ) {
		// calculate the maximum allowed width for this line
		linewidth = getWidthAt( cursor.y );

		switch (mBoundary ) {
		case LINE:
			// aitr already points to mitr, thus the whole line is rendered
			trimmed = boost::trim_copy( mText.substr(index, *aitr - index + 1) );
			break;
		case WORD:
			// measure chunks to see if it fits
			trimmed = boost::trim_copy( mText.substr(index, *aitr - index + 1) );
			width = mFont->measure( trimmed, mFontSize ).getX2();

			// if they don't, remove the last chunk and try again until they all fit
			while( linewidth > 0.0f && width > linewidth && *aitr > index ) {
				--aitr;

				// perform fast approximation of width by subtracting last chunk
				std::wstring chunk = mText.substr(*aitr, *(aitr+1) - *aitr);
				width -= mFont->measure( chunk, mFontSize ).getX2();
			
				// perform precise measurement only if approximation was less than linewidth
				if( width <= linewidth ) {
					trimmed = boost::trim_copy( mText.substr(index, *aitr - index + 1) );
					width = mFont->measure( trimmed, mFontSize ).getX2();
				}
			}
			break;
		}

		// if any number of chunks fit on this line, render them
		if( *aitr > index ) {
			// adjust alignment
			switch( mAlignment ) {
			case CENTER: cursor.x = 0.5f * (linewidth - width); break;
			case RIGHT: cursor.x = (linewidth - width); break;
			}

			// add this fitting part of the text to the mesh 
			renderString( trimmed, &cursor );

			// advance cursor to new line
			if( !newLine(&cursor) ) break;			

			// advance iterators
			index = *aitr; // start at end of this chunk

			if( *aitr == *mitr ) 
				++mitr; // if all chunks on this line are rendered, end at next "must break"

			if( mitr != mMust.end() ) // try to render the remaining chunks of this line
				aitr = std::find( aitr, mAllow.end(), *mitr );
		}
		else {
			// advance to a new line with perhaps more space
			newLine(&cursor);

			// try to render the remaining chunks of this line
			aitr = std::find( aitr, mAllow.end(), *mitr );
		}
	}
}

void Text::renderString( const std::wstring &str, Vec2f *cursor )
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

			cursor->x += mFont->getAdvance(id, mFontSize);
		}
	}
}

void Text::createMesh()
{
	//
	if( mVertices.empty() || mIndices.empty() )
		return;

	//
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticIndices();
	layout.setStaticTexCoords2d();
	//layout.setStaticColorsRGBA();

	mVboMesh = gl::VboMesh( mVertices.size(), mIndices.size(), layout, GL_TRIANGLES );
	mVboMesh.bufferPositions( &mVertices.front(), mVertices.size() );
	mVboMesh.bufferIndices( mIndices );
	mVboMesh.bufferTexCoords2d( 0, mTexcoords );
	//mVboMesh.bufferColorsRGBA( colors );

	mInvalid = false;
}

Rectf Text::getBounds() const
{
	if(mBoundsInvalid) {
		//mBounds = mMesh.calcBoundingBox();
		mBoundsInvalid = false;
	}

	return mBounds;
}

std::string Text::getVertexShader() const
{
	// vertex shader
	const char *vs = 
		"#version 110\n"
		"\n"
		"void main()\n"
		"{\n"
		"	gl_FrontColor = gl_Color;\n"
		"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
		"\n"
		"	gl_Position = ftransform();\n"
		"}\n";

	return std::string(vs);
}

std::string Text::getFragmentShader() const
{
	// fragment shader
	const char *fs = 
		"#version 110\n"
		"\n"
		"uniform sampler2D	tex0;\n"
		"\n"
		"const float smoothness = 64.0;\n"
		"const float gamma = 2.2;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	// retrieve signed distance\n"
		"	vec4 clr = texture2D( tex0, gl_TexCoord[0].xy );\n"
		"	float sdf = clr.r;\n"
		"\n"
		"	// perform adaptive anti-aliasing of the edges\n"
		"	float w = clamp( smoothness * (abs(dFdx(gl_TexCoord[0].x)) + abs(dFdy(gl_TexCoord[0].y))), 0.0, 0.5);\n"
		"	float a = smoothstep(0.5-w, 0.5+w, sdf);\n"
		"\n"
		"	// gamma correction for linear attenuation\n"
		"	a = pow(a, 1.0/gamma);\n"
		"\n"
		"	// final color\n"
		"	gl_FragColor.rgb = gl_Color.rgb;\n"
		"	gl_FragColor.a = gl_Color.a * a;\n"
		"}\n";

	return std::string(fs);
}

bool Text::bindShader()
{
	if( ! mShader ) 
	{
		try { 
			mShader = gl::GlslProg( getVertexShader().c_str(), getFragmentShader().c_str() ); 
		}
		catch( const std::exception &e ) { 
			app::console() << "Could not load&compile shader: " << e.what() << std::endl;
			mShader = gl::GlslProg(); return false; 
		}
	}

	mShader.bind();
	mShader.uniform( "tex0", 0 );

	return true;
}

bool Text::unbindShader()
{
	if( mShader ) 
		mShader.unbind();
	
	return true;
}

} } // namespace ph::text