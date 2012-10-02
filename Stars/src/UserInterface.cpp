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

#include "cinder/app/AppBasic.h"
#include "text/FontStore.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace ph;

UserInterface::UserInterface(void)
	: mDistance(0.0f)
{
}

UserInterface::~UserInterface(void)
{
}

void UserInterface::setup()
{	
	text::fonts().loadFont( loadAsset("fonts/Ubuntu-BoldItalic.sdff") ); 
	mBox.setFont( text::fonts().getFont("Ubuntu-BoldItalic") );
	mBox.setFontSize( 24.0f );
	mBox.setBoundary( text::Text::WORD );
	mBox.setAlignment( text::Text::CENTER );
	mBox.setSize( 800, 100 );

	mText = std::string("%.0f lightyears from the Sun");
}

void UserInterface::draw()
{
	Vec2f position = Vec2f(0.5f, 0.92f) * Vec2f(getWindowSize());

	gl::drawLine( position + Vec2f(-400, 0.5f), position + Vec2f(400, 0.5f) );

	glPushAttrib( GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT );
	gl::enableAlphaBlending();
	gl::pushModelView();

	gl::translate( position + Vec2f(-400, 5) );
	gl::color( Color::white() );

	mBox.draw();

	gl::popModelView();
	glPopAttrib();
}