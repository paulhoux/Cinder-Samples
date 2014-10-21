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

#include "cinder/ImageIo.h"
#include "cinder/app/AppBasic.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/VboMesh.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class GeometryShaderApp : public AppBasic {
public:
	void prepareSettings( Settings *settings );

	void setup();
	void update();
	void draw();

	void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );
	void mouseUp( MouseEvent event );

	void keyDown( KeyEvent event );

	void resize();
protected:
	gl::TextureRef	loadTexture( const std::string &path );
	void			loadShader( const std::string &path );
protected:
	float				mRadius;
	float				mThickness;
	float				mLimit;

	bool				mDrawWireframe;

	std::vector<vec2>	mPoints;

	bool				bIsDragging;
	ivec2				mDragPos;
	vec2				mDragFrom;
	vec2				*mDragVecPtr;

	ivec2				mWindowSize;

	gl::GlslProgRef		mShader;
	gl::VboMeshRef		mVboMesh;

	gl::TextureRef		mTexture;
	gl::TextureRef		mHelpTexture;
};

void GeometryShaderApp::prepareSettings( Settings *settings )
{
	settings->setTitle( "Drawing smooth lines using a geometry shader" );
	settings->setWindowSize( 640, 640 );
}

void GeometryShaderApp::setup()
{
	mRadius = 5.0f;
	mThickness = 50.0f;
	mLimit = 0.75f;

	mDrawWireframe = true;

	mPoints.clear();

	bIsDragging = false;

	mWindowSize = getWindowSize();

	loadShader( "shaders/lines1.geom" );
	mTexture = loadTexture( "textures/pattern1.png" );

	mHelpTexture = loadTexture( "textures/help.png" );
}

void GeometryShaderApp::update()
{
	// brute-force method: recreate mesh if anything changed
	if( !mVboMesh ) {
		if( mPoints.size() > 1 ) {
			// create a new vector that can contain 3D vertices
			std::vector<vec3> vertices;

			// to improve performance, make room for the vertices + 2 adjacency vertices
			vertices.reserve( mPoints.size() + 2 );

			// first, add an adjacency vertex at the beginning
			vertices.push_back( 2.0f * vec3( mPoints[0], 0 ) - vec3( mPoints[1], 0 ) );

			// next, add all 2D points as 3D vertices
			std::vector<vec2>::iterator itr;
			for( itr = mPoints.begin(); itr != mPoints.end(); ++itr )
				vertices.push_back( vec3( *itr, 0 ) );

			// next, add an adjacency vertex at the end
			size_t n = mPoints.size();
			vertices.push_back( 2.0f * vec3( mPoints[n - 1], 0 ) - vec3( mPoints[n - 2], 0 ) );

			// now that we have a list of vertices, create the index buffer
			n = vertices.size() - 2;
			std::vector<uint16_t> indices;
			indices.reserve( n * 4 );

			for( size_t i = 1; i < vertices.size() - 2; ++i ) {
				indices.push_back( i - 1 );
				indices.push_back( i );
				indices.push_back( i + 1 );
				indices.push_back( i + 2 );
			}

			// finally, create the mesh
			gl::VboMesh::Layout layout;
			layout.attrib( geom::POSITION, 3 );

			mVboMesh = gl::VboMesh::create( vertices.size(), GL_LINES_ADJACENCY_EXT, { layout }, indices.size() );
			mVboMesh->bufferAttrib( geom::POSITION, vertices.size() * sizeof( vec3 ), vertices.data() );
			mVboMesh->bufferIndices( indices.size() * sizeof( uint16_t ), indices.data() );
		}
		else
			mVboMesh.reset();
	}
}

void GeometryShaderApp::draw()
{
	// clear out the window 
	gl::clear( Color( 0.95f, 0.95f, 0.95f ) );

	// bind the shader and send the mesh to the GPU
	if( mShader && mVboMesh ) {
		gl::ScopedGlslProg shader( mShader );

		mShader->uniform( "WIN_SCALE", vec2( getWindowSize() ) ); // casting to vec2 is mandatory!
		mShader->uniform( "MITER_LIMIT", mLimit );
		mShader->uniform( "THICKNESS", mThickness );

		if( mTexture ) {
			gl::enableAlphaBlending();
			gl::color( Color::white() );
			mTexture->bind();
		}
		else
			gl::color( Color::black() );

		gl::draw( mVboMesh );

		if( mTexture ) {
			mTexture->unbind();
			gl::disableAlphaBlending();
		}

		if( mDrawWireframe ) {
			gl::color( Color::black() );
			gl::enableWireframe();
			gl::draw( mVboMesh );
			gl::disableWireframe();
		}
	}

	// draw all points as red circles
	gl::color( Color( 1, 0, 0 ) );

	std::vector<vec2>::const_iterator itr;
	for( itr = mPoints.begin(); itr != mPoints.end(); ++itr )
		gl::drawSolidCircle( *itr, mRadius );

	if( !mPoints.empty() )
		gl::drawStrokedCircle( mPoints.back(), mRadius + 2.0f );

	// draw help
	if( mHelpTexture ) {
		gl::color( Color::white() );
		gl::enableAlphaBlending();
		gl::draw( mHelpTexture );
		gl::disableAlphaBlending();
	}
}

void GeometryShaderApp::mouseDown( MouseEvent event )
{
	// check if any of the points is clicked (distance < radius)
	std::vector<vec2>::reverse_iterator itr;
	for( itr = mPoints.rbegin(); itr != mPoints.rend(); ++itr ) {
		float d = glm::distance( vec2( event.getPos() ), *itr );
		if( d < mRadius ) {
			// start dragging
			mDragPos = event.getPos();
			mDragFrom = *itr;
			mDragVecPtr = &( *itr );
			bIsDragging = true;

			//
			return;
		}
	}

	// not dragging, create new point
	mPoints.push_back( vec2( event.getPos() ) );

	// ...and drag it right away
	mDragPos = event.getPos();
	mDragFrom = mPoints.back();
	mDragVecPtr = &( mPoints.back() );
	bIsDragging = true;

	// invalidate mesh
	mVboMesh.reset();
}

void GeometryShaderApp::mouseDrag( MouseEvent event )
{
	if( bIsDragging ) {
		*mDragVecPtr = mDragFrom + vec2( event.getPos() - mDragPos );

		// invalidate mesh
		mVboMesh.reset();
	}
}

void GeometryShaderApp::mouseUp( MouseEvent event )
{
	if( bIsDragging ) {
		*mDragVecPtr = mDragFrom + vec2( event.getPos() - mDragPos );
		bIsDragging = false;

		// invalidate mesh
		mVboMesh.reset();
	}
}

void GeometryShaderApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_SPACE:
		mPoints.clear();
		// invalidate mesh
		mVboMesh.reset();
		break;
	case KeyEvent::KEY_DELETE:
		mPoints.pop_back();
		// invalidate mesh
		mVboMesh.reset();
		break;
	case KeyEvent::KEY_LEFTBRACKET:
		if( mThickness > 1.0f ) mThickness -= 1.0f;
		break;
	case KeyEvent::KEY_RIGHTBRACKET:
		if( mThickness < 100.0f ) mThickness += 1.0f;
		break;
	case KeyEvent::KEY_EQUALS: //For Macs without a keypad or a plus key
		if( !event.isShiftDown() ) {
			break;
		}
	case KeyEvent::KEY_PLUS:
	case KeyEvent::KEY_KP_PLUS:
		if( mLimit < 1.0f ) mLimit += 0.1f;
		break;
	case KeyEvent::KEY_MINUS:
	case KeyEvent::KEY_KP_MINUS:
		if( mLimit > -1.0f ) mLimit -= 0.1f;
		break;
	case KeyEvent::KEY_w:
		mDrawWireframe = !mDrawWireframe;
		break;
	case KeyEvent::KEY_F5:
		mTexture = loadTexture( "textures/pattern1.png" );
		break;
	case KeyEvent::KEY_F6:
		mTexture = loadTexture( "textures/pattern2.png" );
		break;
	case KeyEvent::KEY_F7:
		loadShader( "shaders/lines1.geom" );
		break;
	case KeyEvent::KEY_F8:
		loadShader( "shaders/lines2.geom" );
		break;
	}
}

void GeometryShaderApp::resize()
{
	// keep points centered
	vec2 offset = 0.5f * vec2( getWindowSize() - mWindowSize );

	std::vector<vec2>::iterator itr;
	for( itr = mPoints.begin(); itr != mPoints.end(); ++itr )
		*itr += offset;

	mWindowSize = getWindowSize();

	// invalidate mesh
	mVboMesh.reset();

	// force redraw
	update();
}

gl::TextureRef GeometryShaderApp::loadTexture( const std::string &path )
{
	try {
		return gl::Texture::create( loadImage( loadAsset( path ) ) );
	}
	catch( const std::exception &e ) {
		console() << "Could not load texture:" << e.what() << std::endl;
		return gl::TextureRef();
	}
}

void GeometryShaderApp::loadShader( const std::string &path )
{
	// Load the geometry shader as a text file into memory and prepend the header
	DataSourceRef geomFile = loadAsset( path );

	// Load vertex and fragments shaders as text files and compile the shader
	try {
		DataSourceRef vertFile = loadAsset( "shaders/lines.vert" );

		DataSourceRef fragFile = loadAsset( "shaders/lines.frag" );

		mShader = gl::GlslProg::create( vertFile, fragFile, geomFile );
	}
	catch( const std::exception &e ) {
		console() << "Could not compile shader:" << e.what() << std::endl;
	}
}

CINDER_APP_BASIC( GeometryShaderApp, RendererGl )
