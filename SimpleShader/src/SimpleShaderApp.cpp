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

#include "cinder/ImageIo.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class SimpleShaderApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;

  private:
	gl::TextureRef mTexture0;
	gl::TextureRef mTexture1;

	gl::GlslProgRef mShader;
};

void SimpleShaderApp::setup()
{
	try {
		// load the two textures
		mTexture0 = gl::Texture::create( loadImage( loadAsset( "bottom.jpg" ) ) );
		mTexture1 = gl::Texture::create( loadImage( loadAsset( "top.jpg" ) ) );

		// load and compile the shader
		mShader = gl::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader.frag" ) );
	}
	catch( const std::exception &e ) {
		// if anything went wrong, show it in the output window
		console() << e.what() << std::endl;
	}
}

void SimpleShaderApp::update() {}

void SimpleShaderApp::draw()
{
	// clear the window
	gl::clear();

	// bind the shader and tell it which texture units to use (see: shader.frag)
	gl::ScopedGlslProg shader( mShader );
	mShader->uniform( "tex0", 0 );
	mShader->uniform( "tex1", 1 );

	// bind the textures
	gl::ScopedTextureBind tex0( mTexture0, uint8_t( 0 ) );
	gl::ScopedTextureBind tex1( mTexture1, uint8_t( 1 ) );

	// now run the shader for every pixel in the window
	// by drawing a full screen rectangle
	gl::drawSolidRect( getWindowBounds() );

	// the shader and the textures will automatically unbind
}

CINDER_APP( SimpleShaderApp, RendererGl )
