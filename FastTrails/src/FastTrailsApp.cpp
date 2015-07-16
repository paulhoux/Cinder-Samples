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
#include "cinder/CameraUi.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"

#include <deque>

using namespace ci;
using namespace ci::app;
using namespace std;

#define TRAIL_LENGTH	16000

class FastTrailsApp : public App {
public:
	static void prepare( Settings *settings );

	void setup();
	void update();
	void draw();

	void resize();

	void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );

	void keyDown( KeyEvent event );
private:
private:
	CameraPersp         mCam;
	CameraUi            mCamera;

	gl::Texture2dRef    mTexture;

	gl::VboMeshRef      mVboMesh;

	std::deque<vec3>    mTrail;

	double              mTime;
	float               mAngle;
};

void FastTrailsApp::prepare( Settings *settings )
{
	settings->disableFrameRate();
	settings->setTitle( "Fast Trails Sample" );
}

void FastTrailsApp::setup()
{
	// initialize camera
	mCam.setPerspective( 60.0f, getWindowAspectRatio(), 0.1f, 500.0f );
	mCam.lookAt( vec3( 0, 0, -100.0f ), vec3( 0 ) );

	mCamera.setCamera( &mCam );

	// load texture
	try { mTexture = gl::Texture2d::create( loadImage( loadAsset( "gradient.png" ) ) ); }
	catch( const std::exception &e ) { console() << e.what() << std::endl; }

	// observation: texture coordinates never change
	std::vector<vec2>     texcoords;	texcoords.reserve( TRAIL_LENGTH );
	for( size_t i = 0; i < TRAIL_LENGTH; ++i ) {
		float x = math<float>::floor( i * 0.5f ) / ( TRAIL_LENGTH * 0.5f );
		float y = float( i % 2 );
		texcoords.emplace_back( vec2( x, y ) );
	}

	// create VBO mesh
	gl::VboMesh::Layout layout;
	layout.usage( GL_STATIC_DRAW );
	layout.attrib( geom::Attrib::POSITION, 3 );
	layout.attrib( geom::Attrib::TEX_COORD_0, 2 );

	mVboMesh = gl::VboMesh::create( TRAIL_LENGTH, GL_TRIANGLE_STRIP, { layout } );
	mVboMesh->bufferAttrib( geom::Attrib::TEX_COORD_0, texcoords );

	// clear our trail buffer
	mTrail.clear();

	// initialize time and angle
	mTime = getElapsedSeconds();
	mAngle = 0.0f;

	// disable vertical sync, so we can see the actual frame rate
	gl::enableVerticalSync( false );
}

void FastTrailsApp::update()
{
	// find out how many trails we should add
	const double	trails_per_second = 2000.0;
	double			elapsed = getElapsedSeconds() - mTime;
	uint32_t		num_trails = uint32_t( elapsed * trails_per_second );

	// add this number of trails 
	// (note: it's an ugly function that draws a swirling trail around a sphere, just for demo purposes)
	for( size_t i = 0; i < num_trails; ++i ) {
		float		phi = mAngle * 0.01f;
		float		prev_phi = phi - 0.01f;
		float		theta = phi * 0.03f;
		float		prev_theta = prev_phi * 0.03f;

		vec3		pos = 45.0f * vec3( sinf( phi ) * cosf( theta ), sinf( phi ) * sinf( theta ), cosf( phi ) );
		vec3		prev_pos = 45.0f * vec3( sinf( prev_phi ) * cosf( prev_theta ), sinf( prev_phi ) * sinf( prev_theta ), cosf( prev_phi ) );

		vec3		direction = pos - prev_pos;
		vec3		right = vec3( sinf( 20.0f * phi ), 0.0f, cosf( 20.0f * phi ) );
		vec3		normal = glm::normalize( glm::cross( direction, right ) );

		// add two vertices, one at each side of the center line
		mTrail.push_front( pos - 1.0f * normal );
		mTrail.push_front( pos + 1.0f * normal );

		mAngle += 1.0;
	}

	// keep trail length within bounds
	while( mTrail.size() > TRAIL_LENGTH )
		mTrail.pop_back();

	// copy to trail to vbo
	auto mapped = mVboMesh->mapAttrib3f( geom::POSITION, true );
	for( auto &trail : mTrail ) {
		*mapped++ = trail;
	}
	mapped.unmap();

	// advance time
	mTime += num_trails / trails_per_second;
}

void FastTrailsApp::draw()
{
	// clear window
	gl::clear();

	// set render states
	gl::enableWireframe();

	gl::ScopedBlendAlpha blend;
	gl::ScopedDepth depth( true, true );

	// enable 3D camera
	gl::pushMatrices();
	gl::setMatrices( mCamera.getCamera() );

	// draw VBO mesh using texture
	gl::ScopedTextureBind tex0( mTexture );
	gl::ScopedGlslProg shader( gl::getStockShader( gl::ShaderDef().texture() ) );
	gl::draw( mVboMesh, 0, mTrail.size() );

	// restore camera and render states
	gl::popMatrices();
	gl::disableWireframe();
}

void FastTrailsApp::resize()
{
	// adjust aspect ratio
	mCam.setAspectRatio( getWindowAspectRatio() );
}

void FastTrailsApp::mouseDown( MouseEvent event )
{
	mCamera.mouseDown( event.getPos() );
}

void FastTrailsApp::mouseDrag( MouseEvent event )
{
	mCamera.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void FastTrailsApp::keyDown( KeyEvent event )
{
	bool isVerticalSyncEnabled;

	switch( event.getCode() ) {
		case KeyEvent::KEY_ESCAPE:
			quit();
			break;
		case KeyEvent::KEY_f:
			setFullScreen( !isFullScreen() );
			break;
		case KeyEvent::KEY_v:
			gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
			break;
	}
}

CINDER_APP( FastTrailsApp, RendererGl, &FastTrailsApp::prepare )
