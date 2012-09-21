#pragma once

#include "cinder/Vector.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"

class Background
{
public:
	Background(void);
	~Background(void);

	void	setup();
	void	draw();

	void	create();

	void	setCameraDistance( float distance );

	void	rotateX( float degrees ) { mRotation.x += degrees; ci::app::console() << mRotation << std::endl; }
	void	rotateY( float degrees ) { mRotation.y += degrees; ci::app::console() << mRotation << std::endl; }
	void	rotateZ( float degrees ) { mRotation.z += degrees; ci::app::console() << mRotation << std::endl; }
private:
	//! converts galactic coordinates (longitude, latitude) to equatorial coordinates (J2000: ra, dec)
	ci::Vec2d	toEquatorial( const ci::Vec2d &radians );
	//! converts equatorial coordinates (J2000: ra, dec) to galactic coordinates (longitude, latitude)
	ci::Vec2d	toGalactic( const ci::Vec2d &radians );

public:
	static const ci::Vec3d	GALACTIC_CENTER_EQUATORIAL;
	static const ci::Vec2d	GALACTIC_NORTHPOLE_EQUATORIAL;

private:
	//
	float				mAttenuation;
	ci::Vec3f			mRotation;

	ci::gl::Texture		mTexture;
	ci::gl::VboMesh		mVboMesh;

	ci::Matrix44f		mTransform;
};

