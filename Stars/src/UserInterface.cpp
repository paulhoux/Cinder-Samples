#include "UserInterface.h"

#include "cinder/app/AppBasic.h"

#include <boost/format.hpp>

using namespace ci;
using namespace ci::app;
using namespace std;

UserInterface::UserInterface(void)
	: mDistance(0.0f)
{
}

UserInterface::~UserInterface(void)
{
}

void UserInterface::setup()
{
	try { mFont = Font( loadAsset("fonts/sdf.ttf"), 20.0f ); }
	catch( const std::exception &e ) { console() << "Could not load font: " << e.what() << std::endl; }

	mText = std::string("%.0f lightyears from the Sun");
}

void UserInterface::draw()
{
	if(!mFont) return;

	glPushAttrib( GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT );

	gl::enableAlphaBlending();
	gl::color( Color::white() );

	Vec2f position = Vec2f(0.5f, 0.95f) * Vec2f(getWindowSize());

	gl::drawLine( position + Vec2f(-400, -10), position + Vec2f(400, -10) );
	gl::drawStringCentered( (boost::format(mText) % mDistance).str(), position, Color::white(), mFont );

	glPopAttrib();
}