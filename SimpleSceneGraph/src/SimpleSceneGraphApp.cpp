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

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "NodeRectangle.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace ph::nodes;

class SimpleSceneGraphApp : public AppBasic {
public:
	void prepareSettings( Settings *settings );

	void setup();
	void shutdown();
	void update();
	void draw();

	void mouseMove( MouseEvent event );
	void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );
	void mouseUp( MouseEvent event );

	void keyDown( KeyEvent event );
	void keyUp( KeyEvent event );

	void resize();
protected:
	//! The root node
	Node2DRef			mRoot;
	//! The big rectangle that acts as a parent for the smaller ones
	NodeRectangleRef	mParent;
};

void SimpleSceneGraphApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize(800, 600);
}

void SimpleSceneGraphApp::setup()
{
	// create the root node
	mRoot = Node2DRef( new Node2D() );

	// create a large rectangle first
	mParent = NodeRectangleRef( new NodeRectangle() );
	// specify the position of the anchor point on our canvas
	mParent->setPosition(400, 300); // relative to parent node
	// we can easily set the anchor point to its center
	mParent->setAnchorPercentage(0.5f, 0.5f);
	// set the size of the node
	mParent->setSize(600, 450);
	// add it to the root of our scenegraph
	mRoot->addChild(mParent);

		// add smaller rectangles to the root node
	NodeRectangleRef child1( new NodeRectangle() );
	child1->setPosition(200, 225); // relative to parent node
	child1->setAnchorPercentage(0.5f, 0.5f);
	child1->setSize(240, 200);
	mParent->addChild(child1);

	NodeRectangleRef child2( new NodeRectangle() );
	child2->setPosition(400, 225); // relative to parent node
	child2->setAnchorPercentage(0.5f, 0.5f);
	child2->setSize(240, 200);
	mParent->addChild(child2);

	// add even smaller rectangles to the child rectangles
	NodeRectangleRef child( new NodeRectangle() );
	child->setPosition(60, 100); // relative to parent node
	child->setAnchorPercentage(0.5f, 0.5f);
	child->setSize(100, 100);
	child1->addChild(child);

	child.reset( new NodeRectangle() );
	child->setPosition(180, 100); // relative to parent node
	child->setAnchorPercentage(0.5f, 0.5f);
	child->setSize(100, 100);
	child1->addChild(child);

	child.reset( new NodeRectangle() );
	child->setPosition(60, 100); // relative to parent node
	child->setAnchorPercentage(0.5f, 0.5f);
	child->setSize(100, 100);
	child2->addChild(child);

	child.reset( new NodeRectangle() );
	child->setPosition(180, 100); // relative to parent node
	child->setAnchorPercentage(0.5f, 0.5f);
	child->setSize(100, 100);
	child2->addChild(child);

	// note that we only keep a reference to the root node. The children are
	// not deleted when this function goes out of scope, because they sit happily
	// in the list of children of their parent node. They are not deleted until
	// they are removed from their parent.
}

void SimpleSceneGraphApp::shutdown()
{
}

void SimpleSceneGraphApp::update()
{
	// update all nodes
	mRoot->treeUpdate();
}

void SimpleSceneGraphApp::draw()
{
	// clear the window
	gl::clear();
	gl::setMatricesWindow( getWindowSize(), true );

	// draw all nodes, starting with the root node
	mRoot->treeDraw();

	// example of coordinate conversion: 
	// convert big rectangle's origin to screen coordinates and draw a red circle there
	gl::color( Color(1, 0, 0) );
	gl::drawSolidCircle( mParent->objectToScreen( Vec2f::zero() ), 5.0f );
}

void SimpleSceneGraphApp::mouseMove( MouseEvent event )
{
	// pass the mouseMove event to all nodes. Important: this can easily bring your
	// frame rate down if you have a lot of nodes and none of them does anything with
	// this event. Only use it if you have just a few nodes, like in this sample,
	// or catch it as soon as possible by returning TRUE in your mouseMove() method.
	mRoot->treeMouseMove(event);
}

void SimpleSceneGraphApp::mouseDown( MouseEvent event )
{
	// pass the mouseDown event to all nodes. This is usually very quick because
	// it starts at the top nodes and they often catch the event.
	mRoot->treeMouseDown(event);
}

void SimpleSceneGraphApp::mouseDrag( MouseEvent event )
{
	// pass the mouseDrag event to all nodes. This is usually very quick.
	mRoot->treeMouseDrag(event);
}

void SimpleSceneGraphApp::mouseUp( MouseEvent event )
{
	// pass the mouseUp event to all nodes. This is usually very quick.
	mRoot->treeMouseUp(event);
}

void SimpleSceneGraphApp::keyDown( KeyEvent event )
{
	// let nodes handle keys first
	if(! mRoot->treeKeyDown(event) ) {
		switch( event.getCode() ) {
		case KeyEvent::KEY_ESCAPE:
			quit();
			break;
		case KeyEvent::KEY_RETURN:
			if(event.isAltDown()) {
				setFullScreen( !isFullScreen() );
			}
			break;
		}
	}
}

void SimpleSceneGraphApp::keyUp( KeyEvent event )
{
	mRoot->treeKeyUp(event);
}

void SimpleSceneGraphApp::resize()
{
	mRoot->treeResize();
}

CINDER_APP_BASIC( SimpleSceneGraphApp, RendererGl )