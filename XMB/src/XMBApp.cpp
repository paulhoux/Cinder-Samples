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
#include "cinder/Perlin.h"
#include "cinder/Surface.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class XMBApp : public AppBasic {
public:
	void prepareSettings( Settings *settings );
	
	void setup();
	void update();
	void draw();
	
	void resize( ResizeEvent event );
	
	void mouseMove( MouseEvent event );	
	void mouseDown( MouseEvent event );	
	void mouseDrag( MouseEvent event );	
	void mouseUp( MouseEvent event );	
	
	void keyDown( KeyEvent event );
	void keyUp( KeyEvent event );
private:
	void createMesh();
	void createTextures();
private:
	MayaCamUI		mMayaCam;
	CameraPersp		mCamera;
	gl::GlslProg	mShader;
	gl::Texture		mWaveTexture;
	gl::Texture		mNoiseTexture;
	gl::Texture		mBackgroundTexture;
	gl::VboMesh		mVboMesh;
};

void XMBApp::prepareSettings(Settings *settings)
{
	settings->setTitle("Cinder Sample");
}

void XMBApp::setup()
{
	mCamera.setEyePoint( Vec3f( 0.0f, 0.0f, -130.0f ) );
	mCamera.setCenterOfInterestPoint( Vec3f( 0.0f, 0.0f, 0.0f ) ) ;
	mMayaCam.setCurrentCam( mCamera );

	createMesh();
	createTextures();

	try {
		mBackgroundTexture = gl::Texture( loadImage( loadAsset("background.jpg") ) );
		mShader = gl::GlslProg( loadAsset("xmb_vert.glsl"), loadAsset("xmb_frag.glsl"), loadAsset("xmb_geom.glsl"), GL_TRIANGLES, GL_TRIANGLES, 3 );
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
		quit();
	}
}

void XMBApp::update()
{
}

void XMBApp::draw()
{
	gl::clear();

	gl::draw( mBackgroundTexture, getWindowBounds() );

	gl::pushMatrices();
	gl::setMatrices( mCamera );

	gl::color( Color::white() );
	//gl::enableWireframe();
	gl::enableAlphaBlending();

	mWaveTexture.enableAndBind();
	mNoiseTexture.bind(1);

	mShader.bind();
	mShader.uniform( "time", float( getElapsedSeconds() ) );
	mShader.uniform( "sinus", 0 );
	mShader.uniform( "noise", 1 );

	gl::draw( mVboMesh );

	mShader.unbind();

	mNoiseTexture.unbind();
	mWaveTexture.unbind();

	gl::disableAlphaBlending();
	//gl::disableWireframe();

	gl::popMatrices();

	//gl::draw( mNoiseTexture );
	//gl::draw( mNoiseTexture, Vec2f(256,0) );
	//gl::draw( mNoiseTexture, Vec2f(0,256) );
}

void XMBApp::resize( ResizeEvent event )
{
	mCamera.setAspectRatio( event.getAspectRatio() );
	mMayaCam.setCurrentCam( mCamera );
}

void XMBApp::mouseMove( MouseEvent event )
{
}

void XMBApp::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void XMBApp::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	mCamera = mMayaCam.getCamera();
}

void XMBApp::mouseUp( MouseEvent event )
{
}

void XMBApp::keyDown( KeyEvent event )
{
	switch( event.getCode() )
	{
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_SPACE:
		mCamera.setEyePoint( Vec3f( 0.0f, 0.0f, -130.0f ) );
		mCamera.setCenterOfInterestPoint( Vec3f( 0.0f, 0.0f, 0.0f ) ) ;
		mMayaCam.setCurrentCam( mCamera );
		break;
	}
}

void XMBApp::keyUp( KeyEvent event )
{
}

void XMBApp::createMesh()
{
	vector<Vec3f>		vertices;
	vector<Vec2f>		texcoords;
	vector<Vec3f>		normals;
	vector<uint32_t>	indices;

	const int RES_X = 500;
	const int RES_Z = 250;
	const Vec3f size(200.0f, 0.0f, 50.0f);

	for(int x=0;x<RES_X;++x) {
		for(int z=0;z<RES_Z;++z) {
			vertices.push_back( Vec3f( (float(x) / RES_X) - 0.5f , 0.0f, float(z) / RES_Z - 0.5f ) * size );
			normals.push_back( Vec3f::yAxis() );
			texcoords.push_back( Vec2f( float(x) / RES_X, float(z) / RES_Z ) );
		}
	}

	for(int x=0;x<RES_X-1;++x) {
		for(int z=0;z<RES_Z-1;++z) {
			uint32_t a = x * RES_Z + z;

			indices.push_back( a ); indices.push_back( a + RES_Z ); indices.push_back( a + 1 );
			indices.push_back( a + RES_Z ); indices.push_back( a + RES_Z + 1 );  indices.push_back( a + 1 );
		}
	}

	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticTexCoords2d();
	layout.setStaticNormals();
	layout.setStaticIndices();

	mVboMesh = gl::VboMesh( vertices.size(), indices.size(), layout, GL_TRIANGLES );
	mVboMesh.bufferPositions( vertices );
	mVboMesh.bufferTexCoords2d( 0, texcoords );
	mVboMesh.bufferNormals( normals );
	mVboMesh.bufferIndices( indices );
}

void XMBApp::createTextures()
{
	gl::Texture::Format fmt;
	fmt.setWrap( GL_REPEAT, GL_REPEAT );

	// create sinus texture
	Surface s( 1024, 1, false, SurfaceChannelOrder::CHAN_RED );
	Surface::Iter itr = s.getIter();
	while( itr.line() ) {
		while( itr.pixel() ) {
			Vec2i p = itr.getPos();
			float x = 0.5f + 0.5f * math<float>::sin( p.x / 1024.0f * float(2.0 * M_PI) );
			uint8_t c = uint8_t( x * 255.0f );
			s.setPixel( p, ColorT<uint8_t>(c, c, c) );
		}
	}

	mWaveTexture = gl::Texture( s, fmt );

	// create noise texture
	Perlin perlin(6, 1);
	int w = 256;
	int h = 256;
	Surface n( w, h, false, SurfaceChannelOrder::CHAN_RED );
	itr = n.getIter();
	while( itr.line() ) {
		while( itr.pixel() ) {
			Vec2f p = Vec2f( itr.getPos() ) / Vec2f(w, h);
			float x = perlin.fBm( p.x, p.y ) * (1.0f - p.x) * (1.0f - p.y) +
				perlin.fBm( p.x-1.0f, p.y ) * p.x * (1.0f - p.y) +
				perlin.fBm( p.x-1.0f, p.y-1.0f ) * p.x * p.y +
				perlin.fBm( p.x, p.y-1.0f ) * (1.0f - p.x) * p.y;
			x += 0.5f;
			uint8_t c = uint8_t( x * 255.0f );
			n.setPixel( itr.getPos(), ColorT<uint8_t>(c, c, c) );
		}
	}

	mNoiseTexture = gl::Texture( n, fmt );
}

CINDER_APP_BASIC( XMBApp, RendererGl )
