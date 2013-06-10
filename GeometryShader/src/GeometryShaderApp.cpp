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
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"

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
	gl::Texture loadTexture( const std::string &path );
	void		loadShader( const std::string &path );
protected:
	float				mRadius;
	float				mThickness;
	float				mLimit;

	bool				mDrawWireframe;

	std::vector<Vec2f>	mPoints;

	bool				bIsDragging;
	Vec2i				mDragPos;
	Vec2f				mDragFrom;
	Vec2f				*mDragVecPtr;

	Vec2i				mWindowSize;

	gl::GlslProg		mShader;
	gl::VboMesh			mVboMesh;

	gl::Texture			mTexture;
	gl::Texture			mHelpTexture;
};

void GeometryShaderApp::prepareSettings( Settings *settings )
{
	settings->setTitle("Drawing smooth lines using a geometry shader");
	settings->setWindowSize(640, 640);
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

	loadShader("shaders/lines_geom1.glsl");
	mTexture = loadTexture("textures/pattern1.png");

	mHelpTexture = loadTexture("textures/help.png");
}

void GeometryShaderApp::update()
{
	// brute-force method: recreate mesh if anything changed
	if( !mVboMesh ) {
		if( mPoints.size() > 1 ) {
			// create a new vector that can contain 3D vertices
			std::vector<Vec3f> vertices;

			// to improve performance, make room for the vertices + 2 adjacency vertices
			vertices.reserve( mPoints.size() + 2);

			// first, add an adjacency vertex at the beginning
			vertices.push_back( 2.0f * Vec3f(mPoints[0]) - Vec3f(mPoints[1]) );

			// next, add all 2D points as 3D vertices
			std::vector<Vec2f>::iterator itr;
			for(itr=mPoints.begin();itr!=mPoints.end();++itr)
				vertices.push_back( Vec3f( *itr ) );

			// next, add an adjacency vertex at the end
			size_t n = mPoints.size();
			vertices.push_back( 2.0f * Vec3f(mPoints[n-1]) - Vec3f(mPoints[n-2]) );

			// now that we have a list of vertices, create the index buffer
			n = vertices.size() - 2;
			std::vector<uint32_t> indices;
			indices.reserve( n * 4 );

			for(size_t i=1;i<vertices.size()-2;++i) {
				indices.push_back(i-1);
				indices.push_back(i);
				indices.push_back(i+1);
				indices.push_back(i+2);
			}

			// finally, create the mesh
			gl::VboMesh::Layout layout;
			layout.setStaticPositions();
			layout.setStaticIndices();

			mVboMesh = gl::VboMesh( vertices.size(), indices.size(), layout, GL_LINES_ADJACENCY_EXT );
			mVboMesh.bufferPositions( &(vertices.front()), vertices.size() );
			mVboMesh.bufferIndices( indices );
		}
		else
			mVboMesh = gl::VboMesh();
	}
}

void GeometryShaderApp::draw()
{
	// clear out the window 
	gl::clear( Color(0.95f, 0.95f, 0.95f) );

	// bind the shader and send the mesh to the GPU
	if(mShader && mVboMesh) {
		mShader.bind();
		mShader.uniform( "WIN_SCALE", Vec2f( getWindowSize() ) ); // casting to Vec2f is mandatory!
		mShader.uniform( "MITER_LIMIT", mLimit );
		mShader.uniform( "THICKNESS", mThickness );
		
		if(mTexture) {
			gl::enableAlphaBlending();
			gl::color( Color::white() );
			mTexture.enableAndBind();
		}
		else
			gl::color( Color::black() );

		gl::draw( mVboMesh );

		if(mTexture) {
			mTexture.unbind();
			gl::disableAlphaBlending();
		}

		if(mDrawWireframe) {
			gl::color( Color::black() );
			gl::enableWireframe();
			gl::draw( mVboMesh );
			gl::disableWireframe();
		}

		mShader.unbind();
	}

	// draw all points as red circles
	gl::color( Color(1, 0, 0) );

	std::vector<Vec2f>::const_iterator itr;
	for(itr=mPoints.begin();itr!=mPoints.end();++itr)
		gl::drawSolidCircle( *itr, mRadius );

	if( ! mPoints.empty() )
		gl::drawStrokedCircle( mPoints.back(), mRadius + 2.0f );

	// draw help
	if(mHelpTexture) {
		gl::color( Color::white() );
		gl::enableAlphaBlending();
		gl::draw( mHelpTexture );
		gl::disableAlphaBlending();
	}
}

void GeometryShaderApp::mouseDown( MouseEvent event )
{
	// check if any of the points is clicked (distance < radius)
	std::vector<Vec2f>::reverse_iterator itr;
	for(itr=mPoints.rbegin();itr!=mPoints.rend();++itr) {
		float d = Vec2f( event.getPos() ).distance( *itr );
		if(d < mRadius) {
			// start dragging
			mDragPos = event.getPos();
			mDragFrom = *itr;
			mDragVecPtr = &(*itr);
			bIsDragging = true;

			//
			return;
		}
	}

	// not dragging, create new point
	mPoints.push_back( Vec2f( event.getPos() ) );

	// ...and drag it right away
	mDragPos = event.getPos();
	mDragFrom = mPoints.back();
	mDragVecPtr = &(mPoints.back());
	bIsDragging = true;

	// invalidate mesh
	mVboMesh = gl::VboMesh();
}

void GeometryShaderApp::mouseDrag( MouseEvent event )
{
	if(bIsDragging) {
		*mDragVecPtr = mDragFrom + (event.getPos() - mDragPos);

		// invalidate mesh
		mVboMesh = gl::VboMesh();
	}
}

void GeometryShaderApp::mouseUp( MouseEvent event )
{
	if(bIsDragging) {
		*mDragVecPtr = mDragFrom + (event.getPos() - mDragPos);
		bIsDragging = false;

		// invalidate mesh
		mVboMesh = gl::VboMesh();
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
		mVboMesh = gl::VboMesh();
		break;
	case KeyEvent::KEY_DELETE:
		mPoints.pop_back();
		// invalidate mesh
		mVboMesh = gl::VboMesh();
		break;
	case KeyEvent::KEY_LEFTBRACKET:
		if(mThickness > 1.0f) mThickness -= 1.0f;
		break;
	case KeyEvent::KEY_RIGHTBRACKET:
		if(mThickness < 100.0f) mThickness += 1.0f;
		break;
    case KeyEvent::KEY_EQUALS: //For Macs without a keypad or a plus key
        if(!event.isShiftDown()){
            break;
        }
	case KeyEvent::KEY_PLUS:
	case KeyEvent::KEY_KP_PLUS:
		if(mLimit < 1.0f) mLimit += 0.1f;
		break;
	case KeyEvent::KEY_MINUS:
	case KeyEvent::KEY_KP_MINUS:
		if(mLimit > -1.0f) mLimit -= 0.1f;
		break;
	case KeyEvent::KEY_w:
		mDrawWireframe = !mDrawWireframe;
		break;
	case KeyEvent::KEY_F5:
		mTexture = loadTexture("textures/pattern1.png");
		break;
	case KeyEvent::KEY_F6:
		mTexture = loadTexture("textures/pattern2.png");
		break;
	case KeyEvent::KEY_F7:		
		loadShader("shaders/lines_geom1.glsl");
		break;
	case KeyEvent::KEY_F8:		
		loadShader("shaders/lines_geom2.glsl");
		break;
	}
}

void GeometryShaderApp::resize()
{
	// keep points centered
	Vec2f offset = 0.5f * Vec2f( getWindowSize() - mWindowSize );

	std::vector<Vec2f>::iterator itr;
	for(itr=mPoints.begin();itr!=mPoints.end();++itr)
		*itr += offset;

	mWindowSize = getWindowSize();

	// invalidate mesh
	mVboMesh = gl::VboMesh();

	// force redraw
	update();
}

gl::Texture GeometryShaderApp::loadTexture( const std::string &path )
{
	try {
		return gl::Texture( loadImage( loadAsset(path) ) );
	}
	catch( const std::exception &e ) {
		console() << "Could not load texture:" << e.what() << std::endl;
		return gl::Texture();
	}
}

void GeometryShaderApp::loadShader( const std::string &path )
{
	try {
		mShader = gl::GlslProg( 
			loadAsset("shaders/lines_vert.glsl"), loadAsset("shaders/lines_frag.glsl"), loadAsset(path),
			GL_LINES_ADJACENCY_EXT, GL_TRIANGLE_STRIP, 7 
		);
	}
	catch( const std::exception &e ) {
		console() << "Could not compile shader:" << e.what() << std::endl;
	}
}

CINDER_APP_BASIC( GeometryShaderApp, RendererGl )
