/*
 Copyright (c) 2010-2013, Paul Houx - All rights reserved.
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

#include "cinder/app/AppBasic.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class AnalogClockApp : public AppBasic {
public:
	// setting up and shutting down
	void prepareSettings( Settings *settings );
	void setup();

	// game loop
	void update();
	void draw();

	// application events	
	void keyDown( KeyEvent event );
protected:
	int getSecondsSinceMidnight();
protected:
	// your class members go here
};

// construction and destruction

void AnalogClockApp::prepareSettings( Settings *settings )
{
	settings->setTitle( "Analog Clock" );
	settings->setWindowSize( 300, 300 );
}

void AnalogClockApp::setup()
{
	// we don't need to load or setup any stuff in this application
}

// game loop

void AnalogClockApp::update()
{
	// instead of in the update method, we will do all 'animation' in the draw method
}

void AnalogClockApp::draw()
{
	// clear the window with a black background
	gl::clear( Color::black() );

	// get the center of the window
	vec2 center = 0.5f * vec2( getWindowSize() );

	// set current drawing color to white
	gl::color( Color::white() );

	// draw the 12 hour digits
	for( int h = 0; h < 12; ++h ) {
		// store the current transformation,
		// so we can 'undo' the translation and rotation
		// to get ready for the next digit
		gl::pushModelView();

		// draw a rectangle, rotated around the clock's center
		gl::translate( center );
		gl::rotate( h * glm::radians( 30.0f ) ); // 30 degrees per hour
		gl::drawSolidRect( Rectf( -3, -110, 3, -90 ) );

		// restore the transformations
		gl::popModelView();
	}

	// get the number of seconds since midnight
	int seconds = getSecondsSinceMidnight();

	// draw the long hand for the minutes
	gl::pushModelView();
	gl::translate( center );
	gl::rotate( seconds * glm::radians( 360.0f / 3600.0f ) ); // 360 degrees per 3600 seconds
	gl::drawSolidRect( Rectf( -2, -100, 2, 15 ) );
	gl::popModelView();

	// draw the short hand for the hours
	gl::pushModelView();
	gl::translate( center );
	gl::rotate( seconds * glm::radians( 30.0f / 3600.0f ) ); // 30 degrees per 3600 seconds
	gl::drawSolidRect( Rectf( -2, -60, 2, 15 ) );
	gl::popModelView();

	// set the current drawing color to red
	gl::color( Color( 1, 0, 0 ) );

	// draw the hand for the seconds
	gl::pushModelView();
	gl::translate( center );
	gl::rotate( seconds * glm::radians( 360.0f / 60.0f ) ); // 360 degrees per 60 seconds
	gl::drawSolidRect( Rectf( -1, -100, 1, 15 ) );
	gl::drawSolidCircle( vec2( 0 ), 6, 24 );
	gl::popModelView();
}

// key events

void AnalogClockApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	}
}

int AnalogClockApp::getSecondsSinceMidnight()
{
	auto t = std::time( nullptr );
	auto d = std::localtime( &t );

	return int( d->tm_hour * 3600 + d->tm_min * 60 + d->tm_sec );
}

// the following macro will create the application
CINDER_APP_BASIC( AnalogClockApp, RendererGl )
