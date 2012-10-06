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
#include "cinder/app/AppBasic.h"

using namespace ci;
using namespace ci::app;
using namespace ph::nodes;

NodeRectangle::NodeRectangle(void)
	: mTouchMode(UNTOUCHED)
{
}

NodeRectangle::~NodeRectangle(void)
{
}

void NodeRectangle::setup()
{
	// apply random rotation on setup
	setRotation( toRadians( Rand::randFloat(-15.0f, 15.0f) ) );
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
	gl::color( (mTouchMode!=UNTOUCHED) ? Color(1,1,0) : mIsSelected ? Color(0,1,0) : Color(1,1,1) );
	gl::drawStrokedRect( bounds );

	// draw lines to the origin of each child
	gl::color( Color(0,1,1) );
	NodeRectangleList nodes = getChildren<NodeRectangle>();
	NodeRectangleList::iterator itr;
	for(itr=nodes.begin(); itr!=nodes.end(); ++itr) 
		gl::drawLine( getAnchor(), (*itr)->getPosition() );
}

bool NodeRectangle::mouseMove(MouseEvent event)
{
	// check if mouse is inside node (convert from screen space to object space)
	Vec2f pt = screenToObject( event.getPos() );

	// select node if mouse is inside
	setSelected( getBounds().contains(pt) );

	// keep doing this for all remaining nodes
	return false;
}

bool NodeRectangle::mouseDown(MouseEvent event)
{	
	// check if mouse is inside node (convert from screen space to object space)
	Vec2f pt = screenToObject( event.getPos() );
	if(! getBounds().contains(pt) ) return false;

	// The event specifies the mouse coordinates in screen space,
	// and our node position is specified in parent space. So, transform coordinates
	// from one space to the other using the built-in methods.
	mCurrentMouse = screenToParent( event.getPos() ) - getPosition();
	mInitialMouse = mCurrentMouse;

	mInitialPosition = getPosition();
	mInitialRotation = getRotation();
	mInitialScale = getScale();

	// drag if clicked with left, scale and rotate if clicked with right
	if( event.isLeftDown() )
		mTouchMode = DRAGGING;
	else if( event.isRightDown() )
		mTouchMode = RESIZING;

	return true;
}

bool NodeRectangle::mouseDrag(MouseEvent event)
{
	// The event specifies the mouse coordinates in screen space,
	// and our position is specified in parent space. So, transform coordinates
	// from one space to the other using the built-in methods.
	mCurrentMouse = screenToParent( event.getPos() ) - mInitialPosition;

	switch(mTouchMode) {
	case DRAGGING:		
		setPosition( mInitialPosition + (mCurrentMouse - mInitialMouse) );
		return true;
	case RESIZING:
		{
			// calculate a new scale
			float d0 = mInitialMouse.length();
			float d1 = mCurrentMouse.length();
			setScale( mInitialScale * (d1 / d0) );

			// calculate angle with x-axis of initial mouse
			float a0 = math<float>::atan2( mInitialMouse.y, mInitialMouse.x );
			// calculate angle with x-axis of current mouse
			float a1 = math<float>::atan2( mCurrentMouse.y, mCurrentMouse.x );
			// add the angle difference to the rotation angle
			setRotation( mInitialRotation * Quatf( Vec3f::zAxis(), (a1-a0) ) );
		}
		return true;
	}

	return false;
}

bool NodeRectangle::mouseUp(MouseEvent event)
{
	mTouchMode = UNTOUCHED;

	return false;
}
