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
#include "cinder/app/AppBasic.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace ph { namespace nodes {

int				Node::nodeCount = 0;
unsigned int	Node::uuidCount = 1;
NodeMap			Node::uuidLookup;

Node::Node(void)
	: mUuid(uuidCount), 
	mIsVisible(true), mIsClickable(true), mIsSelected(false), mIsSetup(false),
	mIsTransformInvalidated(true)
{
	// default constructor for [Node]
	nodeCount++;
	uuidCount++;
	
	mTransform.setToIdentity();
	mWorldTransform.setToIdentity();
}

Node::~Node(void)
{
	// remove all children safely
	removeChildren();

	//
	nodeCount--;

	// remove from lookup table
	uuidLookup.erase(mUuid);
}

void Node::removeFromParent()
{
	NodeRef node = mParent.lock();
	if(node) node->removeChild( shared_from_this() );
}

void Node::addChild(NodeRef node)
{
	if(node && !hasChild(node))
	{
		// remove child from current parent
		NodeRef parent = node->getParent();
		if(parent) parent->removeChild(node);

		// add to children
		mChildren.push_back(node);

		// set parent
		node->setParent( shared_from_this() );

		// store nodes in lookup table if not done yet
		uuidLookup[mUuid] = NodeWeakRef( shared_from_this() );
		uuidLookup[node->mUuid] = NodeWeakRef( node );
	}
}

void Node::removeChild(NodeRef node)
{
	NodeList::iterator itr = std::find(mChildren.begin(), mChildren.end(), node);
	if(itr != mChildren.end()) 
	{
		// reset parent
		(*itr)->setParent( NodeRef() );

		// remove from children
		mChildren.erase(itr);
	}
}

void Node::removeChildren()
{
	NodeList::iterator itr;
	for(itr=mChildren.begin();itr!=mChildren.end();)
	{
		// reset parent
		(*itr)->setParent( NodeRef() );

		// remove from children
		itr = mChildren.erase(itr);
	}
}

bool Node::hasChild(NodeRef node) const 
{
	NodeList::const_iterator itr = std::find(mChildren.begin(), mChildren.end(), node);
	return(itr != mChildren.end());
}

void Node::putOnTop()
{
	NodeRef parent = getParent();
	if(parent) parent->putOnTop( shared_from_this() );
}

void Node::putOnTop(NodeRef node)
{
	// remove from list
	NodeList::iterator itr = std::find(mChildren.begin(), mChildren.end(), node);
	if(itr==mChildren.end()) return;

	mChildren.erase(itr);

	// add to end of list
	mChildren.push_back(node);
}

bool Node::isOnTop() const 
{
	NodeRef parent = getParent();
	if(parent) return parent->isOnTop( shared_from_this() );
	else return false;
}

bool Node::isOnTop(NodeConstRef node) const 
{
	if(mChildren.empty()) return false;
	if(mChildren.back() == node) return true;
	return false;
}

void Node::moveToBottom()
{
	NodeRef parent = getParent();
	if(parent) parent->moveToBottom( shared_from_this() );
}

void Node::moveToBottom(NodeRef node)
{
	// remove from list
	NodeList::iterator itr = std::find(mChildren.begin(), mChildren.end(), node);
	if(itr==mChildren.end()) return;

	mChildren.erase(itr);

	// add to start of list
	mChildren.push_front(node);
}

NodeRef Node::findChild(unsigned int uuid)
{
	if(mUuid == uuid) return shared_from_this();

	NodeRef node;
	NodeList::iterator itr;
	for(itr=mChildren.begin();itr!=mChildren.end();++itr) {
		node = (*itr)->findChild(uuid);
		if(node) return node;
	}

	return node;
}

void Node::treeSetup()
{
	setup();

	NodeList nodes(mChildren);
	NodeList::iterator itr;
	for(itr=nodes.begin();itr!=nodes.end();++itr)
		(*itr)->treeSetup();
}

void Node::treeShutdown()
{
	NodeList nodes(mChildren);
	NodeList::reverse_iterator itr;
	for(itr=nodes.rbegin();itr!=nodes.rend();++itr)
		(*itr)->treeShutdown();

	shutdown();
}

void Node::treeUpdate(double elapsed)
{
	// let derived class perform animation 
	update(elapsed);

	// update this node's children
	NodeList nodes(mChildren);
	NodeList::iterator itr;
	for(itr=nodes.begin();itr!=nodes.end();++itr)
		(*itr)->treeUpdate(elapsed);
}

void Node::treeDraw()
{
	if(!mIsVisible) 
		return;

	if(!mIsSetup) {
		setup();
		mIsSetup = true;
	}

	// update transform matrix by calling derived class's function
	if(mIsTransformInvalidated) transform();

	// let derived class know we are about to draw stuff
	predraw();

	// apply transform
	gl::pushModelView();

	// usual way to update modelview matrix
	gl::multModelView( mTransform );

	// draw this node by calling derived class
	draw();

	// draw this node's children
	NodeList::iterator itr;
	for(itr=mChildren.begin();itr!=mChildren.end();++itr)
		(*itr)->treeDraw();
	
	// restore transform
	gl::popModelView();

	// let derived class know we are done drawing
	postdraw();
}

// Note: the scene graph implementation is currently not fast enough to support mouseMove events
//  when there are more than a few nodes. 
bool Node::treeMouseMove( MouseEvent event )
{
	if(!mIsVisible) return false;

	// test children first, from top to bottom
	NodeList nodes(mChildren);
	NodeList::reverse_iterator itr;
	bool handled = false;
	for(itr=nodes.rbegin();itr!=nodes.rend() && !handled;++itr)
		handled = (*itr)->treeMouseMove(event);

	// if not handled, test this node
	if(!handled) handled = mouseMove(event);

	return handled;
}//*/

bool Node::treeMouseDown( MouseEvent event )
{
	if(!mIsVisible) return false;

	// test children first, from top to bottom
	NodeList nodes(mChildren);
	NodeList::reverse_iterator itr;
	bool handled = false;
	for(itr=nodes.rbegin();itr!=nodes.rend()&&!handled;++itr)
		handled = (*itr)->treeMouseDown(event);

	// if not handled, test this node
	if(!handled) handled = mouseDown(event);

	return handled;
}

bool Node::treeMouseDrag( MouseEvent event )
{
	if(!mIsVisible) return false;

	// test children first, from top to bottom
	NodeList nodes(mChildren);
	NodeList::reverse_iterator itr;
	bool handled = false;
	for(itr=nodes.rbegin();itr!=nodes.rend()&&!handled;++itr)
		handled = (*itr)->treeMouseDrag(event);

	// if not handled, test this node
	if(!handled) handled = mouseDrag(event);

	return handled;
}

bool Node::treeMouseUp( MouseEvent event )
{
	if(!mIsVisible) return false;

	// test children first, from top to bottom
	NodeList nodes(mChildren);
	NodeList::reverse_iterator itr;
	bool handled = false;
	for(itr=nodes.rbegin();itr!=nodes.rend()&&!handled;++itr)
		(*itr)->treeMouseUp(event); // don't care about 'handled' for now

	// if not handled, test this node
	if(!handled) handled = mouseUp(event);

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
	if(!mIsVisible) return false;

	// test children first, from top to bottom
	NodeList nodes(mChildren);
	NodeList::reverse_iterator itr;
	bool handled = false;
	for(itr=nodes.rbegin();itr!=nodes.rend()&&!handled;++itr)
		handled = (*itr)->treeKeyDown(event);

	// if not handled, test this node
	if(!handled) handled = keyDown(event);

	return handled;
}

bool Node::treeKeyUp( KeyEvent event )
{
	if(!mIsVisible) return false;

	// test children first, from top to bottom
	NodeList nodes(mChildren);
	NodeList::reverse_iterator itr;
	bool handled = false;
	for(itr=nodes.rbegin();itr!=nodes.rend()&&!handled;++itr)
		handled = (*itr)->treeKeyUp(event);

	// if not handled, test this node
	if(!handled) handled = keyUp(event);

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
	NodeList nodes(mChildren);
	NodeList::reverse_iterator itr;
	bool handled = false;
	for(itr=nodes.rbegin();itr!=nodes.rend()&&!handled;++itr)
		handled = (*itr)->treeResize();

	// if not handled, test this node
	if(!handled) handled = resize();

	return handled;
}

bool Node::resize()
{
	return false; 
}

Vec2f Node::project(float x, float y) const
{	
	// get viewport and projection matrix
	Area		viewport = gl::getViewport();
	Matrix44f	projection = gl::getProjection();

	// find the modelview-projection-matrix
	Matrix44f mvp = projection * mWorldTransform;

	//
	Vec4f in(x, y, 0.0f, 1.0f);

	Vec4f out = mvp * in;
	if(out.w != 0.0f) out.w = 1.0f / out.w;

	out.x *= out.w;
	out.y *= out.w;
	out.z *= out.w;

	// map to window coordinates
	Vec2f result;
	result.x = viewport.getX1() + viewport.getWidth() * (out.x + 1.0f) / 2.0f;
	result.y = viewport.getY1() + viewport.getHeight() * (1.0f - (out.y + 1.0f) / 2.0f);

	return result;
}

/* transforms clip-space coordinates to object-space coordinates,
	where z is within range [0.0 - 1.0] from near-plane to far-plane */
Vec3f Node::unproject(float x, float y, float z) const
{
	// get viewport and projection matrix
	Area viewport = gl::getViewport();
	Matrix44f projection = gl::getProjection();

	// find the inverse modelview-projection-matrix
	Matrix44f mvp = projection * mWorldTransform;
	mvp.invert(0.0f);

	// map x and y from window coordinates
	Vec4f in(x, float(viewport.getHeight()) - y - 1.0f, z, 1.0f);
	in.x = (in.x - viewport.getX1()) / float( viewport.getWidth() );
	in.y = (in.y - viewport.getY1()) / float( viewport.getHeight() );
	
	// map to range [-1..1]
	in.x = 2.0f * in.x - 1.0f; 
	in.y = 2.0f * in.y - 1.0f; 
	in.z = 2.0f * in.z - 1.0f; 

	//
	Vec4f out = mvp * in;
	if(out.w != 0.0f) out.w = 1.0f / out.w;
	
	Vec3f result;
	result.x = out.x * out.w;
	result.y = out.y * out.w;
	result.z = out.z * out.w;
	
	return result;
}

////////////////// Node2D //////////////////


Node2D::Node2D(void)
{
	mPosition	= Vec2f::zero();
	mRotation	= Quatf::identity();
	mScale		= Vec2f::one();
	mAnchor		= Vec2f::zero();

	mAnchorIsPercentage = false;
}

Node2D::~Node2D(void)
{
}

Vec2f Node2D::screenToParent( const Vec2f &pt ) const
{
	Vec2f p = pt;

	Node2DRef node = getParent<Node2D>();
	if(node) p = node->screenToObject(p);

	return p;
}

Vec2f Node2D::screenToObject( const Vec2f &pt ) const
{
	// near plane intersection 
	Vec3f p0 = unproject(pt.x, pt.y, 0.0f);
	// far plane intersection 
	Vec3f p1 = unproject(pt.x, pt.y, 1.0f);

	// find (x, y) coordinates 
	float t = (0.0f - p0.z) / (p1.z - p0.z);
	float x = (p0.x + t * (p1.x - p0.x));
	float y = (p0.y + t * (p0.y - p1.y));

	return Vec2f(p0.x, p0.y);
}

Vec2f Node2D::parentToScreen( const Vec2f &pt ) const
{
	Vec2f p = pt;

	Node2DRef node = getParent<Node2D>();
	if(node) p = node->objectToScreen(p);

	return p;
}

Vec2f Node2D::parentToObject( const Vec2f &pt ) const
{
	Vec3f p = mTransform.inverted(5.96e-8f).transformPointAffine( Vec3f(pt, 0.0f) );
	return Vec2f(p.x, p.y);
}

Vec2f Node2D::objectToParent( const Vec2f &pt ) const
{
	Vec3f p = mTransform.transformPointAffine( Vec3f(pt, 0.0f) );
	return Vec2f(p.x, p.y);
}

Vec2f Node2D::objectToScreen( const Vec2f &pt ) const
{
	return project(pt);
}

////////////////// Node3D //////////////////

Node3D::Node3D(void)
{
	mPosition	= Vec3f::zero();
	mRotation	= Quatf::identity();
	mScale		= Vec3f::one();
	mAnchor		= Vec3f::zero();
}

Node3D::~Node3D(void)
{
}

void Node3D::treeDrawWireframe()
{
	if(!mIsVisible) return;

	// apply transform
	gl::pushModelView();

	// usual way to update modelview matrix
	gl::multModelView( mTransform );

	// draw this node by calling derived class
	drawWireframe();

	// draw this node's children
	NodeList::iterator itr;
	for(itr=mChildren.begin();itr!=mChildren.end();++itr) {
		// only call other Node3D's
		Node3DRef node = boost::dynamic_pointer_cast<Node3D>(*itr);
		if(node) node->treeDrawWireframe();
	}
	
	// restore transform
	gl::popModelView();
}

} } // namespace ph::nodes