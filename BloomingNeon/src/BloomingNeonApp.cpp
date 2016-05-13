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
#include "cinder/Font.h"
#include "cinder/ImageIo.h"
#include "cinder/TriMesh.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#define SCENE_SIZE 512
#define BLUR_SIZE 128

using namespace ci;
using namespace ci::app;
using namespace std;

class BloomingNeonApp : public App {
  public:
	static void prepare( Settings *settings );

	void setup();
	void update();
	void draw();

	void render();

	void keyDown( KeyEvent event );

	void drawStrokedRect( const Rectf &rect );

  protected:
	ci::Font mFont;

	gl::FboRef mFboScene;
	gl::FboRef mFboBlur1;
	gl::FboRef mFboBlur2;

	gl::GlslProgRef mShaderBlur;
	gl::GlslProgRef mShaderPhong;

	gl::TextureRef mTextureColor;
	gl::TextureRef mTextureIllumination;
	gl::TextureRef mTextureSpecular;
	gl::TextureRef mTextureArrows;

	gl::BatchRef mBatch;
	CameraPersp  mCamera;

	mat4 mTransform;
};

void BloomingNeonApp::prepare( Settings *settings )
{
	settings->setResizable( false );
	settings->setWindowSize( 800, 800 );
	settings->setTitle( "Blurred Neon Effect Using Fbo's and Shaders" );
}

void BloomingNeonApp::setup()
{
	mFont = Font( "Arial", 24.0f );

	gl::Fbo::Format fmt;
	fmt.setSamples( 8 );
	fmt.setCoverageSamples( 8 );

	// setup our scene Fbo
	mFboScene = gl::Fbo::create( SCENE_SIZE, SCENE_SIZE, fmt );

	// setup our blur Fbo's, smaller ones will generate a bigger blur
	mFboBlur1 = gl::Fbo::create( BLUR_SIZE, BLUR_SIZE );
	mFboBlur2 = gl::Fbo::create( BLUR_SIZE, BLUR_SIZE );

	// load and compile the shaders
	try {
		mShaderBlur = gl::GlslProg::create( loadAsset( "blur.vert" ), loadAsset( "blur.frag" ) );
		mShaderPhong = gl::GlslProg::create( loadAsset( "phong.vert" ), loadAsset( "phong.frag" ) );
	}
	catch( const std::exception &e ) {
		console() << e.what() << endl;
		quit();
	}

	// setup the stuff to render our ducky:
	// model and textures generously provided by AngryFly:
	//   http://www.turbosquid.com/3d-models/free-3ds-mode-space/588767
	TriMeshRef mesh = TriMesh::create();
	mesh->read( loadAsset( "space_frigate.msh" ) );
	mBatch = gl::Batch::create( *mesh, mShaderPhong );

	mTextureIllumination = gl::Texture::create( loadImage( loadAsset( "space_frigate_illumination.jpg" ) ) );
	mTextureColor = gl::Texture::create( loadImage( loadAsset( "space_frigate_color.jpg" ) ) );
	mTextureSpecular = gl::Texture::create( loadImage( loadAsset( "space_frigate_specular.jpg" ) ) );
	mTextureArrows = gl::Texture::create( loadImage( loadAsset( "arrows.png" ) ) );

	//
	mCamera.setPerspective( 60.0f, getWindowAspectRatio(), 1.0f, 1000.0f );
	mCamera.lookAt( vec3( 0.0f, 8.0f, 25.0f ), vec3( 0.0f, -1.0f, 0.0f ) );
}

void BloomingNeonApp::update()
{
	mTransform = mat4();
	mTransform *= glm::toMat4( glm::angleAxis( (float)getElapsedSeconds() * 0.2f, vec3( 1, 0, 0 ) ) );
	mTransform *= glm::toMat4( glm::angleAxis( (float)getElapsedSeconds() * 0.1f, vec3( 0, 1, 0 ) ) );
}

void BloomingNeonApp::draw()
{
	// clear our window
	gl::clear( Color::black() );

	gl::pushMatrices();

	// render scene into mFboScene using illumination texture
	{
		gl::ScopedFramebuffer fbo( mFboScene );
		gl::ScopedViewport    viewport( 0, 0, mFboScene->getWidth(), mFboScene->getHeight() );

		gl::ScopedTextureBind tex0( mTextureIllumination, (uint8_t)0 );
		gl::ScopedTextureBind tex1( mTextureSpecular, (uint8_t)1 );

		gl::setMatricesWindow( SCENE_SIZE, SCENE_SIZE );
		gl::clear( Color::black() );

		render();
	}

	// bind the blur shader
	{
		gl::ScopedGlslProg shader( mShaderBlur );
		mShaderBlur->uniform( "tex0", 0 ); // use texture unit 0

		// tell the shader to blur horizontally and the size of 1 pixel
		mShaderBlur->uniform( "sample_offset", vec2( 1.0f / mFboBlur1->getWidth(), 0.0f ) );
		mShaderBlur->uniform( "attenuation", 2.5f );

		// copy a horizontally blurred version of our scene into the first blur Fbo
		{
			gl::ScopedFramebuffer fbo( mFboBlur1 );
			gl::ScopedViewport    viewport( 0, 0, mFboBlur1->getWidth(), mFboBlur1->getHeight() );

			gl::ScopedTextureBind tex0( mFboScene->getColorTexture(), (uint8_t)0 );

			gl::setMatricesWindow( BLUR_SIZE, BLUR_SIZE );
			gl::clear( Color::black() );

			gl::drawSolidRect( mFboBlur1->getBounds() );
		}

		// tell the shader to blur vertically and the size of 1 pixel
		mShaderBlur->uniform( "sample_offset", vec2( 0.0f, 1.0f / mFboBlur2->getHeight() ) );
		mShaderBlur->uniform( "attenuation", 2.5f );

		// copy a vertically blurred version of our blurred scene into the second blur Fbo
		{
			gl::ScopedFramebuffer fbo( mFboBlur2 );
			gl::ScopedViewport    viewport( 0, 0, mFboBlur2->getWidth(), mFboBlur2->getHeight() );

			gl::ScopedTextureBind tex0( mFboBlur1->getColorTexture(), (uint8_t)0 );

			gl::setMatricesWindow( BLUR_SIZE, BLUR_SIZE );
			gl::clear( Color::black() );

			gl::drawSolidRect( mFboBlur2->getBounds() );
		}
	}

	// render scene into mFboScene using color texture
	{
		gl::ScopedFramebuffer fbo( mFboScene );
		gl::ScopedViewport    viewport( 0, 0, mFboScene->getWidth(), mFboScene->getHeight() );

		gl::ScopedTextureBind tex0( mTextureColor, (uint8_t)0 );
		gl::ScopedTextureBind tex1( mTextureSpecular, (uint8_t)1 );

		gl::setMatricesWindow( SCENE_SIZE, SCENE_SIZE );
		gl::clear( Color::black() );

		render();
	}

	gl::popMatrices();

	// draw the 3 Fbo's
	gl::color( Color::white() );
	gl::draw( mFboScene->getColorTexture(), Rectf( 0, 0, 400, 400 ) );
	drawStrokedRect( Rectf( 0, 0, 400, 400 ) );

	gl::draw( mFboBlur1->getColorTexture(), Rectf( 0, 400, 0 + 400, 400 + 400 ) );
	drawStrokedRect( Rectf( 0, 400, 0 + 400, 400 + 400 ) );

	gl::draw( mFboBlur2->getColorTexture(), Rectf( 400, 400, 400 + 400, 400 + 400 ) );
	drawStrokedRect( Rectf( 400, 400, 400 + 400, 400 + 400 ) );

	// draw our scene with the blurred version added as a blend
	gl::color( Color::white() );
	gl::draw( mFboScene->getColorTexture(), Rectf( 400, 0, 400 + 400, 0 + 400 ) );

	gl::enableAdditiveBlending();
	gl::draw( mFboBlur2->getColorTexture(), Rectf( 400, 0, 400 + 400, 0 + 400 ) );
	gl::disableAlphaBlending();
	drawStrokedRect( Rectf( 400, 0, 400 + 400, 0 + 400 ) );

	// draw info
	gl::enableAlphaBlending();
	gl::draw( mTextureArrows, vec2( 300, 300 ) );
	gl::drawStringCentered( "Basic Scene", vec2( 200, 370 ), Color::white(), mFont );
	gl::drawStringCentered( "First Blur Pass (Horizontal)", vec2( 200, 400 + 370 ), Color::white(), mFont );
	gl::drawStringCentered( "Second Blur Pass (Vertical)", vec2( 400 + 200, 400 + 370 ), Color::white(), mFont );
	gl::drawStringCentered( "Final Composite", vec2( 400 + 200, 0 + 370 ), Color::white(), mFont );
	gl::disableAlphaBlending();
}

void BloomingNeonApp::render()
{
	// get the current viewport
	auto viewport = gl::getViewport();

	// adjust the aspect ratio of the camera
	mCamera.setAspectRatio( viewport.second.x / (float)viewport.second.y );

	// render our scene (see the Picking3D sample for more info)
	gl::pushMatrices();

	gl::setMatrices( mCamera );
	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::ScopedGlslProg shader( mShaderPhong );
	mShaderPhong->uniform( "tex_diffuse", 0 );
	mShaderPhong->uniform( "tex_specular", 1 );

	gl::multModelMatrix( mTransform );
	gl::color( Color::white() );

	mBatch->draw();

	gl::disableDepthWrite();
	gl::disableDepthRead();

	gl::popMatrices();
}

void BloomingNeonApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	}
}

void BloomingNeonApp::drawStrokedRect( const Rectf &rect )
{
	// we don't want any texture on our lines
	glDisable( GL_TEXTURE_2D );

	gl::drawLine( rect.getUpperLeft(), rect.getUpperRight() );
	gl::drawLine( rect.getUpperRight(), rect.getLowerRight() );
	gl::drawLine( rect.getLowerRight(), rect.getLowerLeft() );
	gl::drawLine( rect.getLowerLeft(), rect.getUpperLeft() );
}

CINDER_APP( BloomingNeonApp, RendererGl( RendererGl::Options().msaa( 16 ) ), &BloomingNeonApp::prepare )