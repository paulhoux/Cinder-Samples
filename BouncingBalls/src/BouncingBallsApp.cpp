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
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;


//////////////////////////////////////////

typedef shared_ptr<class Ball> BallRef;

class Ball {
  public:
	Ball();

	void reset();

	void update();
	void draw( const gl::BatchRef &batch, bool useMotionBlur = true );

	bool isCollidingWith( BallRef other );
	void collideWith( BallRef other );

	bool isCollidingWithWindow();
	void collideWithWindow();

  public:
	static const int kRadius = 10;

  public:
	vec2 mPrevPosition;
	vec2 mPosition;
	vec2 mVelocity;

	Colorf mColor;

	float mGravity;

	bool mHasBeenDrawn;
};

Ball::Ball()
{
	// Pick a random color.
	float h = Rand::randFloat( 0.0f, 1.0f );
	float s = Rand::randFloat( 0.75f, 1.0f );
	float v = Rand::randFloat( 0.75f, 1.0f );
	mColor = Colorf( CM_HSV, h, s, v );

	reset();
}

void Ball::reset()
{
	// Pick a random position.
	float x = Rand::randFloat() * getWindowWidth();
	float y = -0.1f * getWindowHeight();
	mPosition = vec2( x, y );
	mPrevPosition = mPosition;

	// Note: you can use the multiplier to tweak the speed of the system.
	float multiplier = 0.5f;

	// Note: set gravity to zero for outer space.
	mGravity = 0.981f * multiplier;

	// Pick a random velocity.
	x = Rand::randFloat( -15.0f, 15.0f ) * multiplier;
	y = Rand::randFloat( -15.0f, 0.0f ) * multiplier;
	mVelocity = vec2( x, y );

	//
	mHasBeenDrawn = false;
}

void Ball::update()
{
	// Store current position.
	if( mHasBeenDrawn )
		mPrevPosition = mPosition;

	// First, update the ball's velocity.
	if( !isCollidingWithWindow() )
		mVelocity.y += mGravity;

	// Next, update the ball's position.
	mPosition += mVelocity;

	// Finally, perform collision detection.
	collideWithWindow();

	//
	mHasBeenDrawn = false;
}

void Ball::draw( const gl::BatchRef &batch, bool useMotionBlur )
{
	// Store the current model matrix.
	gl::pushModelMatrix();

	if( useMotionBlur ) {
		// Determine the number of balls that make up the motion blur trail (minimum of 3, maximum of 30).
		float trailsize = math<float>::clamp( math<float>::floor( glm::distance( mPrevPosition, mPosition ) ), 3.0f, 30.0f );
		float segments = trailsize - 1.0f;

		// Draw ball with motion blur (using additive blending).
		gl::ScopedColor color( mColor / trailsize );

		vec2 offset( 0.0f, 0.0f );
		for( size_t i = 0; i < trailsize; ++i ) {
			vec2 difference = ci::lerp<vec2>( mPrevPosition, mPosition, i / segments ) - offset;
			offset += difference;

			gl::translate( difference );
			batch->draw();
		}
	}
	else {
		// Draw ball without motion blur.
		gl::ScopedColor color( mColor );
		gl::translate( mPosition );
		batch->draw();
	}

	// Restore the model matrix.
	gl::popModelMatrix();

	//
	mHasBeenDrawn = true;
}

bool Ball::isCollidingWith( BallRef other )
{
	// This is a simplification: there is a change we will miss the collision.
	return ( glm::distance( mPosition, other->mPosition ) < ( 2 * kRadius ) );
}

void Ball::collideWith( BallRef other )
{
	static const float kMinimal = 2.0f * kRadius;

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
	if( velocity_a == velocity_b )
		return; // no collision will happen

	// 3) find time of collision
	float t = ( kMinimal - distance ) / ( velocity_b - velocity_a );

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
	if( mPosition.x < ( 0.0f + kRadius ) || mPosition.x > ( getWindowWidth() - kRadius ) )
		return true;
	if( mPosition.y > ( getWindowHeight() - kRadius ) )
		return true;

	return false;
}

void Ball::collideWithWindow()
{
	//	1) check if the ball hits the left or right side of the window
	if( mPosition.x < ( 0.0f + kRadius ) || mPosition.x > ( getWindowWidth() - kRadius ) ) {
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
	if( mPosition.y > ( getWindowHeight() - kRadius ) ) {
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
	if( mPosition.x < ( 0.0f + kRadius ) ) {
		mPosition.x = 0.0f + (float)kRadius;
		mVelocity.x = 0.0f;
	}
	else if( mPosition.x > ( getWindowWidth() - kRadius ) ) {
		mPosition.x = getWindowWidth() - (float)kRadius;
		mVelocity.x = 0.0f;
	}
	if( mPosition.y > ( getWindowHeight() - kRadius ) ) {
		mPosition.y = getWindowHeight() - (float)kRadius;
		mVelocity.y = 0.0f;
	}
}


//////////////////////////////////////////

class BouncingBallsApp : public App {
  public:
	void setup();
	void update();
	void draw();

	void keyDown( KeyEvent event );

  private:
	void performCollisions();

  private:
	bool mUseMotionBlur;
	bool mIsPaused;

	// our list of balls
	std::vector<BallRef> mBalls;

	// mesh and texture
	gl::VboMeshRef mMesh;
	gl::BatchRef   mBatch;
	gl::TextureRef mTexture;

	// default shader
	gl::GlslProgRef mShader;
};

void BouncingBallsApp::setup()
{
	// randomize the random generator
	Rand::randSeed( clock() );

	//
	mUseMotionBlur = true;
	mIsPaused = false;

	// allow maximum frame rate
	disableFrameRate();
	gl::enableVerticalSync( false );

	// create a few balls
	for( size_t i = 0; i < 25; ++i )
		mBalls.push_back( BallRef( new Ball() ) );

	// create a default shader with color and texture support
	mShader = gl::context()->getStockShader( gl::ShaderDef().color().texture() );

	// create ball mesh ( much faster than using gl::drawSolidCircle() )
	size_t slices = 20;

	std::vector<vec3>    positions;
	std::vector<vec2>    texcoords;
	std::vector<uint8_t> indices;

	texcoords.push_back( vec2( 0.5f, 0.5f ) );
	positions.push_back( vec3( 0 ) );

	for( size_t i = 0; i <= slices; ++i ) {
		float angle = i / (float)slices * 2.0f * (float)M_PI;
		vec2  v( sinf( angle ), cosf( angle ) );

		texcoords.push_back( vec2( 0.5f, 0.5f ) + 0.5f * v );
		positions.push_back( (float)Ball::kRadius * vec3( v, 0.0f ) );
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
}

void BouncingBallsApp::update()
{
	// Use a fixed time step for a steady 60 updates per second.
	static const double timestep = 1.0 / 60.0;

	// Keep track of time.
	static double time = getElapsedSeconds();
	static double accumulator = 0.0;

	// Calculate elapsed time since last frame.
	double elapsed = getElapsedSeconds() - time;
	time += elapsed;

	// Update the simulation.
	accumulator += math<double>::min( elapsed, 0.1 ); // prevents 'spiral of death'
	while( accumulator >= timestep ) {
		accumulator -= timestep;

		if( !mIsPaused ) {
			// Move the balls.
			for( auto &ball : mBalls )
				ball->update();

			// Perform collision detection and response.
			performCollisions();
		}
	}
}

void BouncingBallsApp::draw()
{
	gl::clear();

	gl::ScopedBlendAdditive blend;

	gl::ScopedGlslProg shader( mShader );
	mShader->uniform( "uTex0", 0 );

	gl::ScopedTextureBind tex0( mTexture );

	for( auto &ball : mBalls )
		ball->draw( mBatch, mUseMotionBlur );
}

void BouncingBallsApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	case KeyEvent::KEY_ESCAPE:
		// quit the application
		quit();
		break;
	case KeyEvent::KEY_SPACE:
		// reset all balls
		for( auto &ball : mBalls )
			ball->reset();
		break;
	case KeyEvent::KEY_RETURN:
		// pause/resume simulation
		mIsPaused = !mIsPaused;
		break;
	case KeyEvent::KEY_EQUALS: // For Macs without a keypad or a plus key
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

CINDER_APP( BouncingBallsApp, RendererGl )