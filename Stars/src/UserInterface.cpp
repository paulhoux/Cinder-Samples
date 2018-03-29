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

#include "UserInterface.h"

#include "cinder/app/App.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/scoped.h"
#include "text/FontStore.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace ph;

UserInterface::UserInterface( void )
    : mDistance( 0.0f )
{
}

UserInterface::~UserInterface( void ) {}

void UserInterface::setup()
{
	text::fonts().loadFont( loadAsset( "fonts/Ubuntu-BoldItalic.sdff" ) );
	mBox.setFont( text::fonts().getFont( "Ubuntu-BoldItalic" ) );
	mBox.setFontSize( 24.0f );
	mBox.setBoundary( text::Text::WORD );
	mBox.setAlignment( text::Text::CENTER );
	mBox.setSize( 800, 100 );

	mText = std::string( "%.0f lightyears from the Sun" );
}

void UserInterface::draw( const std::string &text )
{
	gl::disableDepthRead();
	gl::disableDepthWrite();

	gl::ScopedBlendAlpha blend;
	gl::ScopedColor      color( 1, 1, 1 );

	auto viewport = gl::getViewport();
	vec2 position = vec2( 0.5f * viewport.second.x, 0.92f * viewport.second.y );

	gl::pushMatrices();
	{
		gl::setMatricesWindow( viewport.second );

		gl::translate( position );

		float s = math<float>::min( 1000.0f, viewport.second.x ) / 1000.0f;
		gl::scale( s, s );

		gl::ScopedGlslProg shader( gl::context()->getStockShader( gl::ShaderDef().color() ) );
		gl::drawLine( vec2( -400, 0.5f ), vec2( 400, 0.5f ) );

		gl::translate( ivec2( -400, -29 ) );
		mBox.setText( text );
		mBox.draw();

		gl::translate( ivec2( 0, 34 ) );
		mBox.setText( ( boost::format( mText ) % mDistance ).str() );
		mBox.draw();
	}
	gl::popMatrices();
}