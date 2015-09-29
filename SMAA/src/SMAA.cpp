/*
 Copyright (c) 2014, Paul Houx - All rights reserved.
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

#include "SMAA.h"

#include "AreaTex.h"
#include "SearchTex.h"

#include "cinder/app/App.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void SMAA::setup()
{
	// Load and compile our shaders
	mSMAAFirstPass = gl::GlslProg::create( loadAsset( "smaa1.vert" ), loadAsset( "smaa1.frag" ) );
	mSMAASecondPass = gl::GlslProg::create( loadAsset( "smaa2.vert" ), loadAsset( "smaa2.frag" ) );
	mSMAAThirdPass = gl::GlslProg::create( loadAsset( "smaa3.vert" ), loadAsset( "smaa3.frag" ) );

	// Create lookup textures
	gl::Texture2d::Format fmt;
	fmt.setMinFilter( GL_LINEAR );
	fmt.setMagFilter( GL_LINEAR );
	fmt.setWrap( GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER );
	fmt.setInternalFormat( GL_RED );
	fmt.setSwizzleMask( GL_RED, GL_RED, GL_RED, GL_ONE );
	fmt.loadTopDown( true );

	// Search Texture (Grayscale, 8 bits unsigned)
	mSearchTex = gl::Texture2d::create( searchTexBytes, GL_RED, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, fmt );

	// Area Texture (Red+Green Channels, 8 bits unsigned)
	fmt.setInternalFormat( GL_RG );
	fmt.setSwizzleMask( GL_RED, GL_GREEN, GL_ZERO, GL_ONE );
	mAreaTex = gl::Texture2d::create( areaTexBytes, GL_RG, AREATEX_WIDTH, AREATEX_HEIGHT, fmt );
}

void SMAA::apply( const ci::gl::FboRef& destination, const ci::gl::FboRef& source )
{
	// Source and destination should have the same size
	assert( destination->getWidth() == source->getWidth() );
	assert( destination->getHeight() == source->getHeight() );

	// Create or resize frame buffers
	int w = source->getWidth();
	int h = source->getHeight();

	gl::Texture2d::Format fmt;
	fmt.setMinFilter( GL_LINEAR );
	fmt.setMagFilter( GL_LINEAR );
	fmt.setWrap( GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER );

	if( !mFboEdgePass || mFboEdgePass->getWidth() != w || mFboEdgePass->getHeight() != h ) {
		// Note: using only RG channels decreases performance on NVIDIA, 
		//       while RGBA does not decrease performance on Intel and AMD
		//fmt.setInternalFormat( GL_RG );
		//fmt.setSwizzleMask( GL_RED, GL_GREEN, GL_ZERO, GL_ONE );
		mFboEdgePass = gl::Fbo::create( w, h, gl::Fbo::Format().colorTexture( fmt ).disableDepth().stencilBuffer() );
	}

	if( !mFboBlendPass || mFboBlendPass->getWidth() != w || mFboBlendPass->getHeight() != h ) {
		fmt.setInternalFormat( GL_RGBA );
		fmt.setSwizzleMask( GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA );
		mFboBlendPass = gl::Fbo::create( w, h, gl::Fbo::Format().colorTexture( fmt ).disableDepth() );
	}

	// Apply first two passes
	doEdgePass( source );
	doBlendPass();

	// Apply SMAA
	gl::ScopedFramebuffer fbo( destination );
	gl::ScopedTextureBind tex0( source->getColorTexture(), 0 );
	gl::ScopedTextureBind tex1( mFboBlendPass->getColorTexture(), 1 );
	gl::ScopedGlslProg shader( mSMAAThirdPass );
	mSMAAThirdPass->uniform( "uColorTex", 0 );
	mSMAAThirdPass->uniform( "uBlendTex", 1 );
	mSMAAThirdPass->uniform( "SMAA_RT_METRICS", vec4( 1.0f / w, 1.0f / h, (float)w, (float)h ) );
	{
		gl::clear();
		gl::ScopedColor color( Color::white() );
		gl::ScopedBlend blend( false );

		gl::drawSolidRect( destination->getBounds() );
	}
}

void SMAA::doEdgePass( const ci::gl::FboRef& source )
{
	int w = mFboEdgePass->getWidth();
	int h = mFboEdgePass->getHeight();

	// Enable frame buffer
	gl::ScopedFramebuffer fbo( mFboEdgePass );
	gl::ScopedTextureBind tex0( source->getColorTexture(), 0 );
	gl::ScopedGlslProg shader( mSMAAFirstPass );
	mSMAAFirstPass->uniform( "uColorTex", 0 );
	mSMAAFirstPass->uniform( "SMAA_RT_METRICS", vec4( 1.0f / w, 1.0f / h, (float)w, (float)h ) );
	{
		gl::clear();
		gl::clearStencil( 0 );

		gl::ScopedColor color( Color::white() );
		gl::ScopedBlend blend( false );

		gl::enableStencilTest();

		gl::stencilFunc( GL_ALWAYS, 1, 0xFF ); // replace, ref = 1
		gl::stencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );
		gl::stencilMask( 0xFF );

		gl::drawSolidRect( mFboEdgePass->getBounds() );

		gl::disableStencilTest();
	}
}

void SMAA::doBlendPass()
{
	int w = mFboBlendPass->getWidth();
	int h = mFboBlendPass->getHeight();

	// Enable frame buffer
	gl::ScopedFramebuffer fbo( mFboBlendPass );

	gl::ScopedTextureBind tex0( mFboEdgePass->getColorTexture(), 0 );
	gl::ScopedTextureBind tex1( mAreaTex, 1 );
	gl::ScopedTextureBind tex2( mSearchTex, 2 );

	gl::ScopedGlslProg shader( mSMAASecondPass );
	mSMAASecondPass->uniform( "uEdgesTex", 0 );
	mSMAASecondPass->uniform( "uAreaTex", 1 );
	mSMAASecondPass->uniform( "uSearchTex", 2 );
	mSMAASecondPass->uniform( "SMAA_RT_METRICS", vec4( 1.0f / w, 1.0f / h, (float)w, (float)h ) );
	{
		gl::clear();
		gl::ScopedColor color( Color::white() );
		gl::ScopedBlend blend( false );

		gl::enableStencilTest();

		gl::stencilFunc( GL_EQUAL, 1, 0xFF ); // func = equal, pass = keep, ref = 1
		gl::stencilMask( 0x00 );

		gl::drawSolidRect( mFboBlendPass->getBounds() );

		gl::disableStencilTest();
	}
}