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

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"

#include "FXAA.h"
#include "Pistons.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// Our application class
class FXAAApp : public AppNative {
public:
	void setup();
	void update();
	void draw();

	void mouseDrag( MouseEvent event );	

	void keyDown( KeyEvent event );

	void resize();
private:
	void render();
private:
	CameraPersp         mCamera;

	gl::Fbo             mFboOriginal;
	gl::Fbo             mFboResult;

	gl::TextureRef      mArrow;

	Pistons             mPistons;
	FXAA                mFXAA;

	Timer               mTimer;
	double              mTime;
	double              mTimeOffset;

	int                 mDividerX;
};

void FXAAApp::setup()
{
	// Disable frame rate limiter for profiling
	disableFrameRate();

	// Set a proper title for our window
	getWindow()->setTitle("FXAA");

	// Load and compile our shaders and textures
	try { 
		mFXAA.setup();
		mArrow = gl::Texture::create( loadImage( loadAsset("arrow.png") ) );
	}
	catch( const std::exception& e ) { console() << e.what() << std::endl; quit(); }

	// Setup the pistons
	mPistons.setup();

	// initialize member variables and start the timer
	mDividerX = getWindowWidth() / 2;
	mTimeOffset = 0.0;
	mTimer.start();
}

void FXAAApp::update()
{
	// Keep track of time
	mTime = mTimer.getSeconds() + mTimeOffset;

	// Animate our camera
	double t = mTime / 10.0;

	float phi = (float) t;
	float theta = 3.14159265f * (0.25f + 0.2f * math<float>::sin(phi * 0.9f));
	float x = 150.0f * math<float>::cos(phi) * math<float>::cos(theta);
	float y = 150.0f * math<float>::sin(theta);
	float z = 150.0f * math<float>::sin(phi) * math<float>::cos(theta);

	mCamera.setEyePoint( Vec3f(x, y, z) );
	mCamera.setCenterOfInterestPoint( Vec3f(1, 50, 0) );
	mCamera.setAspectRatio( getWindowAspectRatio() );
	mCamera.setFov( 40.0f );

	// Update the pistons
	mPistons.update(mCamera);
}

void FXAAApp::draw()
{
	// Render our scene to the frame buffer
	render();
	
	// Perform FXAA
	mFXAA.apply(mFboResult, mFboOriginal);

	// Draw the frame buffer...
	gl::clear();
	gl::color( Color::white() );

	int w = getWindowWidth();
	int h = getWindowHeight();

	// ...while applying FXAA for the left side
	gl::draw( mFboResult.getTexture(), 
		Area(0, 0, mDividerX, h), Rectf(0, 0, (float)mDividerX, (float)h) );
	
	// ...and without FXAA for the right side
	gl::draw( mFboOriginal.getTexture(), 
		Area(mDividerX, 0, w, h), Rectf((float)mDividerX, 0, (float)w, (float)h) );

	// Draw divider
	gl::drawLine( Vec2f((float)mDividerX, 0.0f), Vec2f((float)mDividerX, (float)h) );

	Rectf rct = mArrow->getBounds();
	rct.offset( Vec2f(mDividerX - rct.getWidth()/2, h - rct.getHeight()) );

	gl::enableAlphaBlending();
	gl::draw( mArrow, rct );
	gl::disableAlphaBlending();
}

void FXAAApp::mouseDrag( MouseEvent event )
{
	// Adjust the position of the dividing line
	mDividerX = math<int>::clamp( event.getPos().x, 0, getWindowWidth() );
}

void FXAAApp::keyDown( KeyEvent event )
{
	switch( event.getCode() )
	{
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_SPACE:
		// Start/stop the animation
		if(mTimer.isStopped())
		{
			mTimeOffset += mTimer.getSeconds();
			mTimer.start();
		}
		else
			mTimer.stop();
		break;
	case KeyEvent::KEY_v:
		if( gl::isVerticalSyncEnabled() )
			gl::disableVerticalSync();
		else
			gl::enableVerticalSync();
		break;
	}
}

void FXAAApp::resize()
{
	// Do not enable multisampling and make sure the texture is interpolated bilinearly
	gl::Fbo::Format fmt;
	fmt.setMinFilter( GL_LINEAR );
	fmt.setMagFilter( GL_LINEAR );
	fmt.setColorInternalFormat( GL_RGBA );

	mFboOriginal = gl::Fbo( getWindowWidth(), getWindowHeight(), fmt );
	mFboOriginal.getTexture().setFlipped(true);

	mFboResult = gl::Fbo( getWindowWidth(), getWindowHeight(), fmt );
	mFboResult.getTexture().setFlipped(true);
	
	// Reset divider
	mDividerX = getWindowWidth() / 2;
}

void FXAAApp::render()
{
	// Enable frame buffer
	mFboOriginal.bindFramebuffer();

	// Draw scene
	gl::clear();
	gl::color( Color::white() );

	mPistons.draw(mCamera, (float)mTime);

	// Disable frame buffer
	mFboOriginal.unbindFramebuffer();
}

CINDER_APP_NATIVE( FXAAApp, RendererGl( RendererGl::AA_NONE ) )
