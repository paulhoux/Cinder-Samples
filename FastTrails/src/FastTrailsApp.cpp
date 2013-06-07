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
#include "cinder/MayaCamUI.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"

#include <deque>

using namespace ci;
using namespace ci::app;
using namespace std;

#define TRAIL_LENGTH	16000

class FastTrailsApp : public AppBasic {
public:
	void prepareSettings( Settings *settings );
	
	void setup();
	void update();
	void draw();
	
	void resize();
	
	void mouseDown( MouseEvent event );	
	void mouseDrag( MouseEvent event );	
	
	void keyDown( KeyEvent event );
private:
private:
	MayaCamUI			mCamera;

	gl::Texture			mTexture;

	//gl::GlslProg		mShader;

	gl::VboMesh			mVboMesh;

	std::deque< Vec3f >	mTrail;

	double				mTime;
	float				mAngle;
};

void FastTrailsApp::prepareSettings(Settings *settings)
{
	settings->setFrameRate( 500.0f );
	settings->setTitle("Fast Trails Sample");
}

void FastTrailsApp::setup()
{
	// initialize camera
	CameraPersp cam( getWindowWidth(), getWindowHeight(), 60.0f, 0.1f, 500.0f );
	cam.setEyePoint( Vec3f(0, 0, -100.0f) );
	cam.setCenterOfInterestPoint( Vec3f::zero() );

	mCamera.setCurrentCam( cam );

	// load texture
	try { mTexture = gl::Texture( loadImage( loadAsset("gradient.png") ) ); }
	catch( const std::exception &e ) { console() << e.what() << std::endl; }

	// create VBO mesh
	gl::VboMesh::Layout layout;
	layout.setDynamicPositions();
	layout.setStaticIndices();
	layout.setStaticTexCoords2d();

	mVboMesh = gl::VboMesh( TRAIL_LENGTH, TRAIL_LENGTH, layout, GL_TRIANGLE_STRIP );

	// observation: indices and texture coordinates never change
	std::vector< uint32_t >	indices;	indices.reserve( TRAIL_LENGTH );
	std::vector< Vec2f >	texcoords;	texcoords.reserve( TRAIL_LENGTH );
	for( size_t i=0; i<TRAIL_LENGTH; ++i ) {
		indices.push_back( i );

		float x = math<float>::floor( i * 0.5f ) / ( TRAIL_LENGTH * 0.5f );
		float y = float( i % 2 );
		texcoords.push_back( Vec2f( x, y ) );
	}

	// create index and texture coordinate buffers
	mVboMesh.bufferIndices( indices );
	mVboMesh.bufferTexCoords2d( 0, texcoords );

	// clear our trail buffer
	mTrail.clear();
	
	// initialize time and angle
	mTime = getElapsedSeconds();
	mAngle= 0.0f;

	// disable vertical sync, so we can see the actual frame rate
	gl::disableVerticalSync();
}

void FastTrailsApp::update()
{
	// find out how many trails we should add
	const double	trails_per_second = 2000.0;
	double			elapsed = getElapsedSeconds() - mTime;
	uint32_t		num_trails = uint32_t(elapsed * trails_per_second);
	
	// add this number of trails 
	// (note: it's an ugly function that draws a swirling trail around a sphere, just for demo purposes)
	for(size_t i=0; i<num_trails; ++i ) {
		float		phi = mAngle * 0.01f;
		float		prev_phi = phi - 0.01f;
		float		theta = phi * 0.03f;
		float		prev_theta = prev_phi * 0.03f;

		Vec3f		pos = 45.0f * Vec3f( sinf( phi ) * cosf( theta ), sinf( phi ) * sinf( theta ), cosf( phi ) );
		Vec3f		prev_pos = 45.0f * Vec3f( sinf( prev_phi ) * cosf( prev_theta ), sinf( prev_phi ) * sinf( prev_theta ), cosf( prev_phi ) );

		Vec3f		direction = pos - prev_pos;
		Vec3f		right = Vec3f( sinf( 20.0f * phi ), 0.0f, cosf( 20.0f * phi ) );
		Vec3f		normal = direction.cross( right ).normalized();

		// add two vertices, one at each side of the center line
		mTrail.push_front( pos - 1.0f * normal );
		mTrail.push_front( pos + 1.0f * normal );

		mAngle += 1.0;
	}

	// keep trail length within bounds
	while( mTrail.size() > TRAIL_LENGTH )
		mTrail.pop_back();

	// copy to trail to vbo (there's probably a faster way than this, need to check that out later)
	gl::VboMesh::VertexIter itr = mVboMesh.mapVertexBuffer();
	for( size_t i=0; i<mTrail.size(); ++i, ++itr )
		itr.setPosition( mTrail[i] );

	// advance time
	mTime += num_trails / trails_per_second;
}

void FastTrailsApp::draw()
{
	// clear window
	gl::clear(); 

	// set render states
	gl::enableWireframe();
	gl::enableAlphaBlending();

	gl::enableDepthRead();
	gl::enableDepthWrite();

	// enable 3D camera
	gl::pushMatrices();
	gl::setMatrices( mCamera.getCamera() );

	// draw VBO mesh using texture
	mTexture.enableAndBind();
	gl::drawRange( mVboMesh, 0, mTrail.size(), 0, mTrail.size()-1 );
	mTexture.unbind();

	// restore camera and render states
	gl::popMatrices();

	gl::disableDepthWrite();
	gl::disableDepthRead();

	gl::disableAlphaBlending();
	gl::disableWireframe();
}

void FastTrailsApp::resize()
{
	// adjust aspect ratio
	CameraPersp cam = mCamera.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mCamera.setCurrentCam( cam );
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

	switch( event.getCode() )
	{
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_f:
		// switch to/from full screen while preserving vertical sync state
		isVerticalSyncEnabled = gl::isVerticalSyncEnabled();
		setFullScreen( ! isFullScreen() );
		gl::enableVerticalSync( isVerticalSyncEnabled );
		break;
	case KeyEvent::KEY_v:
		gl::enableVerticalSync( ! gl::isVerticalSyncEnabled() );
		break;
	}
}

CINDER_APP_BASIC( FastTrailsApp, RendererGl )
