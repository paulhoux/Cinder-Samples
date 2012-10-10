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

	void renderDisplacementMap();
	void renderNormalMap();

	void resetCamera();
private:
	bool			mDrawTextures;
	bool			mDrawWireframe;

	MayaCamUI		mMayaCam;
	CameraPersp		mCamera;

	gl::Fbo			mDispMapFbo;
	gl::GlslProg	mDispMapShader;

	gl::Fbo			mNormalMapFbo;
	gl::GlslProg	mNormalMapShader;

	gl::Texture		mSinusTexture;
	gl::Texture		mBackgroundTexture;
	
	gl::VboMesh		mVboMesh;
	gl::GlslProg	mMeshShader;
};

void XMBApp::prepareSettings(Settings *settings)
{
	settings->setTitle("Playstation XrossMediaBar");
	settings->setWindowSize( 1280, 720 );
	settings->setFrameRate( 300.0f );
}

void XMBApp::setup()
{
	mDrawTextures = false;
	mDrawWireframe = true;

	resetCamera();

	try {
		mBackgroundTexture = gl::Texture( loadImage( loadAsset("background.jpg") ) );

		mDispMapShader = gl::GlslProg( loadAsset("displacement_map_vert.glsl"), loadAsset("displacement_map_frag.glsl") ); 
		mNormalMapShader = gl::GlslProg( loadAsset("normal_map_vert.glsl"), loadAsset("normal_map_frag.glsl") );
		mMeshShader = gl::GlslProg( loadAsset("xmb_vert.glsl"), loadAsset("xmb_frag.glsl") );
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
		quit();
	}

	createMesh();
	createTextures();

	gl::Fbo::Format fmt;
	fmt.enableDepthBuffer(false);

	fmt.setColorInternalFormat( GL_R32F );
	mDispMapFbo = gl::Fbo(256, 256, fmt);
	
	fmt.setColorInternalFormat( GL_RGB32F );
	mNormalMapFbo = gl::Fbo(256, 256, fmt);
}

void XMBApp::update()
{
	// render displacement map
	renderDisplacementMap();

	// render normal map
	renderNormalMap();
}

void XMBApp::draw()
{
	gl::clear();

	// render background
	gl::draw( mBackgroundTexture, getWindowBounds() );


	//
	if(mDrawTextures) 
	{	
		gl::draw( mDispMapFbo.getTexture(), Vec2f(0,0) );
		gl::draw( mNormalMapFbo.getTexture(), Vec2f(256,0) );
	}

	// finally, render our mesh using vertex displacement
	gl::pushMatrices();
	gl::setMatrices( mCamera );

	//gl::enableDepthRead();
	//gl::enableDepthWrite();
	gl::enableAlphaBlending();
	if(mDrawWireframe) 
		gl::enableWireframe();

	//gl::color( ColorA(1, 1, 1, 0.2f) );
	//gl::draw( mVboMesh );

	mDispMapFbo.getTexture().enableAndBind();
	mNormalMapFbo.getTexture().bind(1);

	mMeshShader.bind();
	mMeshShader.uniform( "displacement_map", 0 );
	mMeshShader.uniform( "normal_map", 1 );

	gl::color( Color::white() );
	gl::draw( mVboMesh );

	mMeshShader.unbind();

	mNormalMapFbo.unbindTexture();
	mDispMapFbo.unbindTexture();
	
	gl::disableWireframe();
	gl::disableAlphaBlending();
	//gl::disableDepthWrite();
	//gl::disableDepthRead();

	gl::popMatrices();
}

void XMBApp::resetCamera()
{
	mCamera.setEyePoint( Vec3f( 0.0f, 0.0f, 130.0f ) );
	mCamera.setCenterOfInterestPoint( Vec3f( 0.0f, 0.0f, 0.0f ) ) ;
	mMayaCam.setCurrentCam( mCamera );
}

void XMBApp::renderDisplacementMap()
{
	if(mDispMapShader) 
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

			// bind the textures containing sinus and noise values
			mSinusTexture.enableAndBind();

			// render the displacement map
			mDispMapShader.bind();
			mDispMapShader.uniform( "time", 4.0f * float( getElapsedSeconds() ) );
			mDispMapShader.uniform( "sinus", 0 );
			gl::drawSolidRect( mDispMapFbo.getBounds() );
			mDispMapShader.unbind();

			// clean up after ourselves
			mSinusTexture.unbind();
			gl::popMatrices();
			glPopAttrib();
		}
		mDispMapFbo.unbindFramebuffer();
	}
}

void XMBApp::renderNormalMap()
{
	if(mNormalMapShader) 
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

			// bind the textures 
			mDispMapFbo.getTexture().enableAndBind();

			// render the normal map
			mNormalMapShader.bind();
			mNormalMapShader.uniform( "texture", 0 );
			mNormalMapShader.uniform( "amplitude", 4.0f );
			gl::drawSolidRect( mNormalMapFbo.getBounds() );
			mNormalMapShader.unbind();

			// clean up after ourselves
			mDispMapFbo.getTexture().unbind();

			gl::popMatrices();

			glPopAttrib();
		}
		mNormalMapFbo.unbindFramebuffer();
	}
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
	case KeyEvent::KEY_s:
		try { 
			mDispMapShader = gl::GlslProg( loadAsset("displacement_map_vert.glsl"), loadAsset("displacement_map_frag.glsl") ); 
			mNormalMapShader = gl::GlslProg( loadAsset("normal_map_vert.glsl"), loadAsset("normal_map_frag.glsl") );
			mMeshShader = gl::GlslProg( loadAsset("xmb_vert.glsl"), loadAsset("xmb_frag.glsl") );
		}
		catch( const std::exception &e ) { console() << e.what() << std::endl; }
		break;
	case KeyEvent::KEY_t:
		mDrawTextures = !mDrawTextures;
		break;
	case KeyEvent::KEY_v:
		gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
		break;
	case KeyEvent::KEY_w:
		mDrawWireframe = !mDrawWireframe;
		break;
	case KeyEvent::KEY_SPACE:
		resetCamera();
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

	const int RES_X = 100;
	const int RES_Z = 50;
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

			indices.push_back( a ); indices.push_back( a + 1 ); indices.push_back( a + RES_Z );
			indices.push_back( a + RES_Z );  indices.push_back( a + 1 ); indices.push_back( a + RES_Z + 1 );
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
	fmt.setInternalFormat( GL_R32F );

	// create sinus texture
	Surface32f s( 1024, 1, false, SurfaceChannelOrder::CHAN_RED );
	Surface32f::Iter itr = s.getIter();
	while( itr.line() ) {
		while( itr.pixel() ) {
			Vec2i p = itr.getPos();
			float x = 0.5f + 0.5f * math<float>::sin( p.x / 1024.0f * float(2.0 * M_PI) );
			s.setPixel( p, Color(x, x, x) );
		}
	}

	mSinusTexture = gl::Texture( s, fmt );
}

CINDER_APP_BASIC( XMBApp, RendererGl )
