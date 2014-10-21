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
#include "cinder/Rand.h"
#include "cinder/Timer.h"
#include "cinder/app/AppBasic.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/VboMesh.h"

using namespace ci;
using namespace ci::app;
using namespace std;


//////////////////////////////////////////

typedef shared_ptr<class Ball> BallRef;

class Ball {
public:
	Ball();

	void	reset();

	void	update();
	void	draw( const gl::BatchRef &batch, bool useMotionBlur = true );

	bool	isCollidingWith( BallRef other );
	void	collideWith( BallRef other );

	bool	isCollidingWithWindow();
	void	collideWithWindow();
public:
	static const int RADIUS = 10;
public:
	vec2	mPrevPosition;
	vec2	mPosition;
	vec2	mVelocity;

	Colorf	mColor;

	float	mGravity;

	bool	mHasBeenDrawn;
};

Ball::Ball()
{
	// pick a random color
	float h = Rand::randFloat( 0.0f, 1.0f );
	float s = Rand::randFloat( 0.75f, 1.0f );
	float v = Rand::randFloat( 0.75f, 1.0f );
	mColor = Colorf( CM_HSV, h, s, v );

	reset();
}

void Ball::reset()
{
	// pick a random position
	float x = Rand::randFloat() * getWindowWidth();
	float y = -0.1f * getWindowHeight();
	mPosition = vec2( x, y );
	mPrevPosition = mPosition;

	// note: you can use the multiplier to tweak the speed of the system
	float multiplier = 0.5f;

	// note: set gravity to zero for outer space
	mGravity = 0.981f * multiplier;

	// pick a random velocity
	x = Rand::randFloat( -15.0f, 15.0f ) * multiplier;
	y = Rand::randFloat( -15.0f, 0.0f ) * multiplier;
	mVelocity = vec2( x, y );

	// 
	mHasBeenDrawn = false;
}

void Ball::update()
{
	// store current position
	if( mHasBeenDrawn ) mPrevPosition = mPosition;

	// first, update the ball's velocity
	if( !isCollidingWithWindow() ) mVelocity.y += mGravity;

	// next, update the ball's position
	mPosition += mVelocity;

	// finally, perform collision detection:
	collideWithWindow();

	//
	mHasBeenDrawn = false;
}

void Ball::draw( const gl::BatchRef &batch, bool useMotionBlur )
{
	// store the current modelview matrix
	gl::pushModelView();

	if( useMotionBlur ) {
		// determine the number of balls that make up the motion blur trail (minimum of 3, maximum of 30)
		float trailsize = math<float>::clamp( math<float>::floor( glm::distance( mPrevPosition, mPosition ) ), 3.0f, 30.0f );
		float segments = trailsize - 1.0f;

		// draw ball with motion blur (using additive blending)
		gl::color( mColor / trailsize );

		vec2 offset( 0.0f, 0.0f );
		for( size_t i = 0; i < trailsize; ++i ) {
			vec2 difference = ci::lerp<vec2>( mPrevPosition, mPosition, i / segments ) - offset;
			offset += difference;

			gl::translate( difference );
			batch->draw();
		}
	}
	else {
		// draw ball without motion blur
		gl::color( mColor );
		gl::translate( mPosition );
		batch->draw();
	}

	// restore the modelview matrix
	gl::popModelView();

	//
	mHasBeenDrawn = true;
}

bool Ball::isCollidingWith( BallRef other )
{
	// this is a simplification: there is a change we will miss the collision
	return ( glm::distance( mPosition, other->mPosition ) < ( 2 * RADIUS ) );
}

void Ball::collideWith( BallRef other )
{
	// calculate minimal distance between balls
	static float minimal = 2 * RADIUS;

	// 1) we have already established that the two balls are colliding,
	// let's move back in time to the moment before collision
	mPosition -= mVelocity;
	other->mPosition -= other->mVelocity;

	// 2) convert to simple 1-dimensional collision 
	//	by projecting onto line through both centers
	vec2 line = other->mPosition - mPosition;
	vec2 unit = glm::normalize( line );

	float distance = glm::dot( line, unit );
	float velocity_a = glm::dot( mVelocity, unit );
	float velocity_b = glm::dot( other->mVelocity, unit );
	if( velocity_a == velocity_b ) return; // no collision will happen

	// 3) find time of collision
	float t = ( minimal - distance ) / ( velocity_b - velocity_a );

	// 4) move to that moment in time
	mPosition += t * mVelocity;
	other->mPosition += t * other->mVelocity;

	// 5) exchange velocities 	
	mVelocity -= velocity_a * unit;
	mVelocity += velocity_b * unit;

	other->mVelocity -= velocity_b * unit;
	other->mVelocity += velocity_a * unit;

	// 6) move forward to current time 
	mPosition += ( 1.0f - t ) * mVelocity;
	other->mPosition += ( 1.0f - t ) * other->mVelocity;

	// 7) make sure the balls stay within window
	collideWithWindow();
	other->collideWithWindow();
}

bool Ball::isCollidingWithWindow()
{
	if( mPosition.x < ( 0.0f + RADIUS ) || mPosition.x >( getWindowWidth() - RADIUS ) ) return true;
	if( mPosition.y > ( getWindowHeight() - RADIUS ) ) return true;

	return false;
}

void Ball::collideWithWindow()
{
	//	1) check if the ball hits the left or right side of the window
	if( mPosition.x < ( 0.0f + RADIUS ) || mPosition.x >( getWindowWidth() - RADIUS ) ) {
		// to reduce the visual effect of the ball missing the border, 
		// set the previous position to where we are now
		mPrevPosition = mPosition;
		// move the ball back into window without adding energy,
		// by placing it where it would have been without friction
		mPosition.x -= mVelocity.x;
		// reduce velocity due to friction
		mVelocity.x *= -0.95f;
	}
	//	2) check if the ball this the bottom of the window
	if( mPosition.y > ( getWindowHeight() - RADIUS ) ) {
		// to reduce the visual effect of the ball missing the border, 
		// set the previous position to where we are now
		mPrevPosition = mPosition;
		// move the ball back into window without adding energy,
		// by placing it where it would have been without friction
		mPosition.y -= mVelocity.y;
		// reduce velocity due to friction
		mVelocity.x *= 0.95f;
		mVelocity.y *= -0.9f;
	}

	//  3) if ball is still outside window, 
	//		it was probably moving very slow or fast. Let's reset it then.
	if( mPosition.x < ( 0.0f + RADIUS ) ) {
		mPosition.x = 0.0f + (float) RADIUS;
		mVelocity.x = 0.0f;
	}
	else if( mPosition.x > ( getWindowWidth() - RADIUS ) ) {
		mPosition.x = getWindowWidth() - (float) RADIUS;
		mVelocity.x = 0.0f;
	}
	if( mPosition.y > ( getWindowHeight() - RADIUS ) ) {
		mPosition.y = getWindowHeight() - (float) RADIUS;
		mVelocity.y = 0.0f;
	}
}


//////////////////////////////////////////

class BouncingBallsApp : public AppBasic {
public:
	void setup();
	void update();
	void draw();

	void keyDown( KeyEvent event );
private:
	void performCollisions();
private:
	bool		mUseMotionBlur;

	// simulation timer
	uint32_t	mStepsPerSecond;
	uint32_t	mStepsPerformed;
	Timer		mTimer;

	// our list of balls 
	std::vector<BallRef> mBalls;

	// mesh and texture
	gl::VboMeshRef  mMesh;
	gl::BatchRef    mBatch;
	gl::TextureRef  mTexture;

	// default shader
	gl::GlslProgRef mShader;
};

void BouncingBallsApp::setup()
{
	// randomize the random generator
	Rand::randSeed( clock() );

	//
	mUseMotionBlur = true;

	// set some kind of sensible maximum to the frame rate
	setFrameRate( 100.0f );

	// initialize simulator
	mStepsPerSecond = 60;
	mStepsPerformed = 0;

	// create a few balls
	for( size_t i = 0; i < 25; ++i )
		mBalls.push_back( BallRef( new Ball() ) );

	// create a default shader with color and texture support
	mShader = gl::context()->getStockShader( gl::ShaderDef().color().texture() );

	// create ball mesh ( much faster than using gl::drawSolidCircle() )
	size_t slices = 20;

	std::vector<vec3> positions;
	std::vector<vec2> texcoords;
	std::vector<uint8_t> indices;

	texcoords.push_back( vec2( 0.5f, 0.5f ) );
	positions.push_back( vec3( 0 ) );

	for( size_t i = 0; i <= slices; ++i ) {
		float angle = i / (float) slices * 2.0f * (float) M_PI;
		vec2 v( sinf( angle ), cosf( angle ) );

		texcoords.push_back( vec2( 0.5f, 0.5f ) + 0.5f * v );
		positions.push_back( (float) Ball::RADIUS * vec3( v, 0.0f ) );
	}

	gl::VboMesh::Layout layout;
	layout.usage( GL_STATIC_DRAW );
	layout.attrib( geom::Attrib::POSITION, 3 );
	layout.attrib( geom::Attrib::TEX_COORD_0, 2 );

	mMesh = gl::VboMesh::create( positions.size(), GL_TRIANGLE_FAN, { layout } );
	mMesh->bufferAttrib( geom::POSITION, positions.size() * sizeof( vec3 ), positions.data() );
	mMesh->bufferAttrib( geom::TEX_COORD_0, texcoords.size() * sizeof( vec2 ), texcoords.data() );

	// combine mesh and shader into batch for much better performance
	mBatch = gl::Batch::create( mMesh, mShader );

	// load texture
	mTexture = gl::Texture::create( loadImage( loadAsset( "ball.png" ) ) );

	// start simulation
	mTimer.start();
}

void BouncingBallsApp::update()
{
	// determine how many simulation steps should have been performed until now
	uint32_t stepsTotal = static_cast<uint32_t>( floor( mTimer.getSeconds() * mStepsPerSecond ) );

	// if too far behind, skip a bit
	if( ( stepsTotal - mStepsPerformed ) > mStepsPerSecond )
		mStepsPerformed = stepsTotal - mStepsPerSecond;

	// kill-switch
	double t = mTimer.getSeconds() + 1.0;

	// perform the remaining steps
	std::vector<BallRef>::iterator itr;
	while( mStepsPerformed < stepsTotal && mTimer.getSeconds() < t ) {
		// move the balls
		for( itr = mBalls.begin(); itr != mBalls.end(); ++itr )
			( *itr )->update();

		// perform collision detection and response
		performCollisions();

		// done
		mStepsPerformed++;
	}

	// in case the kill-switch was activated
	mStepsPerformed = stepsTotal;
}

void BouncingBallsApp::draw()
{
	gl::clear();
	gl::enableAdditiveBlending();

	gl::ScopedGlslProg shader( mShader );
	gl::ScopedTextureBind tex0( mTexture );

	mShader->uniform( "uTex0", 0 );

	std::vector<BallRef>::iterator itr;
	for( itr = mBalls.begin(); itr != mBalls.end(); ++itr )
		( *itr )->draw( mBatch, mUseMotionBlur );

	gl::disableAlphaBlending();
}

void BouncingBallsApp::keyDown( KeyEvent event )
{
	std::vector<BallRef>::iterator itr;

	switch( event.getCode() ) {
	case KeyEvent::KEY_ESCAPE:
		// quit the application
		quit();
		break;
	case KeyEvent::KEY_SPACE:
		// reset all balls
		for( itr = mBalls.begin(); itr != mBalls.end(); ++itr )
			( *itr )->reset();
		break;
	case KeyEvent::KEY_RETURN:
		// pause/resume simulation
		if( mTimer.isStopped() ) {
			mStepsPerformed = 0;
			mTimer.start();
		}
		else mTimer.stop();
		break;
	case KeyEvent::KEY_EQUALS: //For Macs without a keypad or a plus key
		if( !event.isShiftDown() ) {
			break;
		}
	case KeyEvent::KEY_PLUS:
	case KeyEvent::KEY_KP_PLUS:
		// create a new ball
		mBalls.push_back( BallRef( new Ball() ) );
		break;
	case KeyEvent::KEY_MINUS:
	case KeyEvent::KEY_KP_MINUS:
		// remove the oldest ball
		if( !mBalls.empty() )
			mBalls.erase( mBalls.begin() );
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_v:
		gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
		break;
	case KeyEvent::KEY_m:
		mUseMotionBlur = !mUseMotionBlur;
		break;
	case KeyEvent::KEY_1:
		setFrameRate( 10.0f );
		break;
	case KeyEvent::KEY_2:
		setFrameRate( 20.0f );
		break;
	case KeyEvent::KEY_3:
		setFrameRate( 30.0f );
		break;
	case KeyEvent::KEY_4:
		setFrameRate( 100.0f );
		break;
	}
}

void BouncingBallsApp::performCollisions()
{
	// determine which balls are colliding,
	// by checking every pair of balls only once
	std::vector<BallRef>::iterator itr1, itr2;
	for( itr1 = mBalls.begin(); itr1 < mBalls.end() - 1; ++itr1 ) {
		for( itr2 = itr1 + 1; itr2 < mBalls.end(); ++itr2 ) {
			// do quick check
			if( ( *itr1 )->isCollidingWith( *itr2 ) ) {
				// do collision
				( *itr1 )->collideWith( *itr2 );
			}
		}
	}
}

CINDER_APP_BASIC( BouncingBallsApp, RendererGl )