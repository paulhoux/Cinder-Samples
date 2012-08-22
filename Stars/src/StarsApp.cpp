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

#include <irrKlang.h>

#pragma comment(lib, "irrKlang.lib")

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace irrklang;

class StarsApp : public AppBasic {
public:
	void	prepareSettings(Settings *settings);
	void	setup();
	void	shutdown();
	void	update();
	void	draw();

	void	mouseDown( MouseEvent event );	
	void	mouseDrag( MouseEvent event );	
	void	mouseUp( MouseEvent event );

	void	keyDown( KeyEvent event );
	void	resize( ResizeEvent event );
	void	fileDrop( FileDropEvent event );
protected:
	void	playMusic( const fs::path &path, bool loop=false );
	void	stopMusic();
	void	playSound( const fs::path &path, bool loop=false );

	shared_ptr<ISound>	createSound( const fs::path &path );

	void	forceHideCursor();
	void	forceShowCursor();
	void	constrainCursor( const Vec2i &pos );

	fs::path	getFirstFile( const fs::path &path );	
	fs::path	getNextFile( const fs::path &current );
	fs::path	getPrevFile( const fs::path &current );
protected:
	double			mTime;

	// cursor position
	Vec2i			mCursorPos;
	Vec2i			mCursorPrevious;

	// camera
	Cam				mCamera;
	MayaCamUI		mMayaCam;

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
	bool			mIsStereoscopic;

	//
	shared_ptr<ISoundEngine>	mSoundEngine;
	shared_ptr<ISound>			mSound;
	shared_ptr<ISound>			mMusic;

	//
	bool						mPlayMusic;
	fs::path					mMusicPath;
	std::vector<fs::path>		mMusicExtensions;
};

void StarsApp::prepareSettings(Settings *settings)
{
	settings->setFrameRate(200.0f);
	settings->setWindowSize(1280,720);

#if (defined WIN32 && defined NDEBUG)
	settings->setFullScreen(true);
#else
	// never start in full screen on MacOS or in debug mode
	settings->setFullScreen(false);
#endif
}

void StarsApp::setup()
{
	mTime = getElapsedSeconds();

	// create the spherical grid mesh
	mGrid.setup();

	// load the star database and create the VBO mesh
	if( fs::exists( getAssetPath("") / "hygxyz.cdb" ) )
		mStars.read( loadFile( getAssetPath("") / "hygxyz.cdb" ) );
	else
	{
		mStars.load( loadAsset("hygxyz.csv") );
		mStars.write( writeFile( getAssetPath("") / "hygxyz.cdb" ) );	
	}

	// load texture and shader
	mStars.setup();

	// create user interface
	mUserInterface.setup();

	// initialize background image
	mBackground.setup();

	// initialize camera
	mCamera.setup();

	CameraPersp cam( mCamera.getCamera() );
	cam.setNearClip( 0.1f );
	cam.setFarClip( 5000.0f );

	mMayaCam.setCurrentCam( cam );

	//
	mIsGridVisible = true;
	mIsStereoscopic = true;

	//
	mMusicExtensions.push_back( ".flac" );
	mMusicExtensions.push_back( ".ogg" );
	mMusicExtensions.push_back( ".wav" );
	mMusicExtensions.push_back( ".mp3" );

	mPlayMusic = true;

	// initialize the IrrKlang Sound Engine in a very safe way
	mSoundEngine = shared_ptr<ISoundEngine>( createIrrKlangDevice(), std::mem_fun(&ISoundEngine::drop) );

	if(mSoundEngine) {
		// play 3D Sun rumble
		mSound = createSound( getAssetPath("") / "sound/low_rumble_loop.mp3" );
		if(mSound) {
			mSound->setIsLooped(true);
			mSound->setMinDistance(2.5f);
			mSound->setMaxDistance(12.5f);
			mSound->setIsPaused(false);
		}

		// play background music (the first .mp3 file found in ./assets/music)
		fs::path path = getFirstFile( getAssetPath("") / "music" );
		playMusic(path);
	}

	//
	mTimer.start();

#if (defined WIN32 && defined NDEBUG)
	forceHideCursor();
#else
	forceShowCursor();
#endif
}

void StarsApp::shutdown()
{
	if(mSoundEngine) mSoundEngine->stopAllSounds();
}

void StarsApp::update()
{	
	double elapsed = getElapsedSeconds() - mTime;
	mTime += elapsed;

	double time = getElapsedSeconds() / 200.0;
	if(mSoundEngine && mMusic && mPlayMusic) time = mMusic->getPlayPosition() / (double) mMusic->getPlayLength();

	// animate camera
	mCamera.setDistanceTime(time);
	mCamera.update(elapsed);

	// update background and user interface
	mBackground.setCameraDistance( mCamera.getCamera().getEyePoint().length() );
	mUserInterface.setCameraDistance( mCamera.getCamera().getEyePoint().length() );

	//
	if(mSoundEngine) {
		// send camera position to sound engine (for 3D sounds)
		Vec3f pos = mCamera.getPosition();
		mSoundEngine->setListenerPosition( 
			vec3df(pos.x, pos.y, pos.z), 
			vec3df(-pos.x, -pos.y, -pos.z), 
			vec3df(0,0,0), 
			vec3df(0,1,0) );

		// if music has finished, play next track
		if( mPlayMusic && mMusic && mMusic->isFinished() ) {
			playMusic( getNextFile(mMusicPath) );
		}
	}
}

void StarsApp::draw()
{		
	float w = 0.5f * getWindowWidth();
	float h = 1.0f * getWindowHeight();

	gl::clear( Color::black() ); 

	glPushAttrib( GL_VIEWPORT_BIT );
	gl::pushMatrices();

	if(mIsStereoscopic) {
		// render left eye
		mCamera.enableStereoLeft();

		gl::setViewport( Area(0, 0, w, h) );
		gl::setMatrices( mCamera.getCamera() );
		{
			// draw background
			mBackground.draw();

			// draw grid
			if(mIsGridVisible) 
				mGrid.draw();

			// draw stars
			mStars.draw();
		}

		// render right eye
		mCamera.enableStereoRight();

		gl::setViewport( Area(w, 0, w * 2.0f, h) );
		gl::setMatrices( mCamera.getCamera() );
		{
			// draw background
			mBackground.draw();

			// draw grid
			if(mIsGridVisible) 
				mGrid.draw();

			// draw stars
			mStars.draw();
		}
	}
	else {
		gl::setMatrices( mMayaCam.getCamera() );
		{
			// draw background
			mBackground.draw();

			// draw grid
			if(mIsGridVisible) 
				mGrid.draw();

			// draw stars
			mStars.draw();

			// draw stereoscopic camera
			mCamera.enableStereoLeft();
			gl::color( Color(1,1,0) );
			gl::drawFrustum( mCamera.getCamera() );

			mCamera.enableStereoRight();
			gl::color( Color(0,1,1) );
			gl::drawFrustum( mCamera.getCamera() );
		}
	}

	gl::popMatrices();
	glPopAttrib();

	// draw user interface
	//mUserInterface.draw();

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
	mCursorPos = mCursorPrevious = event.getPos();
	if(mIsStereoscopic) 
		mCamera.mouseDown( mCursorPos );
	else 
		mMayaCam.mouseDown( mCursorPos );
}

void StarsApp::mouseDrag( MouseEvent event )
{
	mCursorPos += event.getPos() - mCursorPrevious;
	mCursorPrevious = event.getPos();

	constrainCursor( event.getPos() );

	// allow user to control camera
	if(mIsStereoscopic) 
		mCamera.mouseDrag( mCursorPos, event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	else 
		mMayaCam.mouseDrag( mCursorPos, event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void StarsApp::mouseUp( MouseEvent event )
{
	// allow user to control camera
	mCursorPos = mCursorPrevious = event.getPos();
	
	if(mIsStereoscopic) 
		mCamera.mouseUp( mCursorPos );
}

void StarsApp::keyDown( KeyEvent event )
{
#ifdef WIN32
	// allows the use of the media buttons on your Windows keyboard to control the music
	switch( event.getNativeKeyCode() )
	{
	case VK_MEDIA_NEXT_TRACK:
		// play next music file
		playMusic( getNextFile(mMusicPath) );
		return;
	case VK_MEDIA_PREV_TRACK:
		// play next music file
		playMusic( getPrevFile(mMusicPath) );
		return;
	case VK_MEDIA_STOP:
		stopMusic();
		return;
	case VK_MEDIA_PLAY_PAUSE:
		if( mSoundEngine && mMusic ) {
			if( mMusic->isFinished() )
				playMusic( mMusicPath );
			else
				mMusic->setIsPaused( !mMusic->getIsPaused() );
		}
		return;
	}
#endif

	switch( event.getCode() )
	{
	case KeyEvent::KEY_f:
		// toggle full screen
		setFullScreen( !isFullScreen() );
		if( !isFullScreen() )
			forceShowCursor();
		break;
	case KeyEvent::KEY_v:
		gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
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
	case KeyEvent::KEY_s:
		// toggle stereoscopic view
		mIsStereoscopic = !mIsStereoscopic;
		mStars.setAspectRatio( mIsStereoscopic ? 0.5f : 1.0f );
		break;
		/*// 
		case KeyEvent::KEY_KP7:
		mBackground.rotateX(-0.05f);
		break;
		case KeyEvent::KEY_KP9:
		mBackground.rotateX(+0.05f);
		break;
		case KeyEvent::KEY_KP4:
		mBackground.rotateY(-0.05f);
		break;
		case KeyEvent::KEY_KP6:
		mBackground.rotateY(+0.05f);
		break;
		case KeyEvent::KEY_KP1:
		mBackground.rotateZ(-0.05f);
		break;
		case KeyEvent::KEY_KP3:
		mBackground.rotateZ(+0.05f);
		break;
		//*/
	}
}

void StarsApp::resize( ResizeEvent event )
{
	mCamera.resize( event );

	CameraPersp cam( mMayaCam.getCamera() );
	cam.setAspectRatio( event.getAspectRatio() );
	mMayaCam.setCurrentCam( cam );
}

void StarsApp::fileDrop( FileDropEvent event )
{
	for(size_t i=0;i<event.getNumFiles();++i) {
		fs::path file = event.getFile(i);

		// skip if not a file
		if( !fs::is_regular_file( file ) ) continue;

		if( std::find( mMusicExtensions.begin(), mMusicExtensions.end(), file.extension() ) != mMusicExtensions.end() )
			playMusic(file);
	}
}

void StarsApp::playMusic( const fs::path &path, bool loop )
{
	if(mSoundEngine && !path.empty()) {
		// stop current music
		if(mMusic) 
			mMusic->stop();

		// play music in a very safe way
		mMusic = shared_ptr<ISound>( mSoundEngine->play2D( path.string().c_str(), loop, true ), std::mem_fun(&ISound::drop) );
		if(mMusic) mMusic->setIsPaused(false);

		mMusicPath = path;
		mPlayMusic = true;
	}
}

void StarsApp::stopMusic()
{
	if( mSoundEngine && mMusic && !mMusic->isFinished() ) {
		mPlayMusic = false;
		mMusic->stop();
	}
}

void StarsApp::playSound( const fs::path &path, bool loop )
{
	// play sound in a very safe way
	shared_ptr<ISound> sound( mSoundEngine->play2D( path.string().c_str(), loop, true ), std::mem_fun(&ISound::drop) );
	if(sound) sound->setIsPaused(false);
}

shared_ptr<ISound> StarsApp::createSound( const fs::path &path )
{
	shared_ptr<ISound>	sound;

	if(mSoundEngine && !path.empty()) {
		// create sound in a very safe way
		sound = shared_ptr<ISound>( mSoundEngine->play3D( path.string().c_str(), vec3df(0,0,0), false, true ), std::mem_fun(&ISound::drop) );
	}

	return sound;
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

void StarsApp::constrainCursor( const Vec2i &pos )
{
	// keeps the cursor well within the window bounds,
	// so that we can continuously drag the mouse without
	// ever hitting the sides of the screen

	if( pos.x < 50 || pos.x > getWindowWidth() - 50 || pos.y < 50 || pos.y > getWindowHeight() - 50 )
	{
#ifdef WIN32
		POINT pt;
		mCursorPrevious.x = pt.x = getWindowWidth() / 2;
		mCursorPrevious.y = pt.y = getWindowHeight() / 2;

		HWND hWnd = getRenderer()->getHwnd();
		::ClientToScreen(hWnd, &pt);
		::SetCursorPos(pt.x,pt.y);
#else
		// on MacOS, the results seem to be a little choppy,
		// which might have something to do with the OS
		// suppressing events for a short while after warping
		// the cursor. 
		// A call to "CGSetLocalEventsSuppressionInterval(0.0)"
		// might remedy that.
		// Uncomment the code below to try things out.
		/*//
        Vec2i pt;
        mCursorPrevious.x = pt.x = getWindowWidth() / 2;
		mCursorPrevious.y = pt.y = getWindowHeight() / 2;
		
        CGPoint target = CGPointMake((float) pt.x, (float) pt.y);
		// note: target should first be converted to screen position here
        CGWarpMouseCursorPosition(target);  
		//*/
#endif
	}
}	

fs::path	StarsApp::getFirstFile( const fs::path &path )
{
	fs::directory_iterator end_itr;
	for( fs::directory_iterator i( path ); i != end_itr; ++i )
	{
		// skip if not a file
		if( !fs::is_regular_file( i->status() ) ) continue;

		// skip if extension does not match
		if( std::find( mMusicExtensions.begin(), mMusicExtensions.end(), i->path().extension() ) == mMusicExtensions.end() )
			continue;

		// file matches, return it
		return i->path();
	}

	// failed, return empty path
	return fs::path();
}

fs::path	StarsApp::getNextFile( const fs::path &current )
{
	if( !current.empty() ) {
		bool useNext = false;

		fs::directory_iterator end_itr;
		for( fs::directory_iterator i( current.parent_path() ); i != end_itr; ++i )
		{
			// skip if not a file
			if( !fs::is_regular_file( i->status() ) ) continue;

			if(useNext) {
				// skip if extension does not match
				if( std::find( mMusicExtensions.begin(), mMusicExtensions.end(), i->path().extension() ) == mMusicExtensions.end() )
					continue;

				// file matches, return it
				return i->path();
			}
			else if( *i == current ) {
				useNext = true;
			}
		}
	}

	// failed, return empty path
	return fs::path();
}

fs::path	StarsApp::getPrevFile( const fs::path &current )
{
	if( !current.empty() ) {
		fs::path previous;

		fs::directory_iterator end_itr;
		for( fs::directory_iterator i( current.parent_path() ); i != end_itr; ++i )
		{
			// skip if not a file
			if( !fs::is_regular_file( i->status() ) ) continue;

			if( *i == current ) {
				// do we know what file came before this one?
				if( !previous.empty() )
					return previous;
				else
					break;
			}
			else {
				// skip if extension does not match
				if( std::find( mMusicExtensions.begin(), mMusicExtensions.end(), i->path().extension() ) == mMusicExtensions.end() )
					continue;

				// keep track of this file
				previous = *i;
			}
		}
	}

	// failed, return empty path
	return fs::path();
}

CINDER_APP_BASIC( StarsApp, RendererGl )