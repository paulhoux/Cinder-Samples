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

#include "cinder/Filesystem.h"
#include "cinder/ImageIo.h"
#include "cinder/Matrix.h"
#include "cinder/Surface.h"
#include "cinder/Utilities.h"

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"

#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/scoped.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PostProcessingApp : public App {
public:
	static void prepare( Settings *settings );

	void setup() override;
	void draw() override;

	void keyDown( KeyEvent event ) override;
	void fileDrop( FileDropEvent event ) override;

protected:
	gl::TextureRef			mImage;
	gl::GlslProgRef			mShader;

	fs::path				mFile;
};

void PostProcessingApp::prepare( Settings *settings )
{
	settings->setWindowSize( 1024, 768 );
	settings->disableFrameRate();
	settings->setTitle( "Post-processing" );
}

void PostProcessingApp::setup()
{
	// load test image
	try {
		mImage = gl::Texture::create( loadImage( loadAsset( "test.png" ) ) );
	}
	catch( const std::exception &e ) {
		console() << "Could not load image: " << e.what() << std::endl;
	}

	// load post-processing shader
	//  adapted from a shader by Iñigo Quílez ( http://www.iquilezles.org/ )
	try { mShader = gl::GlslProg::create( loadAsset( "post_process.vert" ), loadAsset( "post_process.frag" ) ); }
	catch( const std::exception &e ) { console() << "Could not load & compile shader: " << e.what() << std::endl; quit(); }
}

void PostProcessingApp::draw()
{
	// clear window
	gl::clear();

	// bind shader and set shader variables
	gl::ScopedGlslProg shader( mShader );
	mShader->uniform( "tex0", 0 );
	mShader->uniform( "time", (float)getElapsedSeconds() );

	// draw image
	gl::ScopedTextureBind tex0( mImage );

	gl::color( Color::white() );
	gl::drawSolidRect( getWindowBounds() );
}

void PostProcessingApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
		case KeyEvent::KEY_ESCAPE:
			quit();
			break;
		case KeyEvent::KEY_f:
			setFullScreen( !isFullScreen() );
			break;
	}
}

void PostProcessingApp::fileDrop( FileDropEvent event )
{
	// use the last of the dropped files
	mFile = event.getFile( event.getNumFiles() - 1 );

	try {
		// try loading image file
		mImage = gl::Texture::create( loadImage( mFile ) );
	}
	catch( const std::exception &e ) {
		console() << "Could not load image: " << e.what() << std::endl;
	}
}

CINDER_APP( PostProcessingApp, RendererGl, &PostProcessingApp::prepare )
