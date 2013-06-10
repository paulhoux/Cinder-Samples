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

#include "cinder/DataSource.h"
#include "cinder/DataTarget.h"
#include "cinder/Utilities.h"

#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"

class Stars
{
public:
	// the Star class will later be used to read/write binary star data files
	class	Star
	{
	public:
		Star(void)
			: mDistance(0.0f), mMagnitude(0.0f), mColor( ci::Color::white() )
		{
			setPosition(0.0f, 0.0f);
		}

		Star( float ra, float dec, float parsecs, float magnitude, const ci::Color &color )
			: mDistance(parsecs), mMagnitude(magnitude), mColor(color) 
		{
			setPosition(ra, dec);
		}

		ci::Vec3f	getPosition() { return mDistance * mPosition; }
		void		setPosition( float ra, float dec ) {
						// convert to world (universe) coordinates
						float alpha = ci::toRadians( ra * 15.0f );
						float delta = ci::toRadians( dec );
						mPosition = ci::Vec3f( sinf(alpha) * cosf(delta), sinf(delta), cosf(alpha) * cosf(delta) );
					}

		float		getDistance() { return mDistance; }
		void		setDistance( float parsecs ) { mDistance = parsecs; }

		ci::Color	getColor() { return mColor; }
		void		setColor( const ci::Color &color ) { mColor = color; }

		float		getMagnitude() { return mMagnitude; }
		void		setMagnitude( float magnitude ) { mMagnitude = magnitude; }
	private:
		ci::Vec3f	mPosition;
		float		mDistance;
		float		mMagnitude;
		ci::Color	mColor;
	};

public:
	Stars(void);
	~Stars(void);

	void	setup();
	void	draw();

	void	clear();

	void	enablePointSprites();
	void	disablePointSprites();

	//
	float	getAspectRatio() const { return mAspectRatio; }
	void	setAspectRatio( float aspect ) { mAspectRatio = aspect; }

	//! load a comma separated file containing the HYG star database
	void	load( ci::DataSourceRef source );

	//! reads a binary star data file
	void	read( ci::DataSourceRef source );
	//! writes a binary star data file
	void	write( ci::DataTargetRef target );
private:
	void	createMesh();
private:
	ci::gl::GlslProg	mShader;
	ci::gl::Texture		mTextureStar;
	ci::gl::Texture		mTextureCorona;
	ci::gl::VboMesh		mVboMesh;

	std::vector< ci::Vec3f > mVertices;
	std::vector< ci::Vec2f > mTexcoords;
	std::vector< ci::Color > mColors;

	float				mAspectRatio;
	float				mScale;
};