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
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class SimpleShaderApp : public AppBasic {
public:
	void setup();	
	void update();
	void draw();
private:
	gl::Texture		mTexture0;
	gl::Texture		mTexture1;

	gl::GlslProg	mShader;
};

void SimpleShaderApp::setup()
{
	try {
		// load the two textures
		mTexture0 = gl::Texture( loadImage( loadAsset("bottom.jpg") ) );
		mTexture1 = gl::Texture( loadImage( loadAsset("top.jpg") ) );
	
		// load and compile the shader
		mShader = gl::GlslProg( loadAsset("shader_vert.glsl"), loadAsset("shader_frag.glsl") );
	}
	catch( const std::exception &e ) {
		// if anything went wrong, show it in the output window
		console() << e.what() << std::endl;
	}
}

void SimpleShaderApp::update()
{
}

void SimpleShaderApp::draw()
{	
	// clear the window
	gl::clear();

	// bind the shader and tell it which texture units to use (see: shader_frag.glsl)
	mShader.bind();
	mShader.uniform("tex0", 0);	
	mShader.uniform("tex1", 1);

	// enable the use of textures
	gl::enable( GL_TEXTURE_2D );

	// bind them
	mTexture0.bind(0);
	mTexture1.bind(1);

	// now run the shader for every pixel in the window
	// by drawing a full screen rectangle
	gl::drawSolidRect( Rectf( getWindowBounds() ), false );

	// unbind textures and shader
	mTexture1.unbind();
	mTexture0.unbind();
	mShader.unbind();
}

CINDER_APP_BASIC( SimpleShaderApp, RendererGl )
