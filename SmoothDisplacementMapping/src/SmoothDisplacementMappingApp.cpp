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
#include "cinder/Surface.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class SmoothDisplacementMappingApp : public AppBasic {
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
	void createMesh();
	void createTextures();
	bool compileShaders();

	void renderDisplacementMap();
	void renderNormalMap();

	void resetCamera();
private:
	bool			mDrawTextures;
	bool			mDrawWireframe;
	bool			mDrawOriginalMesh;
	bool			mEnableShader;

	float			mAmplitude;
	float			mAmplitudeTarget;

	MayaCamUI		mMayaCam;
	CameraPersp		mCamera;

	gl::Fbo			mDispMapFbo;
	gl::GlslProg	mDispMapShader;

	gl::Fbo			mNormalMapFbo;
	gl::GlslProg	mNormalMapShader;
	
	gl::VboMesh		mVboMesh;
	gl::GlslProg	mMeshShader;
	
	gl::Texture		mBackgroundTexture;
	gl::GlslProg	mBackgroundShader;
};

void SmoothDisplacementMappingApp::prepareSettings(Settings *settings)
{
	settings->setTitle("Vertex Displacement Mapping with Smooth Normals");
	settings->setWindowSize( 1280, 720 );
	settings->setFrameRate( 500.0f );
}

void SmoothDisplacementMappingApp::setup()
{
	mDrawTextures = false;
	mDrawWireframe = false;
	mDrawOriginalMesh = false;
	mEnableShader = true;

	mAmplitude = 0.0f;
	mAmplitudeTarget = 10.0f;

	// initialize our camera
	resetCamera();

	// load and compile shaders
	if( ! compileShaders() ) quit();	

	// create the basic mesh (a flat plane)
	createMesh();
	// create the textures
	createTextures();

	// create the frame buffer objects for the displacement map and the normal map
	gl::Fbo::Format fmt;
	fmt.enableDepthBuffer(false);
	fmt.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );

	// use a single channel (red) for the displacement map
	fmt.setColorInternalFormat( GL_R32F );
	mDispMapFbo = gl::Fbo(256, 256, fmt);
	
	// use 3 channels (rgb) for the normal map
	fmt.setColorInternalFormat( GL_RGB32F );
	mNormalMapFbo = gl::Fbo(256, 256, fmt);
}

void SmoothDisplacementMappingApp::update()
{
	mAmplitude += 0.02f * (mAmplitudeTarget - mAmplitude);

	// render displacement map
	renderDisplacementMap();

	// render normal map
	renderNormalMap();
}

void SmoothDisplacementMappingApp::draw()
{
	gl::clear();

	// render background
	if( mBackgroundTexture && mBackgroundShader )
	{
		mBackgroundShader.bind();
		mBackgroundShader.uniform( "texture", 0 );
		mBackgroundShader.uniform( "hue", float( 0.025 * getElapsedSeconds() ) );
		gl::draw( mBackgroundTexture, getWindowBounds() );
		mBackgroundShader.unbind();
	}//*/

	// if enabled, show the displacement and normal maps 
	if(mDrawTextures) 
	{	
		gl::color( Color(0.05f, 0.05f, 0.05f) );
		gl::draw( mDispMapFbo.getTexture(), Vec2f(0,0) );
		gl::color( Color(1, 1, 1) );
		gl::draw( mNormalMapFbo.getTexture(), Vec2f(256,0) );
	}

	// setup the 3D camera
	gl::pushMatrices();
	gl::setMatrices( mCamera );

	// setup render states
	gl::enableAdditiveBlending();
	if(mDrawWireframe) 
		gl::enableWireframe();

	// draw undisplaced mesh if enabled
	if(mDrawOriginalMesh)
	{
		gl::color( ColorA(1, 1, 1, 0.2f) );
		gl::draw( mVboMesh );
	}

	if( mDispMapFbo && mNormalMapFbo && mMeshShader )
	{
		// bind the displacement and normal maps, each to their own texture unit
		mDispMapFbo.getTexture().bind(0);
		mNormalMapFbo.getTexture().bind(1);

		// render our mesh using vertex displacement
		mMeshShader.bind();
		mMeshShader.uniform( "displacement_map", 0 );
		mMeshShader.uniform( "normal_map", 1 );
		mMeshShader.uniform( "falloff_enabled", mEnableShader );

		gl::color( Color::white() );
		gl::draw( mVboMesh );

		mMeshShader.unbind();

		// unbind texture maps
		mNormalMapFbo.unbindTexture();
		mDispMapFbo.unbindTexture();
	}
	
	// clean up after ourselves
	gl::disableWireframe();
	gl::disableAlphaBlending();
	
	gl::popMatrices();//*/
}

void SmoothDisplacementMappingApp::resetCamera()
{
	mCamera.setEyePoint( Vec3f( 0.0f, 0.0f, 130.0f ) );
	mCamera.setCenterOfInterestPoint( Vec3f( 0.0f, 0.0f, 0.0f ) ) ;
	mMayaCam.setCurrentCam( mCamera );
}

void SmoothDisplacementMappingApp::renderDisplacementMap()
{
	if( mDispMapShader && mDispMapFbo ) 
	{
		mDispMapFbo.bindFramebuffer();
		{
			// clear the color buffer
			gl::clear();			

			// setup viewport and matrices 
			glPushAttrib( GL_VIEWPORT_BIT );
			gl::setViewport( mDispMapFbo.getBounds() );

			gl::pushMatrices();
			gl::setMatricesWindow( mDispMapFbo.getSize(), false );

			// render the displacement map
			mDispMapShader.bind();
			mDispMapShader.uniform( "time", float( getElapsedSeconds() ) );
			mDispMapShader.uniform( "amplitude", mAmplitude );
			gl::drawSolidRect( mDispMapFbo.getBounds() );
			mDispMapShader.unbind();

			// clean up after ourselves
			gl::popMatrices();
			glPopAttrib();
		}
		mDispMapFbo.unbindFramebuffer();
	}
}

void SmoothDisplacementMappingApp::renderNormalMap()
{
	if( mNormalMapShader && mNormalMapFbo ) 
	{
		mNormalMapFbo.bindFramebuffer();
		{
			// setup viewport and matrices 
			glPushAttrib( GL_VIEWPORT_BIT );
			gl::setViewport( mNormalMapFbo.getBounds() );

			gl::pushMatrices();
			gl::setMatricesWindow( mNormalMapFbo.getSize(), false );

			// clear the color buffer
			gl::clear();			

			// bind the displacement map
			mDispMapFbo.getTexture().bind(0);

			// render the normal map
			mNormalMapShader.bind();
			mNormalMapShader.uniform( "texture", 0 );
			mNormalMapShader.uniform( "amplitude", 4.0f );

			Area bounds = mNormalMapFbo.getBounds(); //bounds.expand(-1, -1);
			gl::drawSolidRect( bounds );

			mNormalMapShader.unbind();

			// clean up after ourselves
			mDispMapFbo.getTexture().unbind();

			gl::popMatrices();

			glPopAttrib();
		}
		mNormalMapFbo.unbindFramebuffer();
	}
}

bool SmoothDisplacementMappingApp::compileShaders()
{	
	try 
	{ 
		// this shader will render all colors using a change in hue
		mBackgroundShader = gl::GlslProg( loadAsset("background.vert"), loadAsset("background.frag") );
		// this shader will render a displacement map to a floating point texture, updated every frame
		mDispMapShader = gl::GlslProg( loadAsset("displacement_map.vert"), loadAsset("displacement_map.frag") ); 
		// this shader will create a normal map based on the displacement map
		mNormalMapShader = gl::GlslProg( loadAsset("normal_map.vert"), loadAsset("normal_map.frag") );
		// this shader will use the displacement and normal maps to displace vertices of a mesh
		mMeshShader = gl::GlslProg( loadAsset("mesh.vert"), loadAsset("mesh.frag") );
	}
	catch( const std::exception &e ) 
	{ 
		console() << e.what() << std::endl; 
		return false;
	}

	return true;
}

void SmoothDisplacementMappingApp::resize()
{
	// if window is resized, update camera aspect ratio
	mCamera.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( mCamera );
}

void SmoothDisplacementMappingApp::mouseMove( MouseEvent event )
{
}

void SmoothDisplacementMappingApp::mouseDown( MouseEvent event )
{
	// handle user input
	mMayaCam.mouseDown( event.getPos() );
}

void SmoothDisplacementMappingApp::mouseDrag( MouseEvent event )
{
	// handle user input
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	mCamera = mMayaCam.getCamera();
}

void SmoothDisplacementMappingApp::mouseUp( MouseEvent event )
{
}

void SmoothDisplacementMappingApp::keyDown( KeyEvent event )
{
	switch( event.getCode() )
	{
	case KeyEvent::KEY_ESCAPE:
		// quit
		quit();
		break;
	case KeyEvent::KEY_f:
		// toggle full screen
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_m:
		// toggle original mesh
		mDrawOriginalMesh = !mDrawOriginalMesh;
		break;
	case KeyEvent::KEY_s:
		// reload shaders
		compileShaders();
		break;
	case KeyEvent::KEY_t:
		// toggle draw textures
		mDrawTextures = !mDrawTextures;
		break;
	case KeyEvent::KEY_v:
		// toggle vertical sync
		gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
		break;
	case KeyEvent::KEY_w:
		// toggle wire frame
		mDrawWireframe = !mDrawWireframe;
		break;
	case KeyEvent::KEY_SPACE:
		// reset camera
		resetCamera();
		break;
	case KeyEvent::KEY_a:
		if( mAmplitudeTarget < 10.0f )
			mAmplitudeTarget = 10.0f;
		else
			mAmplitudeTarget = 0.0f;
		break;
	case KeyEvent::KEY_q:
		mEnableShader = !mEnableShader;
		break;
	}
}

void SmoothDisplacementMappingApp::keyUp( KeyEvent event )
{
}

void SmoothDisplacementMappingApp::createMesh()
{
	// use the TriMesh class to easily construct the vertex buffer object
	TriMesh mesh;

	// create vertex, normal and texcoord buffers
	const int RES_X = 400;
	const int RES_Z = 100;
	const Vec3f size(200.0f, 1.0f, 50.0f);

	for(int x=0;x<RES_X;++x) {
		for(int z=0;z<RES_Z;++z) {
			float u = float(x) / RES_X;
			float v = float(z) / RES_Z;
			mesh.appendVertex( size * Vec3f( u - 0.5f , 0.0f, v - 0.5f ) );
			mesh.appendNormal( Vec3f::yAxis() );
			mesh.appendTexCoord( Vec2f( u, v ) );
		}
	}

	// create index buffer
	vector< uint32_t > indices;
	for(int x=0;x<RES_X-1;++x) {
		for(int z=0;z<RES_Z-1;++z) {
			uint32_t i = x * RES_Z + z;

			indices.push_back( i ); indices.push_back( i + 1 ); indices.push_back( i + RES_Z );
			indices.push_back( i + RES_Z );  indices.push_back( i + 1 ); indices.push_back( i + RES_Z + 1 );
		}
	}
	mesh.appendIndices( &indices.front(), indices.size() );

	// construct vertex buffer object
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticTexCoords2d();
	layout.setStaticIndices();
	layout.setStaticNormals();

	mVboMesh = gl::VboMesh( mesh, layout );
}

void SmoothDisplacementMappingApp::createTextures()
{
	try {
		mBackgroundTexture = gl::Texture( loadImage( loadAsset( "background.png") ) );
	}
	catch( const std::exception &e ) { 
		console() << "Could not load image: " << e.what() << std::endl;
	}
}

CINDER_APP_BASIC( SmoothDisplacementMappingApp, RendererGl )
