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
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PerspectiveWarpingApp : public App {
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

  public:
	//! set the size in pixels of the actual content
	void setContentSize( float width, float height );

	//! find the index of the nearest corner
	size_t getNearestIndex( const ivec2 &pt ) const;

	//! calculates a perspective transform matrix. Similar to OpenCV's getPerspectiveTransform() function
	mat4 getPerspectiveTransform( const vec2 src[4], const vec2 dst[4] ) const;

	//! helper function
	void gaussianElimination( float *a, int n ) const;

  private:
	//! TRUE if the transform matrix needs to be recalculated
	bool mIsInvalid = true;

	//! size in pixels of the actual content
	float mWidth;
	float mHeight;

	//! corners expressed in content coordinates
	vec2 mSource[4];
	//! corners expressed in window coordinates
	vec2 mDestination[4];
	//! corners expressed in normalized window coordinates
	vec2 mDestinationNormalized[4];

	//! warp transform matrix
	mat4 mTransform;

	//! content image
	gl::Texture2dRef mImage;

	//! edit variables
	bool mIsMouseDown = false;

	size_t mSelected = 0;

	ivec2 mInitialMouse;
	ivec2 mCurrentMouse;

	vec2 mInitialPosition;
};

void PerspectiveWarpingApp::prepare( Settings *settings )
{
	settings->setTitle( "Perspective Warping Sample" );
}

void PerspectiveWarpingApp::setup()
{
	// initialize transform
	mTransform = mat4();

	// set the size of the content (desired resolution)
	setContentSize( 1440, 1080 );

	// determine where the four corners should be,
	// expressed in normalized window coordinates
	mDestinationNormalized[0].x = 0.0f;
	mDestinationNormalized[0].y = 0.0f;
	mDestinationNormalized[1].x = 1.0f;
	mDestinationNormalized[1].y = 0.0f;
	mDestinationNormalized[2].x = 1.0f;
	mDestinationNormalized[2].y = 1.0f;
	mDestinationNormalized[3].x = 0.0f;
	mDestinationNormalized[3].y = 1.0f;

	// mouse is not down
	mIsMouseDown = false;

	// load image
	try {
		mImage = gl::Texture::create( loadImage( loadAsset( "USS Enterprise D TNG Season 1-2.jpg" ) ) );
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
	}
}

void PerspectiveWarpingApp::update()
{
	if( mIsInvalid ) {
		// calculate the actual four corners of the warp,
		// expressed as window coordinates
		const int w = getWindowWidth();
		const int h = getWindowHeight();

		mDestination[0].x = w * mDestinationNormalized[0].x;
		mDestination[0].y = h * mDestinationNormalized[0].y;
		mDestination[1].x = w * mDestinationNormalized[1].x;
		mDestination[1].y = h * mDestinationNormalized[1].y;
		mDestination[2].x = w * mDestinationNormalized[2].x;
		mDestination[2].y = h * mDestinationNormalized[2].y;
		mDestination[3].x = w * mDestinationNormalized[3].x;
		mDestination[3].y = h * mDestinationNormalized[3].y;

		// calculate warp matrix
		mTransform = getPerspectiveTransform( mSource, mDestination );

		mIsInvalid = false;
	}
}

void PerspectiveWarpingApp::draw()
{
	gl::clear( Color( 0.5f, 0.5f, 0.5f ) );

	// enable warp
	gl::pushModelMatrix();
	gl::multModelMatrix( mTransform );

	// draw content
	gl::color( Color::white() );
	if( mImage )
		gl::draw( mImage );

	gl::drawStrokedRect( Rectf( 0, 0, mWidth, mHeight ) );
	gl::drawLine( vec2( 0, 0 ), vec2( mWidth, mHeight ) );
	gl::drawLine( vec2( 0, mHeight ), vec2( mWidth, 0 ) );

	// disable warp
	gl::popModelMatrix();

	// draw currently dragged corner (if any)
	if( mIsMouseDown )
		gl::drawSolidCircle( vec2( mDestination[mSelected].x, mDestination[mSelected].y ), 3.0f );
}

void PerspectiveWarpingApp::resize()
{
	// simply recalculate the transformation matrix
	mIsInvalid = true;
}

void PerspectiveWarpingApp::mouseMove( MouseEvent event ) {}

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

	const ivec2 d = mCurrentMouse - mInitialMouse;
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

void PerspectiveWarpingApp::keyDown( KeyEvent event ) {}

void PerspectiveWarpingApp::keyUp( KeyEvent event ) {}

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

size_t PerspectiveWarpingApp::getNearestIndex( const ivec2 &pt ) const
{
	uint8_t index = 0;
	float   distance = 10.0e6f;

	for( uint8_t i = 0; i < 4; ++i ) {
		const float d = glm::distance( vec2( mDestination[i].x, mDestination[i].y ), vec2( pt ) );
		if( d < distance ) {
			distance = d;
			index = i;
		}
	}

	return index;
}

// Implementation of OpenCV's `getPerspectiveTransform()`. This way, we don't have to link against the OpenCV library.
// Adapted from code found here: http://forum.openframeworks.cc/t/quad-warping-homography-without-opencv/3121/19
mat4 PerspectiveWarpingApp::getPerspectiveTransform( const vec2 src[4], const vec2 dst[4] ) const
{
	float p[8][9] = {
		{ -src[0][0], -src[0][1], -1, 0, 0, 0, src[0][0] * dst[0][0], src[0][1] * dst[0][0], -dst[0][0] }, // h11
		{ 0, 0, 0, -src[0][0], -src[0][1], -1, src[0][0] * dst[0][1], src[0][1] * dst[0][1], -dst[0][1] }, // h12
		{ -src[1][0], -src[1][1], -1, 0, 0, 0, src[1][0] * dst[1][0], src[1][1] * dst[1][0], -dst[1][0] }, // h13
		{ 0, 0, 0, -src[1][0], -src[1][1], -1, src[1][0] * dst[1][1], src[1][1] * dst[1][1], -dst[1][1] }, // h21
		{ -src[2][0], -src[2][1], -1, 0, 0, 0, src[2][0] * dst[2][0], src[2][1] * dst[2][0], -dst[2][0] }, // h22
		{ 0, 0, 0, -src[2][0], -src[2][1], -1, src[2][0] * dst[2][1], src[2][1] * dst[2][1], -dst[2][1] }, // h23
		{ -src[3][0], -src[3][1], -1, 0, 0, 0, src[3][0] * dst[3][0], src[3][1] * dst[3][0], -dst[3][0] }, // h31
		{ 0, 0, 0, -src[3][0], -src[3][1], -1, src[3][0] * dst[3][1], src[3][1] * dst[3][1], -dst[3][1] }, // h32
	};

	gaussianElimination( &p[0][0], 9 );

	mat4 result = mat4( p[0][8], p[3][8], 0, p[6][8], p[1][8], p[4][8], 0, p[7][8], 0, 0, 1, 0, p[2][8], p[5][8], 0, 1 );

	return result;
}

// Adapted from code found here: http://forum.openframeworks.cc/t/quad-warping-homography-without-opencv/3121/19
void PerspectiveWarpingApp::gaussianElimination( float *a, int n ) const
{
	int       i = 0;
	int       j = 0;
	const int m = n - 1;

	while( i < m && j < n ) {
		int maxi = i;
		for( int k = i + 1; k < m; ++k ) {
			if( fabs( a[k * n + j] ) > fabs( a[maxi * n + j] ) ) {
				maxi = k;
			}
		}

		if( a[maxi * n + j] != 0 ) {
			if( i != maxi )
				for( int k = 0; k < n; k++ ) {
					const float aux = a[i * n + k];
					a[i * n + k] = a[maxi * n + k];
					a[maxi * n + k] = aux;
				}

			const float aIj = a[i * n + j];
			for( int k = 0; k < n; k++ ) {
				a[i * n + k] /= aIj;
			}

			for( int u = i + 1; u < m; u++ ) {
				const float aUj = a[u * n + j];
				for( int k = 0; k < n; k++ ) {
					a[u * n + k] -= aUj * a[i * n + k];
				}
			}

			++i;
		}
		++j;
	}

	for( i = m - 2; i >= 0; --i ) {
		for( j = i + 1; j < n - 1; j++ ) {
			a[i * n + m] -= a[i * n + j] * a[j * n + m];
		}
	}
}

CINDER_APP( PerspectiveWarpingApp, RendererGl, &PerspectiveWarpingApp::prepare )
