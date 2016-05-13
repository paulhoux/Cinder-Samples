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

#include "cinder/app/App.h"
#include "cinder/Camera.h"
#include "cinder/ImageIo.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/gl.h"

#include "Pistons.h"
#include "SMAA.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// Our application class
class SMAAApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;

	void mouseDrag( MouseEvent event ) override;
	void keyDown( KeyEvent event ) override;

	void resize() override;

  private:
	void render();

  private:
	enum Mode { EDGE_DETECTION, BLEND_WEIGHTS, BLEND_NEIGHBORS };

	CameraPersp mCamera;

	Pistons mPistons;
	SMAA    mSMAA;

	gl::FboRef mFboOriginal;
	gl::FboRef mFboResult;

	gl::TextureRef mArrow;

	Timer  mTimer;
	double mTime;
	double mTimeOffset;

	int  mDividerX;
	Mode mMode;
};

void SMAAApp::setup()
{
	// Disable frame rate limiter for profiling
	disableFrameRate();

	// Set a proper title for our window
	getWindow()->setTitle( "SMAA" );

	// Load and compile our shaders and textures
	try {
		mArrow = gl::Texture::create( loadImage( loadAsset( "arrow.png" ) ) );

		mSMAA.setup();
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
		quit();
	}

	// Setup the pistons
	mPistons.setup();

	// initialize member variables and start the timer
	mDividerX = getWindowWidth() / 2;
	mMode = Mode::BLEND_NEIGHBORS;

	mTimeOffset = 0.0;
	mTimer.start();
}

void SMAAApp::update()
{
	// Keep track of time
	mTime = mTimer.getSeconds() + mTimeOffset;

	// Animate our camera
	double t = mTime / 10.0;

	float phi = (float)t;
	float theta = 3.14159265f * ( 0.25f + 0.2f * math<float>::sin( phi * 0.9f ) );
	float x = 150.0f * math<float>::cos( phi ) * math<float>::cos( theta );
	float y = 150.0f * math<float>::sin( theta );
	float z = 150.0f * math<float>::sin( phi ) * math<float>::cos( theta );

	mCamera.lookAt( vec3( x, y, z ), vec3( 1, 50, 0 ) );
	mCamera.setAspectRatio( getWindowAspectRatio() );
	mCamera.setFov( 40.0f );

	// Update the pistons
	mPistons.update( mCamera, mTime );
}

void SMAAApp::draw()
{
	gl::clear();

	// Render our scene to the frame buffer
	render();

	// Perform SMAA
	mSMAA.apply( mFboResult, mFboOriginal );

	int w = getWindowWidth();
	int h = getWindowHeight();

	// Draw the scene...
	{
		gl::ScopedColor color( Color::white() );
		gl::ScopedBlend blend( false );

		// ...with SMAA for the left side
		switch( mMode ) {
		case EDGE_DETECTION:
			gl::draw( mSMAA.getEdgePass(), Area( 0, 0, mDividerX, h ), Rectf( 0, 0, (float)mDividerX, (float)h ) );
			break;
		case BLEND_WEIGHTS:
			gl::draw( mSMAA.getBlendPass(), Area( 0, 0, mDividerX, h ), Rectf( 0, 0, (float)mDividerX, (float)h ) );
			break;
		case BLEND_NEIGHBORS:
			gl::draw( mFboResult->getColorTexture(), Area( 0, 0, mDividerX, h ), Rectf( 0, 0, (float)mDividerX, (float)h ) );
			break;
		}

		// ...and without SMAA for the right side
		gl::draw( mFboOriginal->getColorTexture(), Area( mDividerX, 0, w, h ), Rectf( (float)mDividerX, 0, (float)w, (float)h ) );
	}

	// Draw divider
	{
		gl::ScopedColor      color( Color::white() );
		gl::ScopedBlendAlpha blend;

		gl::drawLine( vec2( (float)mDividerX, 0.0f ), vec2( (float)mDividerX, (float)h ) );

		Rectf rct = mArrow->getBounds();
		rct.offset( vec2( mDividerX - rct.getWidth() / 2, h - rct.getHeight() ) );

		gl::draw( mArrow, rct );
	}
}

void SMAAApp::mouseDrag( MouseEvent event )
{
	// Adjust the position of the dividing line
	mDividerX = math<int>::clamp( event.getPos().x, 0, getWindowWidth() );
}

void SMAAApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_SPACE:
		// Start/stop the animation
		if( mTimer.isStopped() ) {
			mTimeOffset += mTimer.getSeconds();
			mTimer.start();
		}
		else
			mTimer.stop();
		break;
	case KeyEvent::KEY_1:
		mMode = Mode::EDGE_DETECTION;
		break;
	case KeyEvent::KEY_2:
		mMode = Mode::BLEND_WEIGHTS;
		break;
	case KeyEvent::KEY_3:
		mMode = Mode::BLEND_NEIGHBORS;
		break;
	case KeyEvent::KEY_v:
		gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
		break;
	}
}

void SMAAApp::resize()
{
	// Do not enable multisampling and make sure the texture is interpolated bilinearly
	gl::Texture2d::Format fmt;
	fmt.setMinFilter( GL_LINEAR );
	fmt.setMagFilter( GL_LINEAR );
	fmt.setInternalFormat( GL_RGB );

	mFboOriginal = gl::Fbo::create( getWindowWidth(), getWindowHeight(), gl::Fbo::Format().colorTexture( fmt ) );
	mFboResult = gl::Fbo::create( getWindowWidth(), getWindowHeight(), gl::Fbo::Format().colorTexture( fmt ) );

	// Reset divider
	mDividerX = getWindowWidth() / 2;
}

void SMAAApp::render()
{
	// Enable frame buffer
	gl::ScopedFramebuffer fbo( mFboOriginal );

	// Draw scene
	gl::clear();
	gl::ScopedColor color( Color::white() );
	gl::ScopedBlend blend( false );

	mPistons.draw( mCamera );
}

CINDER_APP( SMAAApp, RendererGl )
