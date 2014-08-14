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
#include "cinder/ImageIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/ObjLoader.h"
#include "cinder/Rand.h"
#include "cinder/TriMesh.h"
#include "cinder/Utilities.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define NUM_INSTANCES		9600 
#define INSTANCES_PER_ROW	60

class HexagonMirrorApp : public AppBasic {
public:
	void prepareSettings( Settings *settings );
	
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
	MayaCamUI		mCamera;

	// our shader with instanced rendering support
	gl::GlslProg	mShaderInstanced;

	// VBO containing one hexagon mesh
	gl::VboMesh		mVboMesh;

	// a VBO containing a list of matrices, 
	// that we can apply as a vertex shader attribute
	gl::Vbo			mBuffer;
	// a vertex array object that encapsulates our buffer
	GLuint			mVAO;

	CaptureRef		mCapture;
	gl::Texture		mCaptureTexture;
};

void HexagonMirrorApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize( 800, 600 );
	settings->setTitle("Hexagon Mirror");
	settings->setFrameRate( 500.0f );
}

void HexagonMirrorApp::setup()
{
	// initialize camera
	CameraPersp	cam;
	cam.setEyePoint( Vec3f(90, 70, 90) );
	cam.setCenterOfInterestPoint( Vec3f(90, 70, 0) );
	cam.setFov( 60.0f );
	mCamera.setCurrentCam( cam );

	// load shader
	try { mShaderInstanced = gl::GlslProg( loadAsset("phong.vert"), loadAsset("phong.frag") ); }
	catch( const std::exception &e ) { console() << "Could not load and compile shader: " << e.what() << std::endl; }

	// create a vertex array object, which allows us to efficiently position each instance
	initializeBuffer();

	// load hexagon mesh
	loadMesh();

	// connect to a webcam
	try { 
		mCapture = Capture::create( 160, 120 ); 
		mCapture->start();
	}
	catch( const std::exception &e ) { 
		console() << "Could not connect to webcam: " << e.what() << std::endl;

		try { mCaptureTexture = loadImage( loadAsset("placeholder.png") ); }
		catch( const std::exception& ) { }
	}
}

void HexagonMirrorApp::update()
{
	// update webcam image
	if( mCapture && mCapture->checkNewFrame() )
		mCaptureTexture = gl::Texture( mCapture->getSurface() );
}

void HexagonMirrorApp::draw()
{
	// clear the window
	gl::clear();

	// activate our camera
	gl::pushMatrices();
	gl::setMatrices( mCamera.getCamera() );

	// set render states
	gl::enable( GL_CULL_FACE );
	gl::enableDepthRead();
	gl::enableDepthWrite();
	gl::color( Color::white() );

	if( mVboMesh && mShaderInstanced && mBuffer ) 
	{
		// bind webcam image
		if( mCaptureTexture )
			mCaptureTexture.bind(0);

		// bind the shader, which will do all the hard work for us
		mShaderInstanced.bind();
		mShaderInstanced.uniform( "texture", 0 );
		mShaderInstanced.uniform( "scale", Vec2f( 1.0f / (3.0f * INSTANCES_PER_ROW), 1.0f / (2.25f * INSTANCES_PER_ROW) ) );

		// bind the buffer containing the model matrix for each instance,
		// this will allow us to pass this information as a vertex shader attribute.
		// See: initializeBuffer()
		glBindVertexArray(mVAO);

		// we do all positioning in the shader, and therefor we only need 
		// a single draw call to render all instances.
		drawInstanced( mVboMesh, NUM_INSTANCES );

		// make sure our VBO is no longer bound
		mVboMesh.unbindBuffers();
			
		// unbind vertex array object containing our buffer
		glBindVertexArray(0);

		// unbind shader
		mShaderInstanced.unbind();

		if( mCaptureTexture )
			mCaptureTexture.unbind();
	}			
	
	// reset render states
	gl::disableDepthWrite();
	gl::disableDepthRead();
	gl::disable( GL_CULL_FACE );

	// restore 2D drawing
	gl::popMatrices();
}

void HexagonMirrorApp::loadMesh()
{
	ObjLoader	loader( loadAsset("hexagon.obj") );
	TriMesh		mesh;

	try {
		loader.load( &mesh, true, false, false );
		mVboMesh = gl::VboMesh( mesh );
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
	std::vector< Matrix44f > matrices;
	matrices.reserve( NUM_INSTANCES );

	for(size_t i=0;i<NUM_INSTANCES;++i)
	{
		// determine position for this hexagon
		float x = math<float>::fmod( float(i), INSTANCES_PER_ROW );
		float y = math<float>::floor( float(i) / INSTANCES_PER_ROW );

		// create transform matrix, then rotate and translate it
		Matrix44f model;
		model.translate( Vec3f( 3.0f * x + 1.5f * math<float>::fmod( y, 2.0f ) , 0.866025f * y, 0.0f ) );
		matrices.push_back( model );
	}

	// retrieve attribute location from the shader
	// (note: make sure the shader is loaded and compiled before calling this function)
	GLint ulocation = mShaderInstanced.getAttribLocation( "model_matrix" );

	// if found...
	if( ulocation != -1 )
	{ 
		// create vertex array object to hold our buffer
		// (note: this is required for OpenGL 3.1 and above)
		glGenVertexArrays(1, &mVAO);   
		glBindVertexArray(mVAO);

		// create array buffer to store model matrices
		mBuffer = gl::Vbo( GL_ARRAY_BUFFER );

		// setup the buffer to contain space for all matrices.
		// we need 4 attributes to contain the 16 floats of a single matrix,
		// because the maximum size of an attribute is 4 floats (16 bytes).
		// When adding a 'mat4' attribute, OpenGL will make sure that 
		// the attribute locations are sequential, e.g.: 3, 4, 5 and 6
		mBuffer.bind();
		for (unsigned int i = 0; i < 4 ; i++) {
			glEnableVertexAttribArray(ulocation + i);
			glVertexAttribPointer( ulocation + i, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix44f), (const GLvoid*) (4 * sizeof(GLfloat) * i) );

			// get the next matrix after each instance, instead of each vertex
			glVertexAttribDivisor( ulocation + i, 1 );
		}

		// fill the buffer with our data
		mBuffer.bufferData( matrices.size() * sizeof(Matrix44f), &matrices.front(), GL_STATIC_READ );
		mBuffer.unbind();

		// unbind the VAO
		glBindVertexArray(0);
	}
}

void HexagonMirrorApp::resize()
{
	// adjust the camera aspect ratio
	CameraPersp cam = mCamera.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mCamera.setCurrentCam( cam );
}

void HexagonMirrorApp::mouseMove( MouseEvent event )
{
}

void HexagonMirrorApp::mouseDown( MouseEvent event )
{
	mCamera.mouseDown( event.getPos() );
}

void HexagonMirrorApp::mouseDrag( MouseEvent event )
{
	mCamera.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void HexagonMirrorApp::mouseUp( MouseEvent event )
{
}

void HexagonMirrorApp::keyDown( KeyEvent event )
{

	switch( event.getCode() )
	{
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_f:	{
		// toggle full screen
		bool wasVerticalSynced = gl::isVerticalSyncEnabled();
		setFullScreen( ! isFullScreen() );
		gl::enableVerticalSync( wasVerticalSynced );

		// when switching to/from full screen, the buffer or shader might be lost
		initializeBuffer();	}
		break;
	case KeyEvent::KEY_v:
		gl::enableVerticalSync( ! gl::isVerticalSyncEnabled() );
		break;
	}
}

void HexagonMirrorApp::keyUp( KeyEvent event )
{
}

///

void HexagonMirrorApp::drawInstanced( const gl::VboMesh &vbo, size_t instanceCount )
{
	if( vbo.getNumIndices() > 0 )
		drawRangeInstanced( vbo, (size_t)0, vbo.getNumIndices(), instanceCount );
	else
		drawArraysInstanced( vbo, 0, vbo.getNumVertices(), instanceCount );
}

void HexagonMirrorApp::drawRangeInstanced( const gl::VboMesh &vbo, size_t startIndex, size_t indexCount, size_t instanceCount )
{
	if( vbo.getNumIndices() <= 0 )
		return;

	vbo.enableClientStates();
	vbo.bindAllData();

#if( defined GLEE_ARB_draw_instanced )
	glDrawElementsInstancedARB( vbo.getPrimitiveType(), indexCount, GL_UNSIGNED_INT, (GLvoid*)( sizeof(uint32_t) * startIndex ), instanceCount );
#elif( defined GLEE_EXT_draw_instanced )
	glDrawElementsInstancedEXT( vbo.getPrimitiveType(), indexCount, GL_UNSIGNED_INT, (GLvoid*)( sizeof(uint32_t) * startIndex ), instanceCount );
#else
	// fall back to rendering a single instance
	glDrawElements( vbo.getPrimitiveType(), indexCount, GL_UNSIGNED_INT, (GLvoid*)( sizeof(uint32_t) * startIndex ) );
#endif

	gl::VboMesh::unbindBuffers();
	vbo.disableClientStates();
}

void HexagonMirrorApp::drawArraysInstanced( const gl::VboMesh &vbo, GLint first, GLsizei count, size_t instanceCount )
{
	vbo.enableClientStates();
	vbo.bindAllData();

#if( defined GLEE_ARB_draw_instanced )
	glDrawArraysInstancedARB( vbo.getPrimitiveType(), first, count, instanceCount );
#elif( defined GLEE_EXT_draw_instanced )
	glDrawArraysInstancedEXT( vbo.getPrimitiveType(), first, count, instanceCount );
#else
	// fall back to rendering a single instance
	glDrawArrays( vbo.getPrimitiveType(), first, count );
#endif

	gl::VboMesh::unbindBuffers();
	vbo.disableClientStates();
}

CINDER_APP_BASIC( HexagonMirrorApp, RendererGl )
