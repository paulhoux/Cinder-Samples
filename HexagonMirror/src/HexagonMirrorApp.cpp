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

#include "cinder/Capture.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"
#include "cinder/ImageIo.h"
#include "cinder/ObjLoader.h"
#include "cinder/Rand.h"
#include "cinder/TriMesh.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define NUM_INSTANCES 9600
#define INSTANCES_PER_ROW 60

class HexagonMirrorApp : public App {
  public:
	static void prepare( Settings *settings );

	void setup();
	void update();
	void draw();

	void resize();

	void mouseMove( MouseEvent event );
	void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );
	void mouseUp( MouseEvent event );

	void keyDown( KeyEvent event );
	void keyUp( KeyEvent event );

  private:
	// the following functions may become part of Cinder's gl:: namespace in the future,
	// but I've included them here so you can link against Cinder v0.8.4
	void drawInstanced( const gl::VboMesh &vbo, size_t instanceCount );
	void drawRangeInstanced( const gl::VboMesh &vbo, size_t startIndex, size_t indexCount, size_t instanceCount );
	void drawArraysInstanced( const gl::VboMesh &vbo, GLint first, GLsizei count, size_t instanceCount );

	// loads the hexagon mesh into a VBO using ObjLoader
	void loadMesh();
	// creates a Vertex Array Object containing a transform matrix for each instance
	void initializeBuffer();

  private:
	// our controlable camera
	CameraPersp mCamera;
	CameraUi    mCameraUi;

	// our shader with instanced rendering support
	gl::GlslProgRef mShader;
	// VBO containing one hexagon mesh
	gl::VboMeshRef mVboMesh;
	// Batch that combines the mesh and shader
	gl::BatchRef mBatch;

	// a VBO containing a list of matrices, one for every instance
	gl::VboRef mInstanceDataVbo;

	CaptureRef       mCapture;
	gl::Texture2dRef mCaptureTexture;

	gl::Texture2dRef mDummyTexture;
};

void HexagonMirrorApp::prepare( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
	settings->setTitle( "Hexagon Mirror" );
	settings->setFrameRate( 500.0f );
}

void HexagonMirrorApp::setup()
{
	// initialize camera
	mCamera.setPerspective( 20.0f, 1.0f, 1.0f, 5000.0f );
	mCamera.lookAt( vec3( 90, 69, 380 ), vec3( 90, 69, 0 ) );
	mCameraUi.setCamera( &mCamera );

	// load shader
	try {
		mShader = gl::GlslProg::create( loadAsset( "phong.vert" ), loadAsset( "phong.frag" ) );
	}
	catch( const std::exception &e ) {
		console() << "Could not load and compile shader: " << e.what() << std::endl;
	}

	// load hexagon mesh
	loadMesh();

	// create a vertex array object, which allows us to efficiently position each instance
	initializeBuffer();

	// connect to a webcam
	try {
		mCapture = Capture::create( 320, 240 );
		mCapture->start();
	}
	catch( const std::exception &e ) {
		console() << "Could not connect to webcam: " << e.what() << std::endl;
	}

	// load a dummy texture in case we have no webcam
	try {
		mCaptureTexture = mDummyTexture = gl::Texture2d::create( loadImage( loadAsset( "placeholder.png" ) ) );
	}
	catch( const std::exception & ) {
	}
}

void HexagonMirrorApp::update()
{
	// update webcam image
	if( mCapture && mCapture->checkNewFrame() )
		mCaptureTexture = gl::Texture2d::create( *mCapture->getSurface() );
}

void HexagonMirrorApp::draw()
{
	// clear the window
	gl::clear();

	// activate our camera
	gl::pushMatrices();
	gl::setMatrices( mCamera );

	// set render states
	gl::ScopedFaceCulling cull( true );
	gl::ScopedDepth       depth( true, true );
	gl::ScopedColor       color( 1, 0.8f, 0.6f );

	if( mVboMesh && mShader && mInstanceDataVbo && mCaptureTexture ) {
		// bind webcam image to texture unit 0
		gl::ScopedTextureBind tex0( mCaptureTexture, 0 );

		// bind the shader, which will do all the hard work for us
		gl::ScopedGlslProg shader( mShader );
		mShader->uniform( "uTexture", 0 );
		mShader->uniform( "uScale", vec2( 1.0f / ( 3.0f * INSTANCES_PER_ROW ), 1.0f / ( 2.25f * INSTANCES_PER_ROW ) ) );

		// we do all positioning in the shader, and therefor we only need
		// a single draw call to render all instances.
		mBatch->drawInstanced( NUM_INSTANCES );
	}

	// restore 2D drawing
	gl::popMatrices();
}

void HexagonMirrorApp::loadMesh()
{
	ObjLoader loader( loadAsset( "hexagon.obj" ) );

	try {
		TriMeshRef mesh = TriMesh::create( loader );
		mVboMesh = gl::VboMesh::create( *mesh );
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
	}
}

void HexagonMirrorApp::initializeBuffer()
{
	// To pass the model matrix for each instance, we will use
	// a custom vertex shader attribute. Using an attribute will
	// give us more freedom and more storage capacity than using
	// a uniform buffer, which often has a capacity limit of 64KB
	// or less, which is just enough for 1024 instances.
	//
	// Because we want the attribute to stay the same for all
	// vertices of an instance, we set the divisor to '1' instead
	// of '0', so the attribute will only change after all
	// vertices of the instance have been drawn.
	//
	// See for more information: http://ogldev.atspace.co.uk/www/tutorial33/tutorial33.html

	// initialize transforms for every instance
	std::vector<mat4> matrices;
	matrices.reserve( NUM_INSTANCES );

	for( size_t i = 0; i < NUM_INSTANCES; ++i ) {
		// determine position for this hexagon
		float x = math<float>::fmod( float( i ), INSTANCES_PER_ROW );
		float y = math<float>::floor( float( i ) / INSTANCES_PER_ROW );

		// create transform matrix, then rotate and translate it
		mat4 model = glm::translate( vec3( 3.0f * x + 1.5f * math<float>::fmod( y, 2.0f ), 0.866025f * y, 0.0f ) );
		matrices.push_back( model );
	}

	// create array buffer to store model matrices
	mInstanceDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, matrices.size() * sizeof( mat4 ), matrices.data(), GL_STATIC_DRAW );

	// setup the buffer to contain space for all matrices. Each matrix needs 16 floats.
	geom::BufferLayout instanceDataLayout;
	instanceDataLayout.append( geom::Attrib::CUSTOM_0, 16, sizeof( mat4 ), 0, 1 /* per instance */ );

	// add buffer to mesh and create batch
	mVboMesh->appendVbo( instanceDataLayout, mInstanceDataVbo );
	mBatch = gl::Batch::create( mVboMesh, mShader, { { geom::CUSTOM_0, "iModelMatrix" } } );
}

void HexagonMirrorApp::resize()
{
	// adjust the camera aspect ratio
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

void HexagonMirrorApp::mouseMove( MouseEvent event )
{
}

void HexagonMirrorApp::mouseDown( MouseEvent event )
{
	mCameraUi.mouseDown( event );
}

void HexagonMirrorApp::mouseDrag( MouseEvent event )
{
	mCameraUi.mouseDrag( event );
}

void HexagonMirrorApp::mouseUp( MouseEvent event )
{
}

void HexagonMirrorApp::keyDown( KeyEvent event )
{

	switch( event.getCode() ) {
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_f:
		// toggle full screen
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_v:
		gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
		break;
	}
}

void HexagonMirrorApp::keyUp( KeyEvent event )
{
}

CINDER_APP( HexagonMirrorApp, RendererGl( RendererGl::Options().msaa( 16 ) ), &HexagonMirrorApp::prepare )
