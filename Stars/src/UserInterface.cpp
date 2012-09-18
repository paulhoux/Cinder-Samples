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
	text::fonts().loadFont( loadAsset("fonts/SDF.sdff") ); 
	mBox.setFont( text::fonts().getFont("SDF") );
	mBox.setFontSize( 20.0f );
	mBox.setBoundary( text::Text::WORD );
	mBox.setAlignment( text::Text::CENTER );
	mBox.setSize( 800, 100 );

	mText = std::string("%.0f lightyears from the Sun");
}

void UserInterface::draw()
{
	Vec2f position = Vec2f(0.5f, 0.92f) * Vec2f(getWindowSize());

	gl::drawLine( position + Vec2f(-400, 0), position + Vec2f(400, 0) );

	glPushAttrib( GL_CURRENT_BIT );
	gl::pushModelView();

	gl::translate( position + Vec2f(-400, 0) );
	gl::color( Color::white() );

	mBox.draw();

	gl::popModelView();
	glPopAttrib();
}