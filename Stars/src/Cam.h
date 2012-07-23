#pragma once

#include "cinder/Vector.h"
#include "cinder/Camera.h"
#include "cinder/Timeline.h"
#include "cinder/app/AppBasic.h"

#include <deque>

class Cam {
public:
	Cam();
	Cam( const ci::CameraPersp &aInitialCam ) { mInitialCam = mCurrentCam = aInitialCam; }

	void		setup();
	void		update(double elapsed);

	void		mouseDown( const ci::Vec2i &mousePos );
	void		mouseDrag( const ci::Vec2i &mousePos, bool leftDown, bool middleDown, bool rightDown );
	void		mouseUp( const ci::Vec2i &mousePos );

	void		resize( ci::app::ResizeEvent event );

	const ci::CameraPersp& getCamera();
	void		setCurrentCam( const ci::CameraPersp &aCurrentCam );

	void		setDistanceTime(double time) { mTimeDistanceTarget = time; }

	//! returns the position of the camera in world space
	ci::Vec3f	getPosition();
private:
	//!
	double		average(const std::deque<double> &v){
		double avg = 0.0;

		for(std::deque<double>::const_iterator itr=v.begin();itr!=v.end();++itr) 
			avg += (*itr / v.size());

		return avg;
	};

	//!
	ci::Vec3d	average(const std::deque<ci::Vec3d> &v){
		ci::Vec3d avg(0.0, 0.0, 0.0);

		for(std::deque<ci::Vec3d>::const_iterator itr=v.begin();itr!=v.end();++itr) 
			avg += (*itr / v.size());

		return avg;
	};
private:
	static const double		LATITUDE_LIMIT;
	static const double		LATITUDE_THRESHOLD;
	static const double		DISTANCE_MIN;
	static const double		DISTANCE_MAX;

	ci::CameraPersp			mCurrentCam;

	ci::Vec2i				mInitialMousePos;
	ci::CameraPersp			mInitialCam;

	double					mDeltaX;
	double					mDeltaY;
	double					mDeltaD;
	std::deque<ci::Vec3d>	mDeltas;

	bool					mIsMouseDown;

	double					mTimeMouse;
	double					mTimeOut;

	double					mTimeDistanceTarget;
	double					mTimeDistance;

	ci::Anim<double>		mLatitude;
	ci::Anim<double>		mLongitude;
	ci::Anim<double>		mDistance;
	ci::Anim<double>		mFov;
};