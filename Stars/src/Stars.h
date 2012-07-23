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

	void	enablePointSprites();
	void	disablePointSprites();

	//! load a comma separated file containing the HYG star database
	void	load( ci::DataSourceRef source );

	//! (TODO) will read a binary star data file
	void	read( ci::DataSourceRef source );
	//! (TODO) will write a binary star data file
	void	write( ci::DataTargetRef target );
private:
	ci::gl::GlslProg	mShader;
	ci::gl::Texture		mTextureStar;
	ci::gl::Texture		mTextureCorona;
	ci::gl::VboMesh		mVboMesh;
};

