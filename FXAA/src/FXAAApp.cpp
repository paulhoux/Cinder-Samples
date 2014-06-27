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

using namespace ci;
using namespace ci::app;
using namespace std;

// Create a class that can draw a single box
struct Box
{		
	Box(float x, float z)
		: offset( Rand::randFloat(0.0f, 10.0f) )
		, color( CM_HSV, Rand::randFloat(0.0f, 0.1f), Rand::randFloat(0.0f, 1.0f), Rand::randFloat(0.25f, 1.0f) )
		, position( Vec3f(x, 0.0f, z) )
	{}

	void draw(float time)
	{
		float t = offset + time;
		float height = 55.0f + 45.0f * math<float>::sin(t);

		gl::color( color );
		gl::drawCube( position + Vec3f(0, 0.5f * height, 0), Vec3f(10.0f, height, 10.0f) );
	}

	float offset;
	Colorf color;
	Vec3f position;
};
typedef std::shared_ptr<Box> BoxRef;

class FXAAApp : public AppNative {
public:
	void setup();
	void update();
	void draw();

	void mouseDrag( MouseEvent event );	

	void keyDown( KeyEvent event );

	void resize();
private:
	CameraPersp         mCamera;
	gl::Fbo				mFbo;
	gl::GlslProg        mShader;
	gl::GlslProg        mFXAA;
	gl::TextureRef		mArrow;
	std::vector<BoxRef>	mBoxes;

	Timer				mTimer;
	double				mTime;
	double				mTimeOffset;

	int					mMouseX;
};

void FXAAApp::setup()
{
	// Set a proper title for our window
	getWindow()->setTitle("FXAA");

	// Load and compile our shaders and textures
	try { 
		mShader = gl::GlslProg( loadAsset("phong_vert.glsl"), loadAsset("phong_frag.glsl") ); 
		mFXAA = gl::GlslProg( loadAsset("fxaa_vert.glsl"), loadAsset("fxaa_frag.glsl") ); 
		mArrow = gl::Texture::create( loadImage( loadAsset("arrow.png") ) );
	}
	catch( const std::exception& e ) { console() << e.what() << std::endl; quit(); }

	// Create the boxes
	for(int x=-50; x<=50; x+=10)
		for(int z=-50; z<=50; z+=10)
			mBoxes.push_back( std::make_shared<Box>( float(x), float(z) ) );

	// initialize member variables and start the timer
	mTimeOffset = 0.0;
	mTimer.start();

	mMouseX = getWindowWidth() / 2;
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
}

void FXAAApp::draw()
{
	// Enable frame buffer
	mFbo.bindFramebuffer();

	// Draw scene
	gl::clear();

	gl::enableDepthRead();
	gl::enableDepthWrite();
	{
		gl::pushMatrices();
		gl::setMatrices( mCamera );
		{
			mShader.bind();
			{
				for(auto &box : mBoxes)
					box->draw((float) mTime);
			}
			mShader.unbind();
		}
		gl::popMatrices();
	}
	gl::disableDepthWrite();
	gl::disableDepthRead();

	// Disable frame buffer
	mFbo.unbindFramebuffer();

	// Draw the frame buffer...
	gl::clear();
	gl::color( Color::white() );

	mFXAA.bind();
	mFXAA.uniform("uTexture", 0);
	mFXAA.uniform("uBufferSize", Vec2f( mFbo.getSize() ));
	
	// ...while applying FXAA for the left side
	gl::draw( mFbo.getTexture(), Area(0, 0, mMouseX, mFbo.getHeight()), Rectf(0, 0, mMouseX, getWindowHeight()) );

	mFXAA.unbind();
	
	// ...and without FXAA for the right side
	gl::draw( mFbo.getTexture(), Area(mMouseX, 0, mFbo.getWidth(), mFbo.getHeight()), Rectf(mMouseX, 0, getWindowWidth(), getWindowHeight()) );

	// Draw dividing line
	gl::drawLine( Vec2f(mMouseX, 0.0f), Vec2f(mMouseX, getWindowHeight()) );

	Rectf rct = mArrow->getBounds();
	rct.offset( Vec2f(mMouseX - rct.getWidth()/2, getWindowHeight() - rct.getHeight()) );
	gl::enableAlphaBlending();
	gl::draw( mArrow, rct );
	gl::disableAlphaBlending();
}

void FXAAApp::mouseDrag( MouseEvent event )
{
	// Adjust the position of the dividing line
	mMouseX = math<int>::clamp( event.getPos().x, 0, getWindowWidth() );
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
	}
}

void FXAAApp::resize()
{
	gl::Fbo::Format fmt;
	fmt.setMinFilter( GL_LINEAR );
	fmt.setMagFilter( GL_LINEAR );

	mFbo = gl::Fbo( getWindowWidth(), getWindowHeight(), fmt );
	mFbo.getTexture().setFlipped(true);
	
	mMouseX = getWindowWidth() / 2;
}

CINDER_APP_NATIVE( FXAAApp, RendererGl( RendererGl::AA_NONE ) )
