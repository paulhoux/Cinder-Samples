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

#include "cinder/Camera.h"
#include "cinder/ImageIo.h"
#include "cinder/TriMesh.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/app/AppBasic.h"

#define SCENE_SIZE 512
#define BLUR_SIZE 128

using namespace ci;
using namespace ci::app;
using namespace std;

class BloomingNeonApp : public AppBasic {
 public:
	 void prepareSettings( Settings *settings );

	 void setup();
	 void update();
	 void draw();

	 void render();

	 void keyDown( KeyEvent event );

	 void drawStrokedRect( const Rectf &rect );
protected:
	gl::Fbo			mFboScene;
	gl::Fbo			mFboBlur1;
	gl::Fbo			mFboBlur2;

	gl::GlslProg	mShaderBlur;
	gl::GlslProg	mShaderPhong;
	
	gl::Texture		mTextureColor;
	gl::Texture		mTextureIllumination;
	gl::Texture		mTextureSpecular;

	TriMesh			mMesh;
	Matrix44f		mTransform;
	CameraPersp		mCamera;
};

void BloomingNeonApp::prepareSettings( Settings *settings )
{
	settings->setResizable(false);
	settings->setWindowSize(780 + 256, 256);
	settings->setTitle("Blurred Neon Effect Using Fbo's and Shaders");
}

void BloomingNeonApp::setup()
{
	gl::Fbo::Format fmt;
	fmt.setSamples(8);
	fmt.setCoverageSamples(8);

	// setup our scene Fbo
	mFboScene = gl::Fbo(SCENE_SIZE, SCENE_SIZE, fmt);

	// setup our blur Fbo's, smaller ones will generate a bigger blur
	mFboBlur1 = gl::Fbo(BLUR_SIZE, BLUR_SIZE);
	mFboBlur2 = gl::Fbo(BLUR_SIZE, BLUR_SIZE);

	// load and compile the shaders
	try { 
		mShaderBlur = gl::GlslProg( loadAsset("blur_vert.glsl"), loadAsset("blur_frag.glsl")); 
		mShaderPhong = gl::GlslProg( loadAsset("phong_vert.glsl"), loadAsset("phong_frag.glsl")); 
	} 
	catch( const std::exception &e ) { 
		console() << e.what() << endl; 
		quit();
	}

	// setup the stuff to render our ducky
	mTransform.setToIdentity();	

	// model and textures generously provided by AngryFly: 
	//   http://www.turbosquid.com/3d-models/free-3ds-mode-space/588767
	mMesh.read( loadAsset("space_frigate.msh") );

	mTextureIllumination = gl::Texture( loadImage( loadAsset("space_frigate_illumination.jpg") ) );
	mTextureColor = gl::Texture( loadImage( loadAsset("space_frigate_color.jpg") ) );
	mTextureSpecular = gl::Texture( loadImage( loadAsset("space_frigate_specular.jpg") ) );

	//
	mCamera.setEyePoint( Vec3f(0.0f, 8.0f, 25.0f) );
	mCamera.setCenterOfInterestPoint( Vec3f(0.0f, -1.0f, 0.0f) );
	mCamera.setPerspective( 60.0f, getWindowAspectRatio(), 1.0f, 1000.0f );
}

void BloomingNeonApp::update()
{
	mTransform.setToIdentity();
	mTransform.rotate( Vec3f::yAxis(), (float) getElapsedSeconds() * 0.2f );
}

void BloomingNeonApp::draw()
{
	// clear our window
	gl::clear( Color::black() );

	// store our viewport, so we can restore it later
	Area viewport = gl::getViewport();

	// render scene into mFboScene using illumination texture
	mTextureIllumination.enableAndBind();
	mTextureSpecular.bind(1);
	gl::setViewport( mFboScene.getBounds() );
	mFboScene.bindFramebuffer();
		gl::pushMatrices();
			gl::setMatricesWindow(SCENE_SIZE, SCENE_SIZE, false);
			gl::clear( Color::black() );
			render();
		gl::popMatrices();
	mFboScene.unbindFramebuffer();

	// bind the blur shader
	mShaderBlur.bind();
	mShaderBlur.uniform("tex0", 0); // use texture unit 0
 
	// tell the shader to blur horizontally and the size of 1 pixel
	mShaderBlur.uniform("sample_offset", Vec2f(1.0f/mFboBlur1.getWidth(), 0.0f));
	mShaderBlur.uniform("attenuation", 2.5f);

	// copy a horizontally blurred version of our scene into the first blur Fbo
	gl::setViewport( mFboBlur1.getBounds() );
	mFboBlur1.bindFramebuffer();
		mFboScene.bindTexture(0);
		gl::pushMatrices();
			gl::setMatricesWindow(BLUR_SIZE, BLUR_SIZE, false);
			gl::clear( Color::black() );
			gl::drawSolidRect( mFboBlur1.getBounds() );
		gl::popMatrices();
		mFboScene.unbindTexture();
	mFboBlur1.unbindFramebuffer();	
 
	// tell the shader to blur vertically and the size of 1 pixel
	mShaderBlur.uniform("sample_offset", Vec2f(0.0f, 1.0f/mFboBlur2.getHeight()));
	mShaderBlur.uniform("attenuation", 2.5f);

	// copy a vertically blurred version of our blurred scene into the second blur Fbo
	gl::setViewport( mFboBlur2.getBounds() );
	mFboBlur2.bindFramebuffer();
		mFboBlur1.bindTexture(0);
		gl::pushMatrices();
			gl::setMatricesWindow(BLUR_SIZE, BLUR_SIZE, false);
			gl::clear( Color::black() );
			gl::drawSolidRect( mFboBlur2.getBounds() );
		gl::popMatrices();
		mFboBlur1.unbindTexture();
	mFboBlur2.unbindFramebuffer();

	// unbind the shader
	mShaderBlur.unbind();

	// render scene into mFboScene using color texture
	mTextureColor.enableAndBind();
	mTextureSpecular.bind(1);
	gl::setViewport( mFboScene.getBounds() );
	mFboScene.bindFramebuffer();
		gl::pushMatrices();
			gl::setMatricesWindow(SCENE_SIZE, SCENE_SIZE, false);
			gl::clear( Color::black() );
			render();
		gl::popMatrices();
	mFboScene.unbindFramebuffer();

	// restore the viewport
	gl::setViewport( viewport );

	// because the Fbo's have their origin in the LOWER-left corner,
	// flip the Y-axis before drawing
	gl::pushModelView();
	gl::translate( Vec2f(0, 256) );
	gl::scale( Vec3f(1, -1, 1) );

	// draw the 3 Fbo's 
	gl::color( Color::white() );
	gl::draw( mFboScene.getTexture(), Rectf(0, 0, 256, 256) );
	drawStrokedRect( Rectf(0, 0, 256, 256) );

	gl::draw( mFboBlur1.getTexture(), Rectf(260, 0, 260 + 256, 256) );
	drawStrokedRect( Rectf(260, 0, 260 + 256, 256) );

	gl::draw( mFboBlur2.getTexture(), Rectf(520, 0, 520 + 256, 256) );
	drawStrokedRect( Rectf(520, 0, 520 + 256, 256) );

	// draw our scene with the blurred version added as a blend
	gl::color( Color::white() );
	gl::draw( mFboScene.getTexture(), Rectf(780, 0, 780 + 256, 256) );

	gl::enableAdditiveBlending();
	gl::draw( mFboBlur2.getTexture(), Rectf(780, 0, 780 + 256, 256) );
	gl::disableAlphaBlending();
	drawStrokedRect( Rectf(780, 0, 780 + 256, 256) );
	
	// restore the modelview matrix
	gl::popModelView();

	// draw info
	gl::enableAlphaBlending();
	gl::drawStringCentered("Basic Scene", Vec2f(128, 236));
	gl::drawStringCentered("First Blur Pass (Horizontal)", Vec2f(260 + 128, 236));
	gl::drawStringCentered("Second Blur Pass (Vertical)", Vec2f(520 + 128, 236));
	gl::drawStringCentered("Final Scene", Vec2f(780 + 128, 236));
	gl::disableAlphaBlending();
}

void BloomingNeonApp::render()
{
	// get the current viewport
	Area viewport = gl::getViewport();

	// adjust the aspect ratio of the camera
	mCamera.setAspectRatio( viewport.getWidth() / (float) viewport.getHeight() );
	
	// render our scene (see the Picking3D sample for more info)
	gl::pushMatrices();

		gl::setMatrices( mCamera );
		gl::enableDepthRead();
		gl::enableDepthWrite();
		
			mShaderPhong.bind();
			mShaderPhong.uniform("tex_diffuse", 0);
			mShaderPhong.uniform("tex_specular", 1);
				gl::pushModelView();
				gl::multModelView( mTransform );
					gl::color( Color::white() );
					gl::draw( mMesh );
				gl::popModelView();		
			mShaderPhong.unbind();

		gl::disableDepthWrite();
		gl::disableDepthRead();

	gl::popMatrices();
}

void BloomingNeonApp::keyDown( KeyEvent event )
{
	switch( event.getCode() )
	{
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	}
}

void BloomingNeonApp::drawStrokedRect( const Rectf &rect )
{
	// we don't want any texture on our lines
	glDisable(GL_TEXTURE_2D);

	gl::drawLine(rect.getUpperLeft(), rect.getUpperRight());
	gl::drawLine(rect.getUpperRight(), rect.getLowerRight());
	gl::drawLine(rect.getLowerRight(), rect.getLowerLeft());
	gl::drawLine(rect.getLowerLeft(), rect.getUpperLeft());
}

CINDER_APP_BASIC( BloomingNeonApp, RendererGl )