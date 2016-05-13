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
#include "cinder/app/App.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Texture.h"

class Background {
  public:
	Background( void );
	~Background( void );

	void setup();
	void draw();

	void create();

	void setCameraDistance( float distance );

  private:
	//! converts galactic coordinates (longitude, latitude) to equatorial coordinates (J2000: ra, dec)
	ci::dvec2 toEquatorial( const ci::dvec2 &radians );
	//! converts equatorial coordinates (J2000: ra, dec) to galactic coordinates (longitude, latitude)
	ci::dvec2 toGalactic( const ci::dvec2 &radians );

  public:
	static const ci::dvec3 GALACTIC_CENTER_EQUATORIAL;
	static const ci::dvec2 GALACTIC_NORTHPOLE_EQUATORIAL;

  private:
	//
	float    mAttenuation;
	ci::mat4 mTransform;

	ci::gl::Texture2dRef mTexture;
	ci::gl::BatchRef     mBatch;
};
