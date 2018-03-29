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

#include "Cam.h"
#include "Conversions.h"

#include "cinder/CinderMath.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// initialize static members
const double Cam::LATITUDE_LIMIT = 89.0;
const double Cam::LATITUDE_THRESHOLD = 89.0;
const double Cam::DISTANCE_MIN = 0.001;
const double Cam::DISTANCE_MAX = 1000.0;

Cam::Cam( void )
    : mTimeMouse( 0 )
    , mTimeOut( 0 )
{
	mInitialCam = mCurrentCam = CameraStereo();
	mCurrentCam.setNearClip( 0.02f );
	mCurrentCam.setFarClip( 5000.0f );
	mCurrentCam.setEyeSeparation( 0.005f );

	mDeltaX = mDeltaY = mDeltaD = 0.0;
	mDeltas.clear();
	mIsMouseDown = false;
	mIsOriented = false;

	mLatitude = 0.0;
	mLongitude = 0.0;
	mDistance = DISTANCE_MIN;
	mFov = 40.0;

	mTimeDistance = 0.0;
	mTimeDistanceTarget = 0.0;
}

void Cam::setup()
{
	mTimeOut = 300.0;
	mTimeMouse = getElapsedSeconds() - mTimeOut;
}

void Cam::update( double elapsed )
{
	static const double sqrt2pi = math<double>::sqrt( 2.0 * M_PI );

	//
	mTimeDistance += 0.1 * Conversions::wrap( mTimeDistanceTarget - mTimeDistance, -0.5, 0.5 );

	if( ( getElapsedSeconds() - mTimeMouse ) > mTimeOut ) // screensaver mode
	{
		// calculate time
		double t = getElapsedSeconds();

		// determine distance to the sun (in parsecs)
		double fraction = (mTimeDistance)-math<double>::floor( mTimeDistance );
		double period = 2.0 * M_PI * math<double>::clamp( fraction * 1.20 - 0.10, 0.0, 1.0 );
		double f = cos( period );
		double distance = 100.0 - 99.99 * f;

		// determine where to look
		double longitude = t * 360.0 / 300.0;                              // go around once every 300 seconds
		double latitude = LATITUDE_LIMIT * -sin( t * 2.0 * M_PI / 220.0 ); // go up and down once every 220 seconds

		// determine interpolation factor
		t = math<double>::clamp( ( getElapsedSeconds() - mTimeMouse - mTimeOut ) / 100.0, 0.0, 1.0 );

		// interpolate over time to where we should be, so that the camera doesn't snap from user mode to screensaver mode
		mDistance = lerp<double>( mDistance.value(), distance, t );
		mLatitude = lerp<double>( mLatitude.value(), latitude, t );
		// to prevent rotating from 180 to -180 degrees, always rotate over the shortest distance
		mLongitude = mLongitude.value() + lerp<double>( 0.0, Conversions::wrap( longitude - mLongitude.value(), -180.0, 180.0 ), t );
	}
	else // user mode
	{
		// adjust for frame rate
		float t = (float)elapsed / 0.25f;
		if( t > 1.0f )
			t = 1.0f;

		// zoom in by decreasing distance to 102.5 units
		// mDistance = (1.0f - t) * mDistance.value() + t * 102.5f;

		if( !mIsMouseDown ) {
			// update longitude speed and value
			mDeltaX *= 0.975;
			mLongitude = Conversions::wrap( mLongitude.value() - mDeltaX, -180.0, 180.0 );

			// update latitude speed and value
			mDeltaY *= 0.975;
			mLatitude = math<double>::clamp( mLatitude.value() + mDeltaY, -LATITUDE_LIMIT, LATITUDE_LIMIT );

			// update distance
			// mDeltaD *= 0.975;
			// mDistance = math<double>::clamp( mDistance.value() + mDeltaD, DISTANCE_MIN, DISTANCE_MAX );

			// move latitude back to its threshold
			if( mLatitude.value() < -LATITUDE_THRESHOLD ) {
				mLatitude = 0.9f * mLatitude.value() + 0.1f * -LATITUDE_THRESHOLD;
				mDeltaY = 0.0;
			}
			else if( mLatitude.value() > LATITUDE_THRESHOLD ) {
				mLatitude = 0.9f * mLatitude.value() + 0.1f * LATITUDE_THRESHOLD;
				mDeltaY = 0.0;
			}
		}
		else {
			// add deltas to vector to determine average speed
			mDeltas.push_back( dvec3( mDeltaX, mDeltaY, mDeltaD ) );

			// only keep the last 6 deltas
			while( mDeltas.size() > 6 )
				mDeltas.pop_front();

			// reset deltas
			mDeltaX = 0.0;
			mDeltaY = 0.0;
			mDeltaD = 0.0;
		}
	}

	// focus camera
	mCurrentCam.setConvergence( math<float>::min( 1.0f, 0.95f * glm::length( mCurrentCam.getEyePoint() ) ), true );
}

void Cam::mouseDown( const ivec2 &mousePos )
{
	mInitialMousePos = mousePos;
	mTimeMouse = getElapsedSeconds();

	mInitialCam = mCurrentCam;

	mDeltaX = mDeltaY = mDeltaD = 0.0;
	mDeltas.clear();

	mIsMouseDown = true;
}

void Cam::mouseDrag( const ivec2 &mousePos, bool leftDown, bool middleDown, bool rightDown )
{
	double elapsed = getElapsedSeconds() - mTimeMouse;
	if( elapsed < ( 1.0 / 60.0 ) )
		return;

	double sensitivity = 0.075;

	if( leftDown ) {
		// adjust longitude (east-west)
		mDeltaX = ( mousePos.x - mInitialMousePos.x ) * sensitivity;
		mLongitude = Conversions::wrap( mLongitude.value() - mDeltaX, -180.0, 180.0 );
		// adjust latitude (north-south)
		mDeltaY = ( mousePos.y - mInitialMousePos.y ) * sensitivity;
		mLatitude = math<double>::clamp( mLatitude.value() + mDeltaY, -LATITUDE_LIMIT, LATITUDE_LIMIT );
	}
	else if( rightDown ) {
		mDeltaD = ( mousePos.x - mInitialMousePos.x ) + ( mousePos.y - mInitialMousePos.y );
		// adjust distance
		sensitivity = math<double>::max( 0.005, math<double>::log10( mDistance ) / math<double>::log10( 100.0 ) * 0.075 );
		mDistance = math<double>::clamp( mDistance.value() - mDeltaD * sensitivity, DISTANCE_MIN, DISTANCE_MAX );
	}

	mInitialMousePos = mousePos;
	mTimeMouse = getElapsedSeconds();
}

void Cam::mouseUp( const ivec2 &mousePos )
{
	mInitialMousePos = mousePos;
	mTimeMouse = getElapsedSeconds();

	mIsMouseDown = false;

	// calculate average delta (speed) over the last 10 frames
	// and use that to rotate the camera after mouseUp
	dvec3 avg = average( mDeltas );
	mDeltaX = avg.x;
	mDeltaY = avg.y;
	mDeltaD = avg.z;
}

void Cam::resize()
{
	mCurrentCam.setAspectRatio( getWindowAspectRatio() );
}

void Cam::setCurrentCam( const CameraStereo &aCurrentCam )
{
	mCurrentCam = aCurrentCam;

	// update distance and fov
	mDistance = glm::length( mCurrentCam.getEyePoint() );
	mFov = (double)mCurrentCam.getFov();
}

const CameraStereo &Cam::getCamera()
{
	// update current camera
	mCurrentCam.setFov( (float)mFov.value() );
	mCurrentCam.setEyePoint( getPosition() );

	if( !mIsOriented )
		mCurrentCam.lookAt( vec3( 0 ) );

	return mCurrentCam;
}

void Cam::setOrientation( const quat &orientation )
{
	mIsOriented = true;

	// If we're facing forward, we should always look at the sun.
	mCurrentCam.lookAt( vec3( 0 ) );
	mCurrentCam.setOrientation( mCurrentCam.getOrientation() * orientation );
}

vec3 Cam::getPosition()
{
	// calculates position based on current distance, longitude and latitude
	double theta = M_PI - toRadians( mLongitude.value() );
	double phi = M_PI / 2 - toRadians( mLatitude.value() );

	vec3 orientation( static_cast<float>( sin( phi ) * cos( theta ) ), static_cast<float>( cos( phi ) ), static_cast<float>( sin( phi ) * sin( theta ) ) );

	return static_cast<float>( mDistance.value() ) * orientation;
}
