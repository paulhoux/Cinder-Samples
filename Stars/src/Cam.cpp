#include "Cam.h"
#include "Conversions.h"

#include "cinder/CinderMath.h"
#include "cinder/Utilities.h"
#include "cinder/app/AppBasic.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// initialize static members
const double Cam::LATITUDE_LIMIT = 89.0;
const double Cam::LATITUDE_THRESHOLD = 89.0;
const double Cam::DISTANCE_MIN = 0.015;
const double Cam::DISTANCE_MAX = 1000.0;

Cam::Cam(void)
{
	mInitialCam = mCurrentCam = CameraPersp(); 
	mCurrentCam.setNearClip( 0.01f );
	mCurrentCam.setFarClip( 5000.0f );

	mDeltaX = mDeltaY = mDeltaD = 0.0; 
	mDeltas.clear();
	mIsMouseDown = false;

	mLatitude = 0.0;
	mLongitude = 0.0;
	mDistance = 0.015;
	mFov = 60.0;

	mTimeDistance = 0.0;
	mTimeDistanceTarget = 0.0;
}

void Cam::setup()
{
	mTimeOut = 300.0;
	mTimeMouse = getElapsedSeconds() - mTimeOut;
}

void Cam::update(double elapsed)
{
	static const double sqrt2pi = math<double>::sqrt(2.0 * M_PI);

	//
	mTimeDistance +=  0.1 * Conversions::wrap( mTimeDistanceTarget - mTimeDistance, -0.5, 0.5 );

	if((getElapsedSeconds() - mTimeMouse) > mTimeOut)	// screensaver mode
	{		
		// calculate time 
		double t = getElapsedSeconds();

		// determine distance to the sun (in parsecs)
		double fraction = (mTimeDistance) - math<double>::floor(mTimeDistance);
		double period = 2.0 * M_PI * math<double>::clamp( fraction * 1.20 - 0.10, 0.0, 1.0 );
		double f = cos( period );
		double distance = 500.0 - 499.95 * f;

		// determine where to look
		double longitude = t * 360.0 / 300.0;							// go around once every 300 seconds
		double latitude = LATITUDE_LIMIT * -sin(t * 2.0 * M_PI / 220.0);// go up and down once every 220 seconds

		// determine interpolation factor
		t = math<double>::clamp( (getElapsedSeconds() - mTimeMouse - mTimeOut) / 100.0, 0.0, 1.0);

		// interpolate over time to where we should be, so that the camera doesn't snap from user mode to screensaver mode
		mDistance = lerp<double>( mDistance.value(), distance, t);
		mLatitude = lerp<double>( mLatitude.value(), latitude, t ); 
		// to prevent rotating from 180 to -180 degrees, always rotate over the shortest distance
		mLongitude = mLongitude.value() + lerp<double>( 0.0, Conversions::wrap(longitude - mLongitude.value(), -180.0, 180.0), t );
	}
	else	// user mode
	{		
		// adjust for frame rate
		float t = (float) elapsed / 0.25f;
		if(t > 1.0f) t = 1.0f;

		// zoom in by decreasing distance to 102.5 units
		//mDistance = (1.0f - t) * mDistance.value() + t * 102.5f;

		if(!mIsMouseDown) {
			// update longitude speed and value
			mDeltaX *= 0.975;
			mLongitude = Conversions::wrap( mLongitude.value() - mDeltaX, -180.0, 180.0 ); 

			// update latitude speed and value
			mDeltaY *= 0.975;
			mLatitude = math<double>::clamp( mLatitude.value() + mDeltaY, -LATITUDE_LIMIT, LATITUDE_LIMIT );

			// update distance
			//mDeltaD *= 0.975;
			//mDistance = math<double>::clamp( mDistance.value() + mDeltaD, DISTANCE_MIN, DISTANCE_MAX );

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
			mDeltas.push_back( Vec3d(mDeltaX, mDeltaY, mDeltaD) );

			// only keep the last 6 deltas
			while(mDeltas.size() > 6) mDeltas.pop_front();

			// reset deltas
			mDeltaX = 0.0;
			mDeltaY = 0.0;
			mDeltaD = 0.0;
		}

		// adjust field-of-view to 60 degrees
		//mFov = (1.0f - t) * mFov.value() + t * 60.0f;
	}
}

void Cam::mouseDown( const Vec2i &mousePos )
{
	mInitialMousePos = mousePos;
	mTimeMouse = getElapsedSeconds();

	mInitialCam = mCurrentCam;

	mDeltaX = mDeltaY = mDeltaD = 0.0;
	mDeltas.clear();

	mIsMouseDown = true;
}

void Cam::mouseDrag( const Vec2i &mousePos, bool leftDown, bool middleDown, bool rightDown )
{
	double elapsed = getElapsedSeconds() - mTimeMouse;
	if(elapsed < (1.0/60.0)) return;

	double sensitivity = 0.075;

	if(leftDown) {
		// adjust longitude (east-west)
		mDeltaX = (mousePos.x - mInitialMousePos.x) * sensitivity;
		mLongitude = Conversions::wrap( mLongitude.value() - mDeltaX, -180.0, 180.0 ); 
		// adjust latitude (north-south)
		mDeltaY = (mousePos.y - mInitialMousePos.y) * sensitivity;
		mLatitude = math<double>::clamp( mLatitude.value() + mDeltaY, -LATITUDE_LIMIT, LATITUDE_LIMIT );
	}
	else if(rightDown) {
		mDeltaD = ( mousePos.x - mInitialMousePos.x ) + ( mousePos.y - mInitialMousePos.y );
		// adjust distance
		sensitivity = math<double>::max(0.005, math<double>::log10(mDistance) / math<double>::log10(100.0) * 0.075);
		mDistance = math<double>::clamp( mDistance.value() - mDeltaD * sensitivity, DISTANCE_MIN, DISTANCE_MAX );
	}

	mInitialMousePos = mousePos;
	mTimeMouse = getElapsedSeconds();
}

void Cam::mouseUp(  const Vec2i &mousePos  ) 
{	
	mInitialMousePos = mousePos;
	mTimeMouse = getElapsedSeconds();

	mIsMouseDown = false;

	// calculate average delta (speed) over the last 10 frames 
	// and use that to rotate the camera after mouseUp
	Vec3d avg = average(mDeltas);
	mDeltaX = avg.x;
	mDeltaY = avg.y;
	mDeltaD = avg.z;
}

void Cam::resize( ResizeEvent event )
{
	mCurrentCam.setAspectRatio(event.getAspectRatio());
}

const CameraPersp& Cam::getCamera()  
{
	// update current camera
	mCurrentCam.setFov( mFov.value() );
	mCurrentCam.setEyePoint( getPosition() );
	mCurrentCam.setCenterOfInterestPoint( Vec3f::zero() );

	return mCurrentCam;
}

void Cam::setCurrentCam( const CameraPersp &aCurrentCam ) 
{ 
	mCurrentCam = aCurrentCam;

	// update distance and fov
	mDistance = mCurrentCam.getEyePoint().length();
	mFov = mCurrentCam.getFov();
}

Vec3f Cam::getPosition()
{
	// calculates position based on current distance, longitude and latitude
	double theta = M_PI - toRadians(mLongitude.value());
	double phi = 0.5 * M_PI - toRadians(mLatitude.value());

	Vec3f orientation( 
		static_cast<float>( sin(phi) * cos(theta) ),
		static_cast<float>( cos(phi) ),
		static_cast<float>( sin(phi) * sin(theta) ) );

	return mDistance.value() * orientation; 
}