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

#include "cinder/MayaCamUI.h"
#include "cinder/Utilities.h"
#include "cinder/Timer.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"

#include "Background.h"
#include "Cam.h"
#include "Constellations.h"
#include "ConstellationLabels.h"
#include "Conversions.h"
#include "Grid.h"
#include "Labels.h"
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
	void	resize();
	void	fileDrop( FileDropEvent event );

	bool	isStereoscopic() const { return mIsStereoscopic; }
	bool	isCylindrical() const { return mIsCylindrical; }
protected:
	void	playMusic( const fs::path &path, bool loop=false );
	void	stopMusic();
	void	playSound( const fs::path &path, bool loop=false );

	shared_ptr<ISound>	createSound( const fs::path &path );

	void	forceHideCursor();
	void	forceShowCursor();
	void	constrainCursor( const Vec2i &pos );

	void	render();

	void	createShader();
	void	createFbo();

	fs::path	getFirstFile( const fs::path &path );	
	fs::path	getNextFile( const fs::path &current );
	fs::path	getPrevFile( const fs::path &current );
protected:
	double				mTime;

	// cursor position
	Vec2i				mCursorPos;
	Vec2i				mCursorPrevious;

	// camera
	Cam					mCamera;

	// graphical elements
	Stars				mStars;
	Labels				mLabels;
	Constellations		mConstellations;
	ConstellationLabels	mConstellationLabels;
	Background			mBackground;
	Grid				mGrid;
	UserInterface		mUserInterface;

	// animation timer
	Timer				mTimer;

	// toggles
	bool				mIsGridVisible;
	bool				mIsLabelsVisible;
	bool				mIsConstellationsVisible;
	bool				mIsCursorVisible;
	bool				mIsStereoscopic;
	bool				mIsCylindrical;

	// frame buffer and shader used for cylindrical projection
	gl::Fbo				mFbo;
	gl::GlslProg		mShader;

	// sound
	shared_ptr<ISoundEngine>	mSoundEngine;
	shared_ptr<ISound>			mSound;
	shared_ptr<ISound>			mMusic;

	// music player
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
	// create the spherical grid mesh
	mGrid.setup();

	// load the star database and create the VBO mesh
	if( fs::exists( getAssetPath("") / "stars.cdb" ) )
		mStars.read( loadFile( getAssetPath("") / "stars.cdb" ) );

	if( fs::exists( getAssetPath("") / "labels.cdb" ) )
		mLabels.read( loadFile( getAssetPath("") / "labels.cdb" ) );

	if( fs::exists( getAssetPath("") / "constellations.cdb" ) )
		mConstellations.read( loadFile( getAssetPath("") / "constellations.cdb" ) );

	if( fs::exists( getAssetPath("") / "constellationlabels.cdb" ) )
		mConstellationLabels.read( loadFile( getAssetPath("") / "constellationlabels.cdb" ) );

	// create user interface
	mUserInterface.setup();

	// initialize background image
	mBackground.setup();

	// initialize camera
	mCamera.setup();

	CameraPersp cam( mCamera.getCamera() );
	cam.setNearClip( 0.01f );
	cam.setFarClip( 5000.0f );

	//
	mIsGridVisible = true;
	mIsLabelsVisible = true;
	mIsConstellationsVisible = true;
	mIsStereoscopic = false;
	mIsCylindrical = false;

	// create stars
	mStars.setup();
	mStars.setAspectRatio( mIsStereoscopic ? 0.5f : 1.0f );

	// create labels
	mLabels.setup();
	mConstellationLabels.setup();

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
	createShader();

	//
	mTimer.start();

#if (defined WIN32 && defined NDEBUG)
	forceHideCursor();
#else
	forceShowCursor();
#endif

	mTime = getElapsedSeconds();
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

	// adjust content based on camera distance
	float distance = mCamera.getCamera().getEyePoint().length();
	mBackground.setCameraDistance( distance );
	mLabels.setCameraDistance( distance );
	mConstellations.setCameraDistance( distance );
	mConstellationLabels.setCameraDistance( distance );
	mUserInterface.setCameraDistance( distance );

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
	int w = getWindowWidth();
	int h = getWindowHeight();

	gl::clear( Color::black() ); 

	if(mIsStereoscopic) {
		glPushAttrib( GL_VIEWPORT_BIT );
		gl::pushMatrices();

		// render left eye
		mCamera.enableStereoLeft();

		gl::setViewport( Area(0, 0, w / 2, h) );
		gl::setMatrices( mCamera.getCamera() );
		render();
	
		// draw user interface
		mUserInterface.draw("Stereoscopic Projection");

		// render right eye
		mCamera.enableStereoRight();

		gl::setViewport( Area(w / 2, 0, w, h) );
		gl::setMatrices( mCamera.getCamera() );
		render();
	
		// draw user interface
		mUserInterface.draw("Stereoscopic Projection");

		gl::popMatrices();		
		glPopAttrib();
	}
	else if(mIsCylindrical) {
		// make sure we have a frame buffer to render to
		createFbo();

		// determine correct aspect ratio and vertical field of view for each of the 3 views
		w = mFbo.getWidth() / 3;
		h = mFbo.getHeight();

		const float aspect = float(w) / float(h);
		const float hFoV = 60.0f;
		const float vFoV = toDegrees( 2.0f * math<float>::atan( math<float>::tan( toRadians(hFoV) * 0.5f ) / aspect ) );

		// for values smaller than 1.0, this will cause each view to overlap the other ones
		const float overlap = 1.0f;	

		// bind the frame buffer object
		mFbo.bindFramebuffer();

		// store viewport, camera and matrices, so we can restore later
		glPushAttrib( GL_VIEWPORT_BIT );
		CameraStereo original = mCamera.getCamera();
		gl::pushMatrices();

		// setup camera	
		CameraStereo cam = mCamera.getCamera();
		cam.disableStereo();
		cam.setAspectRatio(aspect);
		cam.setFov( vFoV );

		Vec3f right, up;	
		cam.getBillboardVectors(&right, &up);
		Vec3f forward = up.cross(right);

		// render left side
		gl::setViewport( Area(0, 0, w, h) );

		cam.setViewDirection( Quatf(up, overlap * toRadians(hFoV)) * forward );
		cam.setWorldUp( up );
		gl::setMatrices( cam );
		render();
		
		// render front side
		gl::setViewport( Area(w, 0, w*2, h) );

		cam.setViewDirection( forward );
		cam.setWorldUp( up );
		gl::setMatrices( cam );
		render();	
	
		// draw user interface
		mUserInterface.draw( (boost::format("Cylindrical Projection (%d degrees)") % int( (1.0f + 2.0f * overlap) * hFoV ) ).str() );

		// render right side
		gl::setViewport( Area(w*2, 0, w*3, h) );

		cam.setViewDirection( Quatf(up, -overlap * toRadians(hFoV)) * forward );
		cam.setWorldUp( up );
		gl::setMatrices( cam );
		render();
		
		// unbind the frame buffer object
		mFbo.unbindFramebuffer();

		// restore states
		gl::popMatrices();		
		mCamera.setCurrentCam(original);
		glPopAttrib();

		// draw frame buffer and perform cylindrical projection using a fragment shader
		if(mShader) {
			float sides = 3;
			float radians = sides * toRadians( hFoV );
			float reciprocal = 0.5f / sides;

			mShader.bind();
			mShader.uniform("texture", 0);
			mShader.uniform("sides", sides);
			mShader.uniform("radians", radians );
			mShader.uniform("reciprocal", reciprocal );
		}

		Rectf centered = Rectf(mFbo.getBounds()).getCenteredFit( getWindowBounds(), false );
		gl::draw( mFbo.getTexture(), centered );

		if(mShader) mShader.unbind();
	}
	else {
		mCamera.disableStereo();

		gl::pushMatrices();
		gl::setMatrices( mCamera.getCamera() );
		render();
		gl::popMatrices();
	
		// draw user interface
		mUserInterface.draw("Perspective Projection");
	}

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

void StarsApp::render()
{
	// draw background
	mBackground.draw();

	// draw grid
	if(mIsGridVisible) 
		mGrid.draw();

	// draw stars
	mStars.draw();

	// draw constellations
	if(mIsConstellationsVisible) 
		mConstellations.draw();

	// draw labels (for now, labels don't behave well in cylindrical view)
	if(mIsLabelsVisible && !mIsCylindrical) {
		mLabels.draw();

		if(mIsConstellationsVisible) 
			mConstellationLabels.draw();
	}
}

void StarsApp::mouseDown( MouseEvent event )
{
	// allow user to control camera
	mCursorPos = mCursorPrevious = event.getPos();
	mCamera.mouseDown( mCursorPos );
}

void StarsApp::mouseDrag( MouseEvent event )
{
	mCursorPos += event.getPos() - mCursorPrevious;
	mCursorPrevious = event.getPos();

	constrainCursor( event.getPos() );

	// allow user to control camera
	mCamera.mouseDrag( mCursorPos, event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void StarsApp::mouseUp( MouseEvent event )
{
	// allow user to control camera
	mCursorPos = mCursorPrevious = event.getPos();
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
	case KeyEvent::KEY_l:
		// toggle labels
		mIsLabelsVisible = !mIsLabelsVisible;
		break;
	case KeyEvent::KEY_c:
		// toggle constellations
		mIsConstellationsVisible = !mIsConstellationsVisible;
		break;
	case KeyEvent::KEY_a:
		// toggle cursor arrow
		if(mIsCursorVisible) 
			forceHideCursor();
		else 
			forceShowCursor();
		break;
	case KeyEvent::KEY_s:
		// toggle stereoscopic view
		mIsStereoscopic = !mIsStereoscopic;
		mIsCylindrical = false;
		mStars.setAspectRatio( mIsStereoscopic ? 0.5f : 1.0f );
		break;
	case KeyEvent::KEY_d:
		// cylindrical panorama
		mIsCylindrical = !mIsCylindrical;
		mIsStereoscopic = false;
		break;
	case KeyEvent::KEY_RETURN:
		createShader();
		break;
	case KeyEvent::KEY_PLUS:
	case KeyEvent::KEY_EQUALS:
	case KeyEvent::KEY_KP_PLUS:
		mCamera.setFov( mCamera.getFov() + 0.1 );
		break;
	case KeyEvent::KEY_MINUS:
	case KeyEvent::KEY_UNDERSCORE:
	case KeyEvent::KEY_KP_MINUS:
		mCamera.setFov( mCamera.getFov() - 0.1 );
		break;
	}
}

void StarsApp::resize()
{
	mCamera.resize();
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

void StarsApp::createShader()
{
	fs::path vs = getAssetPath("") / "shaders/cylindrical_vert.glsl";
	fs::path fs = getAssetPath("") / "shaders/cylindrical_frag.glsl";

	//
	try {
		mShader = gl::GlslProg( loadFile(vs), loadFile(fs) );
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
		mShader = gl::GlslProg();
	}
}

void StarsApp::createFbo()
{
	// determine the size of the frame buffer
	int w = getWindowWidth() * 2;
	int h = getWindowHeight() * 2;

	if( mFbo && mFbo.getSize() == Vec2i(w, h) )
		return;

	// create the FBO
	gl::Fbo::Format fmt;
	fmt.setWrap( GL_REPEAT, GL_CLAMP_TO_BORDER );
	
	mFbo = gl::Fbo( w, h, fmt );

	// work-around for the flipped texture issue
	mFbo.getTexture().setFlipped();
	mFbo.getDepthTexture().setFlipped();
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

// allow easy access to the application from outside
static StarsApp* StarsAppPtr() { return static_cast<StarsApp*>( App::get() ); }

CINDER_APP_BASIC( StarsApp, RendererGl )