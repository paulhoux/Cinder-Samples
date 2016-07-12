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

#include "nodes/Node.h"
#include "cinder/app/App.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace ph {
namespace nodes {

int          Node::nodeCount = 0;
unsigned int Node::uuidCount = 1;
NodeMap      Node::uuidLookup;

Node::Node( void )
    : mUuid( uuidCount )
    , mIsVisible( true )
    , mIsClickable( true )
    , mIsSelected( false )
    , mIsSetup( false )
    , mIsTransformInvalidated( true )
{
	// default constructor for [Node]
	nodeCount++;
	uuidCount++;
}

Node::~Node( void )
{
	// remove all children safely
	removeChildren();

	//
	nodeCount--;

	// remove from lookup table
	uuidLookup.erase( mUuid );
}

void Node::removeFromParent()
{
	NodeRef node = mParent.lock();
	if( node )
		node->removeChild( shared_from_this() );
}

void Node::addChild( NodeRef node )
{
	if( node && !hasChild( node ) ) {
		// remove child from current parent
		NodeRef parent = node->getParent();
		if( parent )
			parent->removeChild( node );

		// add to children
		mChildren.push_back( node );

		// set parent
		node->setParent( shared_from_this() );

		// store nodes in lookup table if not done yet
		uuidLookup[mUuid] = NodeWeakRef( shared_from_this() );
		uuidLookup[node->mUuid] = NodeWeakRef( node );
	}
}

void Node::removeChild( NodeRef node )
{
	NodeList::iterator itr = std::find( mChildren.begin(), mChildren.end(), node );
	if( itr != mChildren.end() ) {
		// reset parent
		( *itr )->setParent( NodeRef() );

		// remove from children
		mChildren.erase( itr );
	}
}

void Node::removeChildren()
{
	NodeList::iterator itr;
	for( itr = mChildren.begin(); itr != mChildren.end(); ) {
		// reset parent
		( *itr )->setParent( NodeRef() );

		// remove from children
		itr = mChildren.erase( itr );
	}
}

bool Node::hasChild( NodeRef node ) const
{
	NodeList::const_iterator itr = std::find( mChildren.begin(), mChildren.end(), node );
	return ( itr != mChildren.end() );
}

void Node::putOnTop()
{
	NodeRef parent = getParent();
	if( parent )
		parent->putOnTop( shared_from_this() );
}

void Node::putOnTop( NodeRef node )
{
	// remove from list
	NodeList::iterator itr = std::find( mChildren.begin(), mChildren.end(), node );
	if( itr == mChildren.end() )
		return;

	mChildren.erase( itr );

	// add to end of list
	mChildren.push_back( node );
}

bool Node::isOnTop() const
{
	NodeRef parent = getParent();
	if( parent )
		return parent->isOnTop( shared_from_this() );
	else
		return false;
}

bool Node::isOnTop( NodeConstRef node ) const
{
	if( mChildren.empty() )
		return false;
	if( mChildren.back() == node )
		return true;
	return false;
}

void Node::moveToBottom()
{
	NodeRef parent = getParent();
	if( parent )
		parent->moveToBottom( shared_from_this() );
}

//! sets the transformation matrix of this node

void Node::setTransform( const ci::mat4 &transform ) const
{
	mTransform = transform;

	auto parent = getParent<Node2D>();
	if( parent )
		mWorldTransform = parent->getWorldTransform() * mTransform;
	else
		mWorldTransform = mTransform;

	mIsTransformInvalidated = false;

	for( auto &child : mChildren )
		child->invalidateTransform();
}

void Node::moveToBottom( NodeRef node )
{
	// remove from list
	NodeList::iterator itr = std::find( mChildren.begin(), mChildren.end(), node );
	if( itr == mChildren.end() )
		return;

	mChildren.erase( itr );

	// add to start of list
	mChildren.insert( mChildren.begin(), node );
}

NodeRef Node::findChild( unsigned int uuid )
{
	if( mUuid == uuid )
		return shared_from_this();

	NodeRef            node;
	NodeList::iterator itr;
	for( itr = mChildren.begin(); itr != mChildren.end(); ++itr ) {
		node = ( *itr )->findChild( uuid );
		if( node )
			return node;
	}

	return node;
}

void Node::treeSetup()
{
	setup();

	NodeList           nodes( mChildren );
	NodeList::iterator itr;
	for( itr = nodes.begin(); itr != nodes.end(); ++itr )
		( *itr )->treeSetup();
}

void Node::treeShutdown()
{
	NodeList                   nodes( mChildren );
	NodeList::reverse_iterator itr;
	for( itr = nodes.rbegin(); itr != nodes.rend(); ++itr )
		( *itr )->treeShutdown();

	shutdown();
}

void Node::treeUpdate( double elapsed )
{
	// let derived class perform animation
	update( elapsed );

	// update this node's children
	NodeList           nodes( mChildren );
	NodeList::iterator itr;
	for( itr = nodes.begin(); itr != nodes.end(); ++itr )
		( *itr )->treeUpdate( elapsed );
}

void Node::treeDraw()
{
	if( !mIsVisible )
		return;

	if( !mIsSetup ) {
		setup();
		mIsSetup = true;
	}

	// update transform matrix by calling derived class's function
	if( mIsTransformInvalidated )
		transform();

	// let derived class know we are about to draw stuff
	predraw();

	// apply transform
	gl::pushModelView();

	// usual way to update model matrix
	gl::setModelMatrix( getWorldTransform() );

	// draw this node by calling derived class
	draw();

	// draw this node's children
	NodeList::iterator itr;
	for( itr = mChildren.begin(); itr != mChildren.end(); ++itr )
		( *itr )->treeDraw();

	// restore transform
	gl::popModelView();

	// let derived class know we are done drawing
	postdraw();
}

// Note: the scene graph implementation is currently not fast enough to support mouseMove events
//  when there are more than a few nodes.
bool Node::treeMouseMove( MouseEvent event )
{
	if( !mIsVisible )
		return false;

	// test children first, from top to bottom
	NodeList                   nodes( mChildren );
	NodeList::reverse_iterator itr;
	bool                       handled = false;
	for( itr = nodes.rbegin(); itr != nodes.rend() && !handled; ++itr )
		handled = ( *itr )->treeMouseMove( event );

	// if not handled, test this node
	if( !handled )
		handled = mouseMove( event );

	return handled;
} //*/

bool Node::treeMouseDown( MouseEvent event )
{
	if( !mIsVisible )
		return false;

	// test children first, from top to bottom
	NodeList                   nodes( mChildren );
	NodeList::reverse_iterator itr;
	bool                       handled = false;
	for( itr = nodes.rbegin(); itr != nodes.rend() && !handled; ++itr )
		handled = ( *itr )->treeMouseDown( event );

	// if not handled, test this node
	if( !handled )
		handled = mouseDown( event );

	return handled;
}

bool Node::treeMouseDrag( MouseEvent event )
{
	if( !mIsVisible )
		return false;

	// test children first, from top to bottom
	NodeList                   nodes( mChildren );
	NodeList::reverse_iterator itr;
	bool                       handled = false;
	for( itr = nodes.rbegin(); itr != nodes.rend() && !handled; ++itr )
		handled = ( *itr )->treeMouseDrag( event );

	// if not handled, test this node
	if( !handled )
		handled = mouseDrag( event );

	return handled;
}

bool Node::treeMouseUp( MouseEvent event )
{
	if( !mIsVisible )
		return false;

	// test children first, from top to bottom
	NodeList                   nodes( mChildren );
	NodeList::reverse_iterator itr;
	bool                       handled = false;
	for( itr = nodes.rbegin(); itr != nodes.rend() && !handled; ++itr )
		( *itr )->treeMouseUp( event ); // don't care about 'handled' for now

	// if not handled, test this node
	if( !handled )
		handled = mouseUp( event );

	return handled;
}

bool Node::mouseMove( MouseEvent event )
{
	return false;
}

bool Node::mouseDown( MouseEvent event )
{
	return false;
}

bool Node::mouseDrag( MouseEvent event )
{
	return false;
}

bool Node::mouseUp( MouseEvent event )
{
	return false;
}

bool Node::mouseUpOutside( MouseEvent event )
{
	return false;
}

bool Node::treeKeyDown( KeyEvent event )
{
	if( !mIsVisible )
		return false;

	// test children first, from top to bottom
	NodeList                   nodes( mChildren );
	NodeList::reverse_iterator itr;
	bool                       handled = false;
	for( itr = nodes.rbegin(); itr != nodes.rend() && !handled; ++itr )
		handled = ( *itr )->treeKeyDown( event );

	// if not handled, test this node
	if( !handled )
		handled = keyDown( event );

	return handled;
}

bool Node::treeKeyUp( KeyEvent event )
{
	if( !mIsVisible )
		return false;

	// test children first, from top to bottom
	NodeList                   nodes( mChildren );
	NodeList::reverse_iterator itr;
	bool                       handled = false;
	for( itr = nodes.rbegin(); itr != nodes.rend() && !handled; ++itr )
		handled = ( *itr )->treeKeyUp( event );

	// if not handled, test this node
	if( !handled )
		handled = keyUp( event );

	return handled;
}

bool Node::keyDown( KeyEvent event )
{
	return false;
}

bool Node::keyUp( KeyEvent event )
{
	return false;
}

bool Node::treeResize()
{
	// test children first, from top to bottom
	NodeList                   nodes( mChildren );
	NodeList::reverse_iterator itr;
	bool                       handled = false;
	for( itr = nodes.rbegin(); itr != nodes.rend() && !handled; ++itr )
		handled = ( *itr )->treeResize();

	// if not handled, test this node
	if( !handled )
		handled = resize();

	return handled;
}

bool Node::resize()
{
	return false;
}

////////////////// Node2D //////////////////


Node2D::Node2D( void )
    : mPosition( 0 )
    , mScale( 1 )
    , mAnchor( 0 )
    , mAnchorIsPercentage( false )
{
}

Node2D::~Node2D( void )
{
}

vec2 Node2D::screenToParent( const vec2 &pt ) const
{
	vec2 p = pt;

	Node2DRef node = getParent<Node2D>();
	if( node )
		p = node->screenToObject( p );

	return p;
}

vec2 Node2D::screenToObject( const vec2 &pt, float z ) const
{
	// Build the viewport (x, y, width, height).
	vec2 offset = gl::getViewport().first;
	vec2 size = gl::getViewport().second;
	vec4 viewport = vec4( offset.x, offset.y, size.x, size.y );

	// Calculate the view-projection matrix.
	mat4 model = getWorldTransform();
	mat4 viewProjection = gl::getProjectionMatrix() * gl::getViewMatrix();

	// Calculate the intersection of the mouse ray with the near (z=0) and far (z=1) planes.
	vec3 near = glm::unProject( vec3( pt.x, size.y - pt.y - 1, 0 ), model, viewProjection, viewport );
	vec3 far = glm::unProject( vec3( pt.x, size.y - pt.y - 1, 1 ), model, viewProjection, viewport );

	// Calculate world position.
	return vec2( ci::lerp( near, far, ( z - near.z ) / ( far.z - near.z ) ) );
}

vec2 Node2D::parentToScreen( const vec2 &pt ) const
{
	vec2 p = pt;

	Node2DRef node = getParent<Node2D>();
	if( node )
		p = node->objectToScreen( p );

	return p;
}

vec2 Node2D::parentToObject( const vec2 &pt ) const
{
	mat4 invTransform = glm::inverse( getTransform() );
	vec4 p = invTransform * vec4( pt, 0, 1 );

	return vec2( p.x, p.y );
}

vec2 Node2D::objectToParent( const vec2 &pt ) const
{
	vec4 p = getTransform() * vec4( pt, 0, 1 );
	return vec2( p.x, p.y );
}

vec2 Node2D::objectToScreen( const vec2 &pt ) const
{
	// Build the viewport (x, y, width, height).
	vec2 offset = gl::getViewport().first;
	vec2 size = gl::getViewport().second;
	vec4 viewport = vec4( offset.x, offset.y, size.x, size.y );

	// Calculate the view-projection matrix.
	mat4 model = getWorldTransform();
	mat4 viewProjection = gl::getProjectionMatrix() * gl::getViewMatrix();

	vec2 p = vec2( glm::project( vec3( pt, 0 ), model, viewProjection, viewport ) );
	p.y = size.y - 1 - p.y;

	return p;
}

////////////////// Node3D //////////////////

Node3D::Node3D( void )
    : mScale( 1 )
{
}

Node3D::~Node3D( void )
{
}

void Node3D::treeDrawWireframe()
{
	if( !mIsVisible )
		return;

	// apply transform
	gl::pushModelView();

	// usual way to update model matrix
	gl::setModelMatrix( getWorldTransform() );

	// draw this node by calling derived class
	drawWireframe();

	// draw this node's children
	NodeList::iterator itr;
	for( itr = mChildren.begin(); itr != mChildren.end(); ++itr ) {
		// only call other Node3D's
		Node3DRef node = std::dynamic_pointer_cast<Node3D>( *itr );
		if( node )
			node->treeDrawWireframe();
	}

	// restore transform
	gl::popModelView();
}
}
} // namespace ph::nodes