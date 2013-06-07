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
#include "cinder/gl/Texture.h"

// this sample needs the Cinder OpenCV block, which comes with the release
// version of Cinder, but should be installed separately if you use the GitHub version.
// See the README.md file for more information.
#include "CinderOpenCV.h"

// On Windows, an easy way to add all required openCV libraries is to add them
// using the #pragma directive
#ifdef CINDER_MSW
#ifndef _DEBUG
	#pragma comment(lib, "opencv_core243.lib")
	#pragma comment(lib, "opencv_imgproc243.lib")
#else
	#pragma comment(lib, "opencv_core243d.lib")
	#pragma comment(lib, "opencv_imgproc243d.lib")
#endif
#endif

using namespace ci;
using namespace ci::app;
using namespace std;

class PerspectiveWarpingApp : public AppBasic {
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
public:
	//! set the size in pixels of the actual content
	void	setContentSize( float width, float height );

	//! find the index of the nearest corner
	size_t	getNearestIndex( const Vec2i &pt );
private:
	//! TRUE if the transform matrix needs to be recalculated
	bool		mIsInvalid;

	//! size in pixels of the actual content
	float		mWidth;
	float		mHeight;

	//! corners expressed in content coordinates
	cv::Point2f	mSource[4];
	//! corners expressed in window coordinates
	cv::Point2f	mDestination[4];
	//! corners expressed in normalized window coordinates
	Vec2f		mDestinationNormalized[4];

	//! warp transform matrix
	Matrix44d	mTransform;

	//! content image
	gl::Texture	mImage;

	//! edit variables
	bool		mIsMouseDown;

	size_t		mSelected;

	Vec2i		mInitialMouse;
	Vec2i		mCurrentMouse;

	cv::Point2f	mInitialPosition;
};

void PerspectiveWarpingApp::prepareSettings(Settings *settings)
{
	settings->setTitle("Perspective Warping Sample");
}

void PerspectiveWarpingApp::setup()
{
	// initialize transform
	mTransform.setToIdentity();
	
	// set the size of the content (desired resolution)
	setContentSize( 1440, 1080 );

	// determine where the four corners should be,
	// expressed in normalized window coordinates
	mDestinationNormalized[0].x = 0.0f;	mDestinationNormalized[0].y = 0.0f;
	mDestinationNormalized[1].x = 1.0f;	mDestinationNormalized[1].y = 0.0f;
	mDestinationNormalized[2].x = 1.0f;	mDestinationNormalized[2].y = 1.0f;
	mDestinationNormalized[3].x = 0.0f;	mDestinationNormalized[3].y = 1.0f;

	// mouse is not down
	mIsMouseDown = false;

	// load image
	try {	mImage = gl::Texture( loadImage( loadAsset("USS Enterprise D TNG Season 1-2.jpg") ) );	}
	catch( const std::exception &e ) { console() << e.what() << std::endl; }
}

void PerspectiveWarpingApp::update()
{
	if( mIsInvalid )
	{
		// calculate the actual four corners of the warp,
		// expressed as window coordinates
		int w = getWindowWidth();
		int h = getWindowHeight();

		mDestination[0].x = w * mDestinationNormalized[0].x;	mDestination[0].y = h * mDestinationNormalized[0].y;
		mDestination[1].x = w * mDestinationNormalized[1].x;	mDestination[1].y = h * mDestinationNormalized[1].y;
		mDestination[2].x = w * mDestinationNormalized[2].x;	mDestination[2].y = h * mDestinationNormalized[2].y;
		mDestination[3].x = w * mDestinationNormalized[3].x;	mDestination[3].y = h * mDestinationNormalized[3].y;

		// calculate warp matrix
		cv::Mat	warp = cv::getPerspectiveTransform( mSource, mDestination );

		// convert to OpenGL matrix
		mTransform[0]	= warp.ptr<double>(0)[0]; 
		mTransform[4]	= warp.ptr<double>(0)[1]; 
		mTransform[12]	= warp.ptr<double>(0)[2]; 

		mTransform[1]	= warp.ptr<double>(1)[0]; 
		mTransform[5]	= warp.ptr<double>(1)[1]; 
		mTransform[13]	= warp.ptr<double>(1)[2]; 

		mTransform[3]	= warp.ptr<double>(2)[0]; 
		mTransform[7]	= warp.ptr<double>(2)[1]; 
		mTransform[15]	= warp.ptr<double>(2)[2]; 

		mIsInvalid = false;
	}
}

void PerspectiveWarpingApp::draw()
{
	gl::clear( Color(0.5f, 0.5f, 0.5f) ); 

	// enable warp
	gl::pushModelView();
	gl::multModelView( mTransform );

	// draw content
	gl::color( Color::white() );
	if( mImage ) gl::draw( mImage );

	gl::drawStrokedRect( Rectf(0, 0, mWidth, mHeight) );
	gl::drawLine( Vec2f( 0, 0 ), Vec2f( mWidth, mHeight ) );
	gl::drawLine( Vec2f( 0, mHeight ), Vec2f( mWidth, 0 ) );

	// disable warp
	gl::popModelView();

	// draw currently dragged corner (if any)
	if( mIsMouseDown )
		gl::drawSolidCircle( Vec2f( mDestination[mSelected].x, mDestination[mSelected].y ), 3.0f );
}

void PerspectiveWarpingApp::resize()
{
	// simply recalculate the transformation matrix
	mIsInvalid = true;
}

void PerspectiveWarpingApp::mouseMove( MouseEvent event )
{
}

void PerspectiveWarpingApp::mouseDown( MouseEvent event )
{
	// start dragging the nearest corner
	mIsMouseDown = true;
	mInitialMouse = mCurrentMouse = event.getPos();
	
	mSelected = getNearestIndex( mInitialMouse );
	mInitialPosition = mDestination[mSelected];
}

void PerspectiveWarpingApp::mouseDrag( MouseEvent event )
{
	// drag the nearest corner
	mCurrentMouse = event.getPos();

	Vec2i d = mCurrentMouse - mInitialMouse;
	mDestination[mSelected].x = mInitialPosition.x + d.x;
	mDestination[mSelected].y = mInitialPosition.y + d.y;

	// don't forget to also update the normalized destination
	mDestinationNormalized[mSelected].x = mDestination[mSelected].x / getWindowWidth();
	mDestinationNormalized[mSelected].y = mDestination[mSelected].y / getWindowHeight();

	// recalculate transform matrix
	mIsInvalid = true;
}

void PerspectiveWarpingApp::mouseUp( MouseEvent event )
{
	// stop dragging the nearest corner
	mIsMouseDown = false;
}

void PerspectiveWarpingApp::keyDown( KeyEvent event )
{
}

void PerspectiveWarpingApp::keyUp( KeyEvent event )
{
}

void PerspectiveWarpingApp::setContentSize( float width, float height )
{
	//! width and height of the CONTENT of your warp (desired resolution)
	mWidth = width;
	mHeight = height;

	//! set the four corners of the CONTENT
	mSource[0].x = 0.0f;		
	mSource[0].y = 0.0f;		
	mSource[1].x = mWidth;		
	mSource[1].y = 0.0f;		
	mSource[2].x = mWidth;		
	mSource[2].y = mHeight;		
	mSource[3].x = 0.0f;		
	mSource[3].y = mHeight;	

	//! we need to recalculate the warp transform
	mIsInvalid = true;
}

size_t PerspectiveWarpingApp::getNearestIndex( const Vec2i &pt )
{
	uint8_t	index = 0;
	float	distance = 10.0e6f;

	for(size_t i=0;i<4;++i)
	{
		float d = Vec2f( mDestination[i].x, mDestination[i].y ).distance( Vec2f(pt) );
		if( d < distance ) 
		{
			distance = d;
			index = i;
		}
	}

	return index;
}

CINDER_APP_BASIC( PerspectiveWarpingApp, RendererGl )
