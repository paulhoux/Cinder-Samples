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

#include "cinder/CameraUi.h"
#include "cinder/ImageIo.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TransparentMeshApp : public App {
  public:
	static void prepare( Settings *settings );

	void setup() override;
	void update() override;
	void draw() override;

	void resize() override;

	void mouseMove( MouseEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;

	void keyDown( KeyEvent event ) override;
	void keyUp( KeyEvent event ) override;

  private:
	// our textures
	gl::Texture2dRef mTexEarth;
	gl::Texture2dRef mTexBackground;

	// our shader
	gl::GlslProgRef mShader;

	// animate the Earth by rotating it around y-axis
	float mDegrees;

	// our controlable 3D camera
	CameraPersp mCamera;
	CameraUi    mCameraUi;
};

void TransparentMeshApp::prepare( Settings *settings )
{
	settings->setTitle( "Transparent Mesh Sample" );
}

void TransparentMeshApp::setup()
{
	// load our textures and shader
	try {
		// by enabling mip-mapping, we can reduce aliasing near the edges of the sphere
		gl::Texture2d::Format fmt;
		fmt.enableMipmapping( true );
		fmt.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );

		mTexEarth = gl::Texture::create( loadImage( loadAsset( "earth.png" ) ), fmt );
		mTexBackground = gl::Texture::create( loadImage( loadAsset( "background.png" ) ) );

		mShader = gl::context()->getStockShader( gl::ShaderDef().color().texture() );
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
		quit();
	}

	// initialize rotation angle
	mDegrees = 0.0f;

	// intialize 3D camera
	mCamera.setPerspective( 60.0f, 1.0f, 0.1f, 1000.0f );
	mCamera.lookAt( vec3( 0, 150, -200 ), vec3( 0 ) );

	mCameraUi.setCamera( &mCamera );
}

void TransparentMeshApp::update()
{
	// rotate the Earth
	mDegrees += glm::radians( 0.25f );
}

void TransparentMeshApp::draw()
{
	gl::clear();

	// first draw a 2D background
	gl::ScopedColor color( Color::white() );
	gl::draw( mTexBackground, getWindowBounds() );

	// next, enable the depth buffer and camera so we can render in 3D
	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::pushMatrices();
	gl::setMatrices( mCamera );

	// rotate the coordinate system, so the earth rotates over time
	gl::rotate( mDegrees, 0, 1, 0 );

	// bind the shader and earth texture (containing transparency) and enable alpha blending
	gl::ScopedGlslProg    shader( mShader );
	gl::ScopedTextureBind tex0( mTexEarth );
	gl::ScopedBlendAlpha  blend;

	// the trick with rendering transparent objects, is to render from back to front.
	// For a single mesh, this is relatively easy: first, only draw all back-facing polygons,
	// then draw all front-facing polygons. This is where face-culling comes in.
	gl::ScopedFaceCulling culling( true );

	// draw all back-facing polygons first (in a slightly darker color)
	gl::cullFace( GL_FRONT );
	gl::color( Color( 0.75f, 0.75f, 0.75f ) );
	gl::drawSphere( vec3( 0 ), 100.0f, 60 );

	// draw all front-facing polygons next (in their original color)
	gl::cullFace( GL_BACK );
	gl::color( Color( 1, 1, 1 ) );
	gl::drawSphere( vec3( 0 ), 100.0f, 60 );

	// restore all the other render states
	gl::popMatrices();
	gl::disableDepthWrite();
	gl::disableDepthRead();
}

void TransparentMeshApp::resize()
{
	// update the camera's aspect ratio
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

void TransparentMeshApp::mouseMove( MouseEvent event ) {}

void TransparentMeshApp::mouseDown( MouseEvent event )
{
	// let the user control our camera
	mCameraUi.mouseDown( event.getPos() );
}

void TransparentMeshApp::mouseDrag( MouseEvent event )
{
	// let the user control our camera
	mCameraUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void TransparentMeshApp::mouseUp( MouseEvent event ) {}

void TransparentMeshApp::keyDown( KeyEvent event ) {}

void TransparentMeshApp::keyUp( KeyEvent event ) {}

CINDER_APP( TransparentMeshApp, RendererGl( RendererGl::Options().msaa( 16 ) ), &TransparentMeshApp::prepare )
