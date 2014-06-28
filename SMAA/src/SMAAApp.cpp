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

#include "AreaTex.h"
#include "SearchTex.h"

#include "Pistons.h"
#include "Shader.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// Our application class
class SMAAApp : public AppNative {
public:
	void setup();
	void update();
	void draw();

	void mouseDrag( MouseEvent event );

	void keyDown( KeyEvent event );

	void resize();
private:
	void createTextures();

	void renderScene();
	void smaaFirstPass();
	void smaaSecondPass();
	void smaaThirdPass();
private:
	CameraPersp         mCamera;
	Pistons             mPistons;

	gl::Fbo             mFboScene;
	gl::Fbo             mFboFirstPass;
	gl::Fbo             mFboSecondPass;
	gl::Fbo             mFboThirdPass;

	gl::TextureRef      mImage;
	gl::TextureRef      mArrow;
	gl::TextureRef      mAreaTex;
	gl::TextureRef      mSearchTex;

	// The Shader class allows us to write and use shaders with support for #include
	ShaderRef           mSMAAFirstPass;		// edge detection
	ShaderRef           mSMAASecondPass;	// blending weight calculation
	ShaderRef           mSMAAThirdPass;		// neighborhood blending

	Timer               mTimer;
	double              mTime;
	double              mTimeOffset;

	int                 mDividerX;
	int                 mPass;
};

void SMAAApp::setup()
{
	// Set a proper title for our window
	getWindow()->setTitle("SMAA");
	//getWindow()->setSize(1280,720);

	// Load and compile our shaders and textures
	try { 
		mSMAAFirstPass = Shader::create("smaa_1st"); // gl::GlslProg( loadAsset("smaa_1st_vert.glsl"), loadAsset("smaa_1st_frag.glsl") );
		mSMAASecondPass = Shader::create("smaa_2nd"); // gl::GlslProg( loadAsset("smaa_2nd_vert.glsl"), loadAsset("smaa_2nd_frag.glsl") );
		mSMAAThirdPass = Shader::create("smaa_3rd"); // gl::GlslProg( loadAsset("smaa_3rd_vert.glsl"), loadAsset("smaa_3rd_frag.glsl") );
		mArrow = gl::Texture::create( loadImage( loadAsset("arrow.png") ) );
		mImage = gl::Texture::create( loadImage( loadAsset("unigine.png") ) );
	}
	catch( const std::exception& e ) { console() << e.what() << std::endl; quit(); }

	createTextures();

	// Setup the pistons
	mPistons.setup();

	// initialize member variables and start the timer
	mDividerX = getWindowWidth() / 2;
	mPass = 3;
	mTimeOffset = 0.0;
	mTimer.start();
}

void SMAAApp::update()
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

void SMAAApp::draw()
{
	// Render our scene to the frame buffer
	renderScene();

	// Perform SMAA
	smaaFirstPass();
	smaaSecondPass();
	smaaThirdPass();

	// Draw the scene...
	gl::clear();
	gl::color( Color::white() );

	int w = getWindowWidth();
	int h = getWindowHeight();

	// ...with SMAA for the left side
	switch(mPass)
	{
	case 1:
		gl::draw( mFboFirstPass.getTexture(),
			Area(0, 0, mDividerX, h), Rectf(0, 0, (float)mDividerX, (float)h) );
		break;
	case 2:
		gl::draw( mFboSecondPass.getTexture(),
			Area(0, 0, mDividerX, h), Rectf(0, 0, (float)mDividerX, (float)h) );
		break;
	case 3:
		gl::draw( mFboThirdPass.getTexture(),
			Area(0, 0, mDividerX, h), Rectf(0, 0, (float)mDividerX, (float)h) );
		break;
	}
	
	// ...and without SMAA for the right side
	gl::draw( mFboScene.getTexture(), 
		Area(mDividerX, 0, w, h), Rectf((float)mDividerX, 0, (float)w, (float)h) );

	// Draw divider
	gl::drawLine( Vec2f((float)mDividerX, 0.0f), Vec2f((float)mDividerX, (float)h) );

	Rectf rct = mArrow->getBounds();
	rct.offset( Vec2f(mDividerX - rct.getWidth()/2, h - rct.getHeight()) );

	gl::enableAlphaBlending();
	gl::draw( mArrow, rct );
	gl::disableAlphaBlending();
}

void SMAAApp::mouseDrag( MouseEvent event )
{
	// Adjust the position of the dividing line
	mDividerX = math<int>::clamp( event.getPos().x, 0, getWindowWidth() );
}

void SMAAApp::keyDown( KeyEvent event )
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
	case KeyEvent::KEY_1:
		mPass = 1;
		break;
	case KeyEvent::KEY_2:
		mPass = 2;
		break;
	case KeyEvent::KEY_3:
		mPass = 3;
		break;
	}
}

void SMAAApp::resize()
{
	// Do not enable multisampling and make sure the texture is interpolated bilinearly
	gl::Fbo::Format fmt;
	fmt.setMinFilter( GL_LINEAR );
	fmt.setMagFilter( GL_LINEAR );
	fmt.setColorInternalFormat( GL_RG );

	mFboFirstPass = gl::Fbo( getWindowWidth(), getWindowHeight(), fmt );
	mFboFirstPass.getTexture().setFlipped(true);

	fmt.setColorInternalFormat( GL_RGBA );

	mFboSecondPass = gl::Fbo( getWindowWidth(), getWindowHeight(), fmt );
	mFboSecondPass.getTexture().setFlipped(true);

	mFboThirdPass = gl::Fbo( getWindowWidth(), getWindowHeight(), fmt );
	mFboThirdPass.getTexture().setFlipped(true);

	mFboScene = gl::Fbo( getWindowWidth(), getWindowHeight(), fmt );
	mFboScene.getTexture().setFlipped(true);
	
	// Reset divider
	mDividerX = getWindowWidth() / 2;
}

void SMAAApp::createTextures()
{
	gl::Texture::Format fmt;
	fmt.setMinFilter( GL_LINEAR );
	fmt.setMagFilter( GL_LINEAR );
	fmt.setWrap( GL_CLAMP, GL_CLAMP );

	// Search Texture (Grayscale, 8 bits unsigned)
	fmt.setInternalFormat( GL_LUMINANCE );
	mSearchTex = gl::Texture::create(searchTexBytes, GL_LUMINANCE, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, fmt);

	// Area Texture (Red+Green Channels, 8 bits unsigned)
	fmt.setInternalFormat( GL_RG );
	mAreaTex = gl::Texture::create(areaTexBytes, GL_RG, AREATEX_WIDTH, AREATEX_HEIGHT, fmt);
}

void SMAAApp::renderScene()
{
	// Enable frame buffer
	mFboScene.bindFramebuffer();

	// Draw scene
	gl::clear();
	gl::color( Color::white() );

	mPistons.draw(mCamera, (float)mTime);
	//gl::draw( mImage );

	// Disable frame buffer
	mFboScene.unbindFramebuffer();
}

void SMAAApp::smaaFirstPass()
{
	// Enable frame buffer
	mFboFirstPass.bindFramebuffer();

	int w = getWindowWidth();
	int h = getWindowHeight();

	mSMAAFirstPass->prog().bind();
	mSMAAFirstPass->prog().uniform("uColorTex", 0);
	mSMAAFirstPass->prog().uniform("SMAA_RT_METRICS", Vec4f(1.0f/w, 1.0f/h, (float)w, (float)h));
	{
		gl::clear();
		gl::color( Color::white() );

		gl::draw( mFboScene.getTexture(), mFboFirstPass.getBounds() );
	}
	mSMAAFirstPass->prog().unbind();

	// Disable frame buffer
	mFboFirstPass.unbindFramebuffer();
}

void SMAAApp::smaaSecondPass()
{
	// Enable frame buffer
	mFboSecondPass.bindFramebuffer();

	mAreaTex->bind(1);
	mSearchTex->bind(2);

	int w = getWindowWidth();
	int h = getWindowHeight();

	mSMAASecondPass->prog().bind();
	mSMAASecondPass->prog().uniform("uEdgesTex", 0);
	mSMAASecondPass->prog().uniform("uAreaTex", 1);
	mSMAASecondPass->prog().uniform("uSearchTex", 2);
	mSMAASecondPass->prog().uniform("SMAA_RT_METRICS", Vec4f(1.0f/w, 1.0f/h, (float)w, (float)h));
	{
		gl::clear();
		gl::color( Color::white() );

		gl::draw( mFboFirstPass.getTexture(), mFboSecondPass.getBounds() );
	}
	mSMAASecondPass->prog().unbind();

	mSearchTex->unbind();
	mAreaTex->unbind();

	// Disable frame buffer
	mFboSecondPass.unbindFramebuffer();
}

void SMAAApp::smaaThirdPass()
{
	// Enable frame buffer
	mFboThirdPass.bindFramebuffer();

	mFboSecondPass.getTexture().bind(1);

	int w = getWindowWidth();
	int h = getWindowHeight();

	mSMAAThirdPass->prog().bind();
	mSMAAThirdPass->prog().uniform("uColorTex", 0);
	mSMAAThirdPass->prog().uniform("uBlendTex", 1);
	mSMAAThirdPass->prog().uniform("SMAA_RT_METRICS", Vec4f(1.0f/w, 1.0f/h, (float)w, (float)h));
	{
		gl::clear();
		gl::color( Color::white() );

		gl::draw( mFboScene.getTexture(), mFboThirdPass.getBounds() );
	}
	mSMAAThirdPass->prog().unbind();

	mFboSecondPass.getTexture().unbind();

	// Disable frame buffer
	mFboThirdPass.unbindFramebuffer();
}

CINDER_APP_NATIVE( SMAAApp, RendererGl( RendererGl::AA_NONE ) )
