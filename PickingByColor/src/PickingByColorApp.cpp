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
#include "cinder/Font.h"
#include "cinder/ObjLoader.h"
#include "cinder/TriMesh.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/scoped.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PickingByColorApp : public App {
  public:
	static void prepare( Settings *settings );

	void setup();
	void shutdown();
	void update();
	void draw();

	void mouseMove( MouseEvent event );
	void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );
	void mouseUp( MouseEvent event );

	void keyDown( KeyEvent event );
	void keyUp( KeyEvent event );

	void resize();

	//! renders the scene
	void render();
	//! samples the color buffer to determine which object is under the mouse
	std::string pick( const ivec2 &position );
	//! loads an OBJ file, writes it to a much faster binary file and loads the mesh
	void loadMesh( const std::string &objFile, const std::string &meshFile, gl::BatchRef &mesh );
	//! loads the shaders
	void loadShaders();
	//! draws a grid on the floor
	void drawGrid( float size = 100.0f, float step = 10.0f );

  public:
	// utility functions to translate colors to and from ints or chars
	static Color charToColor( unsigned char r, unsigned char g, unsigned char b ) { return Color( r / 255.0f, g / 255.0f, b / 255.0f ); };

	static unsigned int charToInt( unsigned char r, unsigned char g, unsigned char b ) { return b + ( g << 8 ) + ( r << 16 ); };

	static Color intToColor( unsigned int i )
	{
		unsigned char r = ( i >> 16 ) & 0xFF;
		unsigned char g = ( i >> 8 ) & 0xFF;
		unsigned char b = ( i >> 0 ) & 0xFF;
		return Color( r / 255.0f, g / 255.0f, b / 255.0f );
	};

	static unsigned int colorToInt( const Color &color )
	{
		unsigned char r = (unsigned char)( color.r * 255 );
		unsigned char g = (unsigned char)( color.g * 255 );
		unsigned char b = (unsigned char)( color.b * 255 );
		return b + ( g << 8 ) + ( r << 16 );
	};

  protected:
	//! our camera
	CameraPersp mCamera;
	CameraUi    mCameraUi;

	//! mesh and picking color of the pitcher object
	gl::BatchRef mMeshPitcher;
	Color        mColorPitcher;

	//! mesh and picking color of the watering can object
	gl::BatchRef mMeshCan;
	Color        mColorCan;

	//! our Phong shader, which supports multiple targets
	gl::GlslProgRef mPhongShader;

	//! our main framebuffer (AA, containing 2 color buffers)
	gl::FboRef mFbo;
	//! our little picking framebuffer (non-AA)
	gl::FboRef mPickingFbo;

	//! keeping track of our cursor position
	ivec2 mMousePos;

	//! fancy font fizzle
	Font mFont;

	//! background color
	Color mColorBackground;
};

void PickingByColorApp::prepare( Settings *settings )
{
	settings->setWindowSize( 900, 600 );
	settings->setFrameRate( 100.0f );
	settings->setTitle( "Picking using multiple targets and color coding" );
}

void PickingByColorApp::setup()
{
	// note: we will setup our camera in the 'resize' function,
	//  because it is called anyway so we don't have to set it up twice

	// create materials
	// mMaterialPitcher.setAmbient( Color::black() );
	// mMaterialPitcher.setDiffuse( Color(0.45f, 0.45f, 0.5f) );
	// mMaterialPitcher.setSpecular( Color(0.55f, 0.55f, 0.55f) );
	// mMaterialPitcher.setShininess( 3.0f );

	// mMaterialCan.setAmbient( Color::black() );
	// mMaterialCan.setDiffuse( Color(0.40f, 0.60f, 0.50f) );
	// mMaterialCan.setSpecular( Color(0.50f, 0.70f, 0.60f) );
	// mMaterialCan.setShininess( 20.0f );

	// load shaders
	loadShaders();

	// load meshes
	loadMesh( "models/pitcher.obj", "models/pitcher.msh", mMeshPitcher );
	loadMesh( "models/watering_can.obj", "models/watering_can.msh", mMeshCan );

	// each object should have a unique picking color.
	// To make sure the colors are pickable, you should use
	// the intToColor() and colorToInt() functions.
	mColorPitcher = intToColor( 0x0000FF ); // blue
	mColorCan = intToColor( 0x00FF00 );     // green

	// load font
	mFont = Font( loadAsset( "font/b2sq.ttf" ), 32 );

	// set background color
	mColorBackground = Color( 0.1f, 0.1f, 0.1f );
}

void PickingByColorApp::shutdown() {}

void PickingByColorApp::update() {}

void PickingByColorApp::draw()
{
	// render the scene
	{
		gl::ScopedFramebuffer fbo( mFbo );
		render();
	}

	// draw the scene
	gl::ScopedColor color( Color::white() );
	gl::draw( mFbo->getTexture2d( GL_COLOR_ATTACHMENT0 ), getWindowBounds() );

	// draw the color coded scene in the upper left corner
	gl::draw( mFbo->getTexture2d( GL_COLOR_ATTACHMENT1 ), Rectf( getWindowBounds() ) * 0.2f );

	// draw the picking framebuffer in the upper right corner
	if( mPickingFbo ) {
		Rectf rct = (Rectf)mPickingFbo->getBounds() * 5.0f;
		rct.offset( vec2( (float)getWindowWidth() - rct.getWidth(), 0 ) );
		gl::draw( mPickingFbo->getColorTexture(), rct );
	}

	// perform picking and display the results
	//  (alternatively you can do it in the 'mouseMove' or 'mouseDown' function)
	gl::ScopedBlendAlpha blend;
	gl::drawStringCentered( pick( mMousePos ), vec2( 0.5f * getWindowWidth(), getWindowHeight() - 50.0f ), Color::white(), mFont );
}

void PickingByColorApp::mouseMove( MouseEvent event )
{
	mMousePos = event.getPos();
}

void PickingByColorApp::mouseDown( MouseEvent event )
{
	// handle the camera
	mCameraUi.mouseDown( event.getPos() );
}

void PickingByColorApp::mouseDrag( MouseEvent event )
{
	mMousePos = event.getPos();

	// move the camera
	mCameraUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void PickingByColorApp::mouseUp( MouseEvent event ) {}

void PickingByColorApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	}
}

void PickingByColorApp::keyUp( KeyEvent event ) {}

void PickingByColorApp::resize()
{
	// setup the camera
	mCamera.setPerspective( 60.0f, getWindowAspectRatio(), 0.1f, 1000.0f );
	mCameraUi.setCamera( &mCamera );

	// create or resize framebuffer if needed
	const int w = getWindowWidth();
	const int h = getWindowHeight();
	if( !mFbo || mFbo->getWidth() != w || mFbo->getHeight() != h ) {
		gl::Fbo::Format fmt;

		// we create multiple color targets:
		//  -one for the scene as we will view it
		//  -one to contain a color coded version of the scene that we can use for picking
		// fmt.enableColorBuffer( true, 2 );
		gl::Texture2d::Format tfmt;
		tfmt.setInternalFormat( GL_RGBA );

		gl::Texture2dRef tex0 = gl::Texture2d::create( w, h, tfmt );
		fmt.attachment( GL_COLOR_ATTACHMENT0, tex0 );
		gl::Texture2dRef tex1 = gl::Texture2d::create( w, h, tfmt );
		fmt.attachment( GL_COLOR_ATTACHMENT1, tex1 );

		// this sample does not work if the fbo uses multi-sampling
		fmt.setSamples( 0 );

		// create the buffer
		mFbo = gl::Fbo::create( w, h, fmt );
	}
}

//

void PickingByColorApp::render()
{
	// clear background
	gl::clear( mColorBackground );

	// specify the camera matrices
	gl::pushMatrices();
	gl::setMatrices( mCamera );

	// specify render states
	gl::ScopedDepth depth( true, true );

	// draw a grid on the floor
	drawGrid();

	// draw meshes:
	// -bind phong shader, which renders to both our color targets.
	//  See 'shaders/phong.frag'
	gl::ScopedGlslProg shader( mPhongShader );

	// -draw pitcher
	{
		// -each mesh should have a unique picking color that we can use to
		//  find out which object is under the cursor.
		mPhongShader->uniform( "pickingColor", mColorPitcher );

		gl::ScopedColor color( 0.45f, 0.45f, 0.5f );
		gl::pushModelMatrix();
		gl::translate( 10.0f, 0.0f, 0.0f );
		mMeshPitcher->draw();
		gl::popModelMatrix();
	}

	// -draw can
	{
		mPhongShader->uniform( "pickingColor", mColorCan );

		gl::color( 0.40f, 0.60f, 0.50f );
		gl::pushModelMatrix();
		gl::translate( -10.0f, 0.0f, 0.0f );
		mMeshCan->draw();
		gl::popModelMatrix();
	}

	// restore matrices
	gl::popMatrices();
}

std::string PickingByColorApp::pick( const ivec2 &position )
{
	// this is the main section of the demo:
	//  here we sample the second color target to find out
	//  which color is under the cursor.

	// prevent errors if framebuffer does not exist
	if( !mFbo )
		return "Error";

	// first, specify a small region around the current cursor position
	float scaleX = mFbo->getWidth() / (float)getWindowWidth();
	float scaleY = mFbo->getHeight() / (float)getWindowHeight();
	ivec2 pixel( (int)( position.x * scaleX ), (int)( ( getWindowHeight() - position.y ) * scaleY ) );
	Area  area( pixel.x - 5, pixel.y - 5, pixel.x + 5, pixel.y + 5 );

	// next, we need to copy this region to a non-anti-aliased framebuffer
	//  because sadly we can not sample colors from an anti-aliased one. However,
	//  this also simplifies the glReadPixels statement, so no harm done.
	//  Here, we create that non-AA buffer if it does not yet exist.
	if( !mPickingFbo ) {
		gl::Fbo::Format fmt;

		// make sure the framebuffer is not anti-aliased
		fmt.setSamples( 0 );
		fmt.setCoverageSamples( 0 );

		// you can omit these lines if you don't intent to display the picking framebuffer
		gl::Texture2d::Format tfmt;
		tfmt.setMagFilter( GL_NEAREST );
		tfmt.setMinFilter( GL_LINEAR );

		fmt.setColorTextureFormat( tfmt );

		mPickingFbo = gl::Fbo::create( area.getWidth(), area.getHeight(), fmt );
	}

	// (Cinder does not yet provide a way to handle multiple color targets in the blitTo function,
	//  so we have to make sure the correct target is selected before calling it)
	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, mFbo->getId() );
	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, mPickingFbo->getId() );
	glReadBuffer( GL_COLOR_ATTACHMENT1_EXT );
	glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );

	mFbo->blitTo( mPickingFbo, area, mPickingFbo->getBounds() );

	// bind the picking framebuffer, so we can read its pixels
	mPickingFbo->bindFramebuffer();

	// read pixel value(s) in the area
	GLubyte buffer[400]; // make sure this is large enough to hold 4 bytes for every pixel!

	glReadBuffer( GL_COLOR_ATTACHMENT0_EXT );
	glReadPixels( 0, 0, mPickingFbo->getWidth(), mPickingFbo->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, (void *)buffer );

	// unbind the picking framebuffer
	mPickingFbo->unbindFramebuffer();

	// calculate the total number of pixels
	unsigned int total = ( mPickingFbo->getWidth() * mPickingFbo->getHeight() );

	// now that we have the color information, count each occuring color
	unsigned int color;

	std::map<unsigned int, unsigned int> occurences;
	for( size_t i = 0; i < total; ++i ) {
		color = charToInt( buffer[( i * 4 ) + 0], buffer[( i * 4 ) + 1], buffer[( i * 4 ) + 2] );
		occurences[color]++;
	}

	// find the most occuring color by calling std::max_element using a custom comparator.
	unsigned int max = 0;

	auto itr = std::max_element( occurences.begin(), occurences.end(), []( const std::pair<unsigned int, unsigned int> &a, const std::pair<unsigned int, unsigned int> &b ) { return a.second < b.second; } );

	if( itr != occurences.end() ) {
		color = itr->first;
		max = itr->second;
	}

	// if this color is present in at least 50% of the pixels,
	//  we can safely assume that it is indeed belonging to one object
	if( max >= ( total / 2 ) ) {
		if( color == colorToInt( mColorCan ) )
			return "Watering Can";
		else if( color == colorToInt( mColorPitcher ) )
			return "Pitcher";
		else if( color == colorToInt( mColorBackground ) )
			return "Background";
		else
			return "Nothing";
	}
	else {
		// we can't be sure about the color, we probably are on an object's edge
		return "Uncertain";
	}
}

void PickingByColorApp::loadMesh( const std::string &objFile, const std::string &meshFile, gl::BatchRef &mesh )
{
	TriMeshRef triMesh;

	try {
		// try to load a binary mesh file - sadly the following line does not work:
		// mMesh.read( loadAsset(meshfile) );

		// note: instead of throwing an exception, Cinder crashes if the
		//   file does not exist. Hopefully this will be changed in future versions.
		DataSourceRef file = loadAsset( meshFile );
		if( !file->createStream() )
			throw std::exception();

		triMesh = TriMesh::create();
		triMesh->read( file );
	}
	catch( ... ) {
		// if it failed or didn't exist, load an obj file...
		DataSourceRef file = loadAsset( objFile );
		if( file->createStream() ) {
			ObjLoader loader( file );
			// ...create a mesh...
			triMesh = TriMesh::create( loader );
			// ...and write it to a binary mesh file for future use
			triMesh->write( writeFile( getAssetPath( "" ) / meshFile ) );
		}
	}

	if( triMesh ) {
		mesh = gl::Batch::create( *triMesh, mPhongShader );
	}
}

void PickingByColorApp::loadShaders()
{
	try {
		mPhongShader = gl::GlslProg::create( loadAsset( "shaders/phong.vert" ), loadAsset( "shaders/phong.frag" ) );
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
	}
}

void PickingByColorApp::drawGrid( float size, float step )
{
	gl::color( Colorf( 0.2f, 0.2f, 0.2f ) );
	for( float i = -size; i <= size; i += step ) {
		gl::drawLine( vec3( i, 0.0f, -size ), vec3( i, 0.0f, size ) );
		gl::drawLine( vec3( -size, 0.0f, i ), vec3( size, 0.0f, i ) );
	}
}

CINDER_APP( PickingByColorApp, RendererGl )