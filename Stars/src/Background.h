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
};

