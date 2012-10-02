#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "NodeRectangle.h"

using namespace ci;
using namespace ci::app;
using namespace std;

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

	void resize( ResizeEvent event );
protected:
	//! Keeps track of current game time, used to calculate elapsed time in seconds.
	double				mTime;

	//! The root node: an instance of our own NodeRectangle class
	NodeRectangleRef	mRoot;
};

void SimpleSceneGraphApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize(800, 600);
}

void SimpleSceneGraphApp::setup()
{
	// Initialize game time
	mTime = getElapsedSeconds();

	// create the root node: a large rectangle
	mRoot = NodeRectangleRef( new NodeRectangle() );
	// specify the position of the anchor point on our canvas
	mRoot->setPosition(400, 300);
	// we can easily set the anchor point to its center
	mRoot->setAnchorPercentage(0.5f, 0.5f);
	// set the size of the node
	mRoot->setSize(600, 450);

	// add smaller rectangles to the root node
	NodeRectangleRef child1( new NodeRectangle() );
	child1->setPosition(0, 0); // relative to parent node
	child1->setSize(240, 200);
	mRoot->addChild(child1);

	NodeRectangleRef child2( new NodeRectangle() );
	child2->setPosition(260, 0); // relative to parent node
	child2->setSize(240, 200);
	mRoot->addChild(child2);

	// add even smaller rectangles to the child rectangles
	NodeRectangleRef child( new NodeRectangle() );
	child->setPosition(5, 5); // relative to parent node
	child->setSize(100, 100);
	child1->addChild(child);

	child.reset( new NodeRectangle() );
	child->setPosition(5, 5); // relative to parent node
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
	// calculate elapsed time since last frame
	double elapsed = getElapsedSeconds() - mTime;
	mTime = getElapsedSeconds();

	// rotate the root node around its anchor point
	mRoot->setRotation( (float) (0.1 * mTime) );

	// update all nodes
	mRoot->treeUpdate( elapsed );
}

void SimpleSceneGraphApp::draw()
{
	// clear the window
	gl::clear();
	gl::setMatricesWindow( getWindowSize(), true );

	// draw all nodes, starting with the root node
	mRoot->treeDraw();
}

void SimpleSceneGraphApp::mouseMove( MouseEvent event )
{
	// pass the mouseMove event to all nodes. Important: this can easily bring your
	// frame rate down if you have a lot of nodes and none of them does anything with
	// this event. Only use it if you must! Needs optimization.
	//mRoot->treeMouseMove(event);
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

void SimpleSceneGraphApp::resize( ResizeEvent event )
{
	mRoot->treeResize(event);
}

CINDER_APP_BASIC( SimpleSceneGraphApp, RendererGl )