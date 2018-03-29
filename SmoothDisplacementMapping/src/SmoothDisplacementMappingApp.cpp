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
#include "cinder/CameraUi.h"
#include "cinder/ImageIo.h"
#include "cinder/Surface.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class SmoothDisplacementMappingApp : public App {
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
	void createMesh();
	void createTextures();
	bool compileShaders();

	void renderDisplacementMap();
	void renderNormalMap();

	void resetCamera();

  private:
	float mAmplitude;
	float mAmplitudeTarget;

	CameraPersp mCamera;
	CameraUi    mCameraUi;

	gl::FboRef      mDispMapFbo;
	gl::GlslProgRef mDispMapShader;

	gl::FboRef      mNormalMapFbo;
	gl::GlslProgRef mNormalMapShader;

	gl::VboMeshRef  mVboMesh;
	gl::GlslProgRef mMeshShader;
	gl::BatchRef    mBatch;

	gl::Texture2dRef mBackgroundTexture;
	gl::GlslProgRef  mBackgroundShader;

	bool mDrawTextures;
	bool mDrawWireframe;
	bool mDrawOriginalMesh;
	bool mEnableShader;
};

void SmoothDisplacementMappingApp::prepare( Settings *settings )
{
	settings->setTitle( "Vertex Displacement Mapping with Smooth Normals" );
	settings->setWindowSize( 1280, 720 );
	settings->disableFrameRate();
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
	mCameraUi.setCamera( &mCamera );
	resetCamera();

	// load and compile shaders
	if( !compileShaders() )
		quit();

	// create the basic mesh (a flat plane)
	createMesh();

	// create the textures
	createTextures();

	// create the frame buffer objects for the displacement map and the normal map
	gl::Fbo::Format fmt;
	fmt.enableDepthBuffer( false );

	// use a single channel (red) for the displacement map
	fmt.setColorTextureFormat( gl::Texture2d::Format().wrap( GL_CLAMP_TO_EDGE ).internalFormat( GL_R32F ) );
	mDispMapFbo = gl::Fbo::create( 256, 256, fmt );

	// use 3 channels (rgb) for the normal map
	fmt.setColorTextureFormat( gl::Texture2d::Format().wrap( GL_CLAMP_TO_EDGE ).internalFormat( GL_RGB32F ) );
	mNormalMapFbo = gl::Fbo::create( 256, 256, fmt );
}

void SmoothDisplacementMappingApp::update()
{
	mAmplitude += 0.02f * ( mAmplitudeTarget - mAmplitude );

	// render displacement map
	renderDisplacementMap();

	// render normal map
	renderNormalMap();
}

void SmoothDisplacementMappingApp::draw()
{
	gl::clear();

	// render background
	if( mBackgroundTexture && mBackgroundShader ) {
		gl::ScopedTextureBind tex0( mBackgroundTexture );
		gl::ScopedGlslProg    shader( mBackgroundShader );
		mBackgroundShader->uniform( "uTex0", 0 );
		mBackgroundShader->uniform( "uHue", float( 0.025 * getElapsedSeconds() ) );
		gl::drawSolidRect( getWindowBounds() );
	}

	// if enabled, show the displacement and normal maps
	if( mDrawTextures ) {
		gl::color( Color( 0.05f, 0.05f, 0.05f ) );
		gl::draw( mDispMapFbo->getColorTexture(), vec2( 0 ) );
		gl::color( Color( 1, 1, 1 ) );
		gl::draw( mNormalMapFbo->getColorTexture(), vec2( 256, 0 ) );
	}

	// setup the 3D camera
	gl::pushMatrices();
	gl::setMatrices( mCamera );

	// setup render states
	gl::enableAdditiveBlending();
	if( mDrawWireframe )
		gl::enableWireframe();

	// draw undisplaced mesh if enabled
	if( mDrawOriginalMesh ) {
		gl::color( ColorA( 1, 1, 1, 0.2f ) );
		gl::draw( mVboMesh );
	}

	if( mDispMapFbo && mNormalMapFbo && mMeshShader ) {
		// bind the displacement and normal maps, each to their own texture unit
		gl::ScopedTextureBind tex0( mDispMapFbo->getColorTexture(), uint8_t( 0 ) );
		gl::ScopedTextureBind tex1( mNormalMapFbo->getColorTexture(), uint8_t( 1 ) );

		// render our mesh using vertex displacement
		gl::ScopedGlslProg shader( mMeshShader );
		mMeshShader->uniform( "uTexDisplacement", 0 );
		mMeshShader->uniform( "uTexNormal", 1 );
		mMeshShader->uniform( "uEnableFallOff", mEnableShader );

		gl::color( Color::white() );
		mBatch->draw();
	}

	// clean up after ourselves
	gl::disableWireframe();
	gl::disableAlphaBlending();

	gl::popMatrices();
}

void SmoothDisplacementMappingApp::resetCamera()
{
	mCamera.lookAt( vec3( 0.0f, 0.0f, 130.0f ), vec3( 0.0f, 0.0f, 0.0f ) );
}

void SmoothDisplacementMappingApp::renderDisplacementMap()
{
	if( mDispMapShader && mDispMapFbo ) {
		// bind frame buffer
		gl::ScopedFramebuffer fbo( mDispMapFbo );

		// setup viewport and matrices
		gl::ScopedViewport viewport( 0, 0, mDispMapFbo->getWidth(), mDispMapFbo->getHeight() );

		gl::pushMatrices();
		gl::setMatricesWindow( mDispMapFbo->getSize() );

		// clear the color buffer
		gl::clear();

		// render the displacement map
		gl::ScopedGlslProg shader( mDispMapShader );
		mDispMapShader->uniform( "uTime", float( getElapsedSeconds() ) );
		mDispMapShader->uniform( "uAmplitude", mAmplitude );

		gl::drawSolidRect( mDispMapFbo->getBounds() );

		// clean up after ourselves
		gl::popMatrices();
	}
}

void SmoothDisplacementMappingApp::renderNormalMap()
{
	if( mNormalMapShader && mNormalMapFbo ) {
		// bind frame buffer
		gl::ScopedFramebuffer fbo( mNormalMapFbo );

		// setup viewport and matrices
		gl::ScopedViewport viewport( 0, 0, mNormalMapFbo->getWidth(), mNormalMapFbo->getHeight() );

		gl::pushMatrices();
		gl::setMatricesWindow( mNormalMapFbo->getSize() );

		// clear the color buffer
		gl::clear();

		// bind the displacement map
		gl::ScopedTextureBind tex0( mDispMapFbo->getColorTexture() );

		// render the normal map
		gl::ScopedGlslProg shader( mNormalMapShader );
		mNormalMapShader->uniform( "uTex0", 0 );
		mNormalMapShader->uniform( "uAmplitude", 4.0f );

		const Area bounds = mNormalMapFbo->getBounds();
		gl::drawSolidRect( bounds );

		// clean up after ourselves
		gl::popMatrices();
	}
}

bool SmoothDisplacementMappingApp::compileShaders()
{
	try {
		// this shader will render all colors using a change in hue
		mBackgroundShader = gl::GlslProg::create( loadAsset( "background.vert" ), loadAsset( "background.frag" ) );
		// this shader will render a displacement map to a floating point texture, updated every frame
		mDispMapShader = gl::GlslProg::create( loadAsset( "displacement_map.vert" ), loadAsset( "displacement_map.frag" ) );
		// this shader will create a normal map based on the displacement map
		mNormalMapShader = gl::GlslProg::create( loadAsset( "normal_map.vert" ), loadAsset( "normal_map.frag" ) );
		// this shader will use the displacement and normal maps to displace vertices of a mesh
		mMeshShader = gl::GlslProg::create( loadAsset( "mesh.vert" ), loadAsset( "mesh.frag" ) );
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
		return false;
	}

	return true;
}

void SmoothDisplacementMappingApp::resize()
{
	// if window is resized, update camera aspect ratio
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

void SmoothDisplacementMappingApp::mouseMove( MouseEvent event ) {}

void SmoothDisplacementMappingApp::mouseDown( MouseEvent event )
{
	// handle user input
	mCameraUi.mouseDown( event.getPos() );
}

void SmoothDisplacementMappingApp::mouseDrag( MouseEvent event )
{
	// handle user input
	mCameraUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void SmoothDisplacementMappingApp::mouseUp( MouseEvent event ) {}

void SmoothDisplacementMappingApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
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

void SmoothDisplacementMappingApp::keyUp( KeyEvent event ) {}

void SmoothDisplacementMappingApp::createMesh()
{
	// create vertex, normal and texcoord buffers
	const int  RES_X = 400;
	const int  RES_Z = 100;
	const vec3 size = vec3( 200.0f, 1.0f, 50.0f );

	std::vector<vec3> positions( RES_X * RES_Z );
	std::vector<vec3> normals( RES_X * RES_Z );
	std::vector<vec2> texcoords( RES_X * RES_Z );

	int i = 0;
	for( int x = 0; x < RES_X; ++x ) {
		for( int z = 0; z < RES_Z; ++z ) {
			const float u = float( x ) / RES_X;
			const float v = float( z ) / RES_Z;
			positions[i] = size * vec3( u - 0.5f, 0.0f, v - 0.5f );
			normals[i] = vec3( 0, 1, 0 );
			texcoords[i] = vec2( u, v );

			i++;
		}
	}

	// create index buffer
	vector<uint16_t> indices;
	indices.reserve( 6 * ( RES_X - 1 ) * ( RES_Z - 1 ) );

	for( int x = 0; x < RES_X - 1; ++x ) {
		for( int z = 0; z < RES_Z - 1; ++z ) {
			uint16_t i = x * RES_Z + z;

			indices.push_back( i );
			indices.push_back( i + 1 );
			indices.push_back( i + RES_Z );
			indices.push_back( i + RES_Z );
			indices.push_back( i + 1 );
			indices.push_back( i + RES_Z + 1 );
		}
	}

	// construct vertex buffer object
	gl::VboMesh::Layout layout;
	layout.attrib( geom::POSITION, 3 );
	layout.attrib( geom::NORMAL, 3 );
	layout.attrib( geom::TEX_COORD_0, 2 );

	mVboMesh = gl::VboMesh::create( positions.size(), GL_TRIANGLES, { layout }, indices.size() );
	mVboMesh->bufferAttrib( geom::POSITION, positions.size() * sizeof( vec3 ), positions.data() );
	mVboMesh->bufferAttrib( geom::NORMAL, normals.size() * sizeof( vec3 ), normals.data() );
	mVboMesh->bufferAttrib( geom::TEX_COORD_0, texcoords.size() * sizeof( vec2 ), texcoords.data() );
	mVboMesh->bufferIndices( indices.size() * sizeof( uint16_t ), indices.data() );

	// create a batch for better performance
	mBatch = gl::Batch::create( mVboMesh, mMeshShader );
}

void SmoothDisplacementMappingApp::createTextures()
{
	try {
		mBackgroundTexture = gl::Texture2d::create( loadImage( loadAsset( "background.png" ) ) );
	}
	catch( const std::exception &e ) {
		console() << "Could not load image: " << e.what() << std::endl;
	}
}

CINDER_APP( SmoothDisplacementMappingApp, RendererGl( RendererGl::Options().msaa( 16 ) ), &SmoothDisplacementMappingApp::prepare )
