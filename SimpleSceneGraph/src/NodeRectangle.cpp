#include "NodeRectangle.h"

using namespace ci;
using namespace ci::app;
using namespace ph::nodes;

NodeRectangle::NodeRectangle(void)
{
	mIsDragged = false;
	mColor = Color::white();
}

NodeRectangle::~NodeRectangle(void)
{
}

void NodeRectangle::update(double elapsed)
{
}

void NodeRectangle::draw()
{
	Rectf bounds = getBounds();

	// draw background
	gl::color( ColorA(1,1,1, 0.25f) );
	gl::enableAlphaBlending();
	gl::drawSolidRect( bounds );
	gl::disableAlphaBlending();

	// draw frame
	gl::color( mColor );
	gl::drawStrokedRect( bounds );

	// draw lines to the origin of each child
	gl::color( Color(0,1,1) );
	NodeRectangleList nodes = getChildren<NodeRectangle>();
	NodeRectangleList::iterator itr;
	for(itr=nodes.begin(); itr!=nodes.end(); ++itr) 
		gl::drawLine( Vec2f::zero(), (*itr)->getPosition() );
}

bool NodeRectangle::mouseMove(MouseEvent event)
{
	// The event specifies the mouse coordinates in screen space,
	// and our node position is specified in parent space. So, transform coordinates
	// from one space to the other using the built-in methods.

	// check if mouse is inside node (screen space -> object space)
	Vec2f pt = screenToObject( event.getPos() );
	if( getBounds().contains(pt) ) 
		mColor = Color(0, 1, 0);
	else if( mIsDragged )
		mColor = Color(1, 1, 0);
	else
		mColor = Color(1, 1, 1);

	return false;
}

bool NodeRectangle::mouseDown(MouseEvent event)
{
	// The event specifies the mouse coordinates in screen space,
	// and our node position is specified in parent space. So, transform coordinates
	// from one space to the other using the built-in methods.
	
	// check if we clicked inside node (screen space -> object space)
	Vec2f pt = screenToObject( event.getPos() );
	if(! getBounds().contains(pt) ) return false;

	mColor = Color(1, 1, 0);

	// calculate click offset
	mOffset = screenToParent( event.getPos() ) - getPosition();
	mIsDragged = true;

	return true;
}

bool NodeRectangle::mouseDrag(MouseEvent event)
{
	if(!mIsDragged) return false;

	// The event specifies the mouse coordinates in screen space,
	// and our position is specified in parent space. So, transform coordinates
	// from one space to the other using the built-in methods.
	Vec2f pt = screenToParent( event.getPos() ) - mOffset;
	setPosition( pt );

	return true;
}

bool NodeRectangle::mouseUp(MouseEvent event)
{
	mIsDragged = false;

	mColor = Color(1, 1, 1);

	return false;
}
