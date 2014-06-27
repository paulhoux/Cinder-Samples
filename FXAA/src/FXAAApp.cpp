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
		, distance(0.0f)
	{}

	// When drawn, the distance to the camera is also calculated,
	// so we can later use it to sort the boxes from front to back.
	// Rendering from front to back is more efficient, as fragments
	// that are behind other fragments will be culled early.
	void draw(float time, const ci::CameraPersp& camera)
	{
		float t = offset + time;
		float height = 55.0f + 45.0f * math<float>::sin(t);

		position.y = 0.5f * height;
		distance = position.distanceSquared( camera.getEyePoint() );

		gl::color( color );
		gl::drawCube( position, Vec3f(10.0f, height, 10.0f) );
	}

	// Our custom sorting comparator
	static int CompareByDistanceToCamera(const void* a, const void* b)
	{
		const Box* pA = reinterpret_cast<const Box*>(a);
		const Box* pB = reinterpret_cast<const Box*>(b);
		if(pA->distance < pB->distance)
			return -1;
		if(pA->distance > pB->distance)
			return 1;
		return 0;
	}

	float offset;
	Colorf color;
	Vec3f position;
	float distance;
};

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
	CameraPersp         mCamera;
	gl::Fbo				mFbo;
	gl::GlslProg        mShader;
	gl::GlslProg        mFXAA;
	gl::TextureRef		mArrow;
	std::vector<Box>	mBoxes;

	Timer				mTimer;
	double				mTime;
	double				mTimeOffset;

	int					mDividerX;
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
			mBoxes.push_back( Box( float(x), float(z) ) );

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

	// sort boxes by distance to camera
	std::qsort( &mBoxes.front(), mBoxes.size(), sizeof(Box), &Box::CompareByDistanceToCamera );
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
					box.draw((float) mTime, mCamera);
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

	int w = getWindowWidth();
	int h = getWindowHeight();

	// ...while applying FXAA for the left side
	mFXAA.bind();
	mFXAA.uniform("uTexture", 0);
	mFXAA.uniform("uRcpBufferSize", Vec2f::one() / Vec2f( mFbo.getSize() ));
	{
		gl::draw( mFbo.getTexture(), 
			Area(0, 0, mDividerX, h), Rectf(0, 0, (float)mDividerX, (float)h) );
	}
	mFXAA.unbind();
	
	// ...and without FXAA for the right side
	gl::draw( mFbo.getTexture(), 
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
	}
}

void FXAAApp::resize()
{
	// Do not enable multisampling and make sure the texture is interpolated bilinearly
	gl::Fbo::Format fmt;
	fmt.setMinFilter( GL_LINEAR );
	fmt.setMagFilter( GL_LINEAR );

	mFbo = gl::Fbo( getWindowWidth(), getWindowHeight(), fmt );
	mFbo.getTexture().setFlipped(true);
	
	// Reset divider
	mDividerX = getWindowWidth() / 2;
}

CINDER_APP_NATIVE( FXAAApp, RendererGl( RendererGl::AA_NONE ) )
