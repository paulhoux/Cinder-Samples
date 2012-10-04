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

#pragma once

#include "cinder/Vector.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "nodes/Node.h"

//! For convenience, create a new type for the shared pointer and node list
typedef boost::shared_ptr<class NodeRectangle>	NodeRectangleRef;
typedef std::deque<NodeRectangleRef>			NodeRectangleList;

//! Our class extends a simple 2D node
class NodeRectangle
	: public ph::nodes::Node2D
{
public:
	typedef enum { UNTOUCHED, DRAGGING, RESIZING } TouchMode;

	NodeRectangle(void);
	virtual ~NodeRectangle(void);

	//! The nodes support Cinder's game loop methods:
	//! setup(), shutdown(), update(), draw().
	//! Note that the update() method takes a double,
	//! which is the elapsed time since last update in seconds.
	void setup();
	void update( double elapsed=0.0 );
	void draw();

	//! The nodes support Cinder's event methods:
	//! mouseMove(), mouseDown(), mouseDrag(), mouseUp(), keyDown(), keyUp() and resize()
	bool mouseMove( ci::app::MouseEvent event );
	bool mouseDown( ci::app::MouseEvent event );
	bool mouseDrag( ci::app::MouseEvent event );
	bool mouseUp( ci::app::MouseEvent event );

protected:
	TouchMode		mTouchMode;

	ci::Vec2f		mInitialPosition;
	ci::Quatf		mInitialRotation;
	ci::Vec2f		mInitialScale;
	
	ci::Vec2f		mInitialMouse;
	ci::Vec2f		mCurrentMouse;
};
