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

#include "NodeRectangle.h"

#include "cinder/Rand.h"

using namespace ci;
using namespace ci::app;
using namespace ph::nodes;

NodeRectangle::NodeRectangle(void)
{
	mIsDragged = false;
	mColor = Color::white();

	// apply random rotation 
	setRotation( toRadians( Rand::randFloat(-15.0f, 15.0f) ) );
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
