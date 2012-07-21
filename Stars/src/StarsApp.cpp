#include "cinder/MayaCamUI.h"
#include "cinder/Utilities.h"
#include "cinder/Timer.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

#include "Background.h"
#include "Cam.h"
#include "Grid.h"
#include "Stars.h"
#include "UserInterface.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class StarsApp : public AppBasic {
public:
	void	prepareSettings(Settings *settings);
	void	setup();
	void	update();
	void	draw();

	void	mouseDown( MouseEvent event );	
	void	mouseDrag( MouseEvent event );	
	void	mouseUp( MouseEvent event );
	void	keyDown( KeyEvent event );
	void	resize( ResizeEvent event );
protected:
	void	forceHideCursor();
	void	forceShowCursor();
protected:
	double			mTime;

	// camera
	Cam				mCamera;
	
	// graphical elements
	Stars			mStars;
	Background		mBackground;
	Grid			mGrid;
	UserInterface	mUserInterface;

	// animation timer
	Timer			mTimer;

	// 
	bool			mIsGridVisible;
	bool			mIsCursorVisible;
};

void StarsApp::prepareSettings(Settings *settings)
{
	settings->setFrameRate(100.0f);
	settings->setFullScreen(true);
	settings->setWindowSize(1280,720);
}

void StarsApp::setup()
{
	mTime = getElapsedSeconds();

	// create the spherical grid mesh
	mGrid.setup();

	// load the star database and create the VBO mesh
	mStars.load( loadAsset("hygxyz.csv") );
	//mStars.write( writeFile( getAssetPath("") / "hygxyz.dat" ) );	// TODO

	// load texture and shader
	mStars.setup();

	// create user interface
	mUserInterface.setup();

	// initialize background image
	mBackground.setup();

	// initialize camera
	mCamera.setup();

	//
	mIsGridVisible = false;

	//
	forceHideCursor();

	//
	mTimer.start();
}

void StarsApp::update()
{	
	double elapsed = getElapsedSeconds() - mTime;
	mTime += elapsed;

	// animate camera
	mCamera.update(elapsed);

	// update background and user interface
	mBackground.setCameraDistance( mCamera.getCamera().getEyePoint().length() );
	mUserInterface.setCameraDistance( mCamera.getCamera().getEyePoint().length() );
}

void StarsApp::draw()
{		
	gl::clear( Color::black() ); 
	
	gl::pushMatrices();
	gl::setMatrices( mCamera.getCamera() );
	{
		gl::enableDepthRead();
		gl::enableDepthWrite();

		// draw grid
		if(mIsGridVisible) 
			mGrid.draw();

		// draw background
		mBackground.draw();

		gl::disableDepthWrite();
		gl::disableDepthRead();

		// draw stars
		mStars.draw();
	}
	gl::popMatrices();

	// draw user interface
	mUserInterface.draw();

	// fade in at start of application
	gl::enableAlphaBlending();
	double t = math<double>::clamp( mTimer.getSeconds() / 3.0, 0.0, 1.0 );
	float a = ci::lerp<float>(1.0f, 0.0f, (float) t);

	if( a > 0.0f ) {
		gl::color( ColorA(0,0,0,a) );
		gl::drawSolidRect( getWindowBounds() );
	}
	gl::disableAlphaBlending();
}

void StarsApp::mouseDown( MouseEvent event )
{
	// allow user to control camera
	mCamera.mouseDown( event.getPos() );
}

void StarsApp::mouseDrag( MouseEvent event )
{
	// allow user to control camera
	mCamera.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void StarsApp::mouseUp( MouseEvent event )
{
	// allow user to control camera
	mCamera.mouseUp( event.getPos() );
}

void StarsApp::keyDown( KeyEvent event )
{
	switch( event.getCode() )
	{
	case KeyEvent::KEY_f:
		// toggle full screen
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_ESCAPE:
		// quit the application
		quit();
		break;
	case KeyEvent::KEY_SPACE:
		// enable animation
		mCamera.setup();
		break;
	case KeyEvent::KEY_g:
		// toggle grid
		mIsGridVisible = !mIsGridVisible;
		break;
	case KeyEvent::KEY_c:
		// toggle cursor
		if(mIsCursorVisible) 
			forceHideCursor();
		else 
			forceShowCursor();
		break;
	}
}

void StarsApp::resize( ResizeEvent event )
{
	mCamera.resize( event );
}

void StarsApp::forceHideCursor()
{
	// forces the cursor to hide
#ifdef WIN32
	while( ::ShowCursor(false) >= 0 );
#else
	hideCursor();
#endif
	mIsCursorVisible = false;
}

void StarsApp::forceShowCursor()
{
	// forces the cursor to show
#ifdef WIN32
	while( ::ShowCursor(true) < 0 );
#else
	showCursor();
#endif
	mIsCursorVisible = true;
}

CINDER_APP_BASIC( StarsApp, RendererGl )