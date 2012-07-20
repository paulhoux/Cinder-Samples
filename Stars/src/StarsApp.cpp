#include "cinder/MayaCamUI.h"
#include "cinder/Utilities.h"
#include "cinder/Timer.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

#include "Background.h"
#include "Grid.h"
#include "Stars.h"
#include "UserInterface.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class StarsApp : public AppBasic {
public:
	void prepareSettings(Settings *settings);
	void setup();
	void update();
	void draw();

	void mouseDown( MouseEvent event );	
	void mouseDrag( MouseEvent event );
	void keyDown( KeyEvent event );
	void resize( ResizeEvent event );
protected:
	void	forceHideCursor();
	void	forceShowCursor();
protected:
	// camera
	CameraPersp		mCam;
	MayaCamUI		mMayaCam;
	Vec3f			mCameraEyePoint;
	
	// graphical elements
	Stars			mStars;
	Background		mBackground;
	Grid			mGrid;
	UserInterface	mUserInterface;

	// animation timer
	Timer			mTimer;

	// 
	bool			mEnableAnimation;
	bool			mEnableGrid;
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
	mCameraEyePoint = Vec3f(0, 0, 0);

	mCam.setFov(60.0f);
	mCam.setNearClip( 0.01f );
	mCam.setFarClip( 5000.0f );
	mCam.setEyePoint( mCameraEyePoint );
	mCam.setCenterOfInterestPoint( Vec3f(0, 0, 0) );
	
	mMayaCam.setCurrentCam( mCam );

	//
	mEnableAnimation = true;
	mEnableGrid = false;

	//
	forceHideCursor();

	//
	mTimer.start();
}

void StarsApp::update()
{	
	//
	static const float sqrt2pi = math<float>::sqrt(2.0f * (float) M_PI);

	// animate camera
	if(mEnableAnimation) {
		// calculate time 
		float t = (float) mTimer.getSeconds();

		// determine distance to the sun (in parsecs)
		float time = t * 0.005f;
		float t_frac = (time) - math<float>::floor(time);
		float n = sqrt2pi * t_frac;
		float f = cosf( n * n );
		float distance = 500.0f - 499.95f * f;

		// determine where to look
		float a = t * 0.029f;
		float b = t * 0.026f;
		float x = -cosf(a) * (0.5f + 0.499f * cosf(b));
		float y = 0.5f + 0.499f * sinf(b);
		float z = sinf(a) * (0.5f + 0.499f * cosf(b));

		mCameraEyePoint = distance * Vec3f(x, y, z).normalized();
	}

	mCam.setEyePoint( mCameraEyePoint );
	mCam.setCenterOfInterestPoint( Vec3f::zero() );
	mMayaCam.setCurrentCam( mCam );

	// update background and user interface
	mBackground.setCameraDistance( mCameraEyePoint.length() );
	mUserInterface.setCameraDistance( mCameraEyePoint.length() );
}

void StarsApp::draw()
{		
	gl::clear( Color::black() ); 
	
	gl::pushMatrices();
	gl::setMatrices( mMayaCam.getCamera() );
	{
		gl::enableDepthRead();
		gl::enableDepthWrite();

		// draw grid
		if(mEnableGrid) 
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
	mEnableAnimation = false;
	mEnableGrid = true;

	mMayaCam.mouseDown( event.getPos() );
	mCameraEyePoint = mMayaCam.getCamera().getEyePoint();
}

void StarsApp::mouseDrag( MouseEvent event )
{
	// allow user to control camera
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	mCameraEyePoint = mMayaCam.getCamera().getEyePoint();
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
		mEnableAnimation = true;
		mEnableGrid = false;
		break;
	case KeyEvent::KEY_g:
		// toggle grid
		mEnableGrid = !mEnableGrid;
		break;
	case KeyEvent::KEY_c:
		// toggle cursor
		if(mIsCursorVisible) 
			forceHideCursor();
		else 
			forceShowCursor();
		break;
	case KeyEvent::KEY_KP0:
		// restart animation
		mTimer.stop();
		mTimer.start();
		mEnableAnimation = true;
		mEnableGrid = false;
		break;
	}
}

void StarsApp::resize( ResizeEvent event )
{
	// update camera to the new aspect ratio
	mCam.setAspectRatio( event.getAspectRatio() );
	mMayaCam.setCurrentCam( mCam );
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