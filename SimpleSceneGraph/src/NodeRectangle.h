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
	NodeRectangle(void);
	virtual ~NodeRectangle(void);

	//! The nodes support Cinder's game loop methods:
	//! setup(), shutdown(), update(), draw(). Note that
	//! the update method takes a double "elapsed time"!
	void update( double elapsed );
	void draw();

	//! The nodes support Cinder's event methods:
	//! mouseMove(), mouseDown(), mouseDrag(), mouseUp(), keyDown(), keyUp() and resize()
	bool mouseMove( ci::app::MouseEvent event );
	bool mouseDown( ci::app::MouseEvent event );
	bool mouseDrag( ci::app::MouseEvent event );
	bool mouseUp( ci::app::MouseEvent event );
protected:
	bool		mIsDragged;
	ci::Vec2f	mOffset;
	ci::Color	mColor;
};
