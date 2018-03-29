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

#include "cinder/Filesystem.h"
#include "cinder/Timer.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/gl.h"

#include "Background.h"
#include "Cam.h"
#include "ConstellationArt.h"
#include "ConstellationLabels.h"
#include "Constellations.h"
#include "Conversions.h"
#include "Grid.h"
#include "Labels.h"
#include "Stars.h"
#include "UserInterface.h"

#include <irrKlang.h>
#pragma comment( lib, "irrKlang.lib" )

#if defined( CINDER_MSW )
#include <windows.h>
#include <winuser.h>

// Make our application NVIDIA Optimus aware.
extern "C" {
_declspec( dllexport ) DWORD NvOptimusEnablement = 0x0000001;
}
#endif

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace irrklang;

class StarsApp : public App {
  public:
	static void prepare( Settings *settings );

	void setup() override;
	void cleanup() override;

	void update() override;
	void draw() override;

	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;

	void keyDown( KeyEvent event ) override;
	void fileDrop( FileDropEvent event ) override;

	void resize() override;

	bool isStereoscopic() const { return mIsStereoscopic; }
	bool isCylindrical() const { return mIsCylindrical; }

  protected:
	void playMusic( const fs::path &path, bool loop = false );
	void stopMusic();
	void playSound( const fs::path &path, bool loop = false );

	shared_ptr<ISound> createSound( const fs::path &path );

	void forceHideCursor();
	void forceShowCursor();
	void constrainCursor( const ivec2 &pos );

	void render();

	void createShader();
	void createFbo();

	fs::path getFirstFile( const fs::path &path );
	fs::path getNextFile( const fs::path &current );
	fs::path getPrevFile( const fs::path &current );

  protected:
	double mTime = 0.0;

	// cursor position
	ivec2 mCursorPos;
	ivec2 mCursorPrevious;

	// camera
	Cam mCamera;

	// graphical elements
	Stars               mStars;
	Labels              mLabels;
	Constellations      mConstellations;
	ConstellationArt    mConstellationArt;
	ConstellationLabels mConstellationLabels;
	Background          mBackground;
	Grid                mGrid;
	UserInterface       mUserInterface;

	gl::BatchRef mSun;

	// animation timer
	Timer mTimer;

	// toggles
	bool mIsGridVisible = false;
	bool mIsLabelsVisible = false;
	bool mIsConstellationsVisible = false;
	bool mIsConstellationArtVisible = false;
	bool mIsCursorVisible = false;
	bool mIsStereoscopic = false;
	bool mIsCylindrical = false;
	bool mIsDemo = true;
	bool mDrawUserInterface = false;

	// frame buffer and shader used for cylindrical projection
	gl::FboRef      mFbo;
	gl::GlslProgRef mShader;
	unsigned        mSectionCount;
	float           mSectionFovDegrees;
	float           mSectionOverlap;

	// sound
	shared_ptr<ISoundEngine> mSoundEngine;
	shared_ptr<ISound>       mSound;
	shared_ptr<ISound>       mMusic;

	// music player
	bool                  mPlayMusic;
	fs::path              mMusicPath;
	std::vector<fs::path> mMusicExtensions;
};

void StarsApp::prepare( Settings *settings )
{
	auto displays = Display::getDisplays();

	settings->disableFrameRate();
	settings->setBorderless( true );
	settings->setWindowPos( 0, 0 );
	settings->setWindowSize( 1352, 2080 );

#if !_DEBUG
	settings->setFullScreen( true );
#endif
}

void StarsApp::setup()
{
	// cylindrical projection settings
	mSectionCount = 3;
	mSectionFovDegrees = 72.0f;
	// for values smaller than 1.0, this will cause each view to overlap the other ones
	//  (angle of overlap: (1 - mSectionOverlap) * mSectionFovDegrees)
	mSectionOverlap = 1.0f;

	// create the spherical grid mesh
	mGrid.setup();

	// create stars
	mStars.setup();
	mStars.setAspectRatio( mIsStereoscopic ? 0.5f : 1.0f );

	// load the star database and create the VBO mesh
	if( fs::exists( getAssetPath( "" ) / "stars.cdb" ) )
		mStars.read( loadFile( getAssetPath( "" ) / "stars.cdb" ) );

	if( fs::exists( getAssetPath( "" ) / "labels.cdb" ) )
		mLabels.read( loadFile( getAssetPath( "" ) / "labels.cdb" ) );
	else {
		mLabels.load( loadAsset( "hygxyz.csv" ) );
		mLabels.write( writeFile( getAssetPath( "" ) / "labels.cdb" ) );
	}

	if( fs::exists( getAssetPath( "" ) / "constellations.cdb" ) )
		mConstellations.read( loadFile( getAssetPath( "" ) / "constellations.cdb" ) );

	if( fs::exists( getAssetPath( "" ) / "constellationlabels.cdb" ) )
		mConstellationLabels.read( loadFile( getAssetPath( "" ) / "constellationlabels.cdb" ) );

	// create user interface
	mUserInterface.setup();

	// initialize background image
	mBackground.setup();

	// initialize camera
	mCamera.setup();

	// create labels
	mLabels.setup();
	mConstellationArt.setup();
	mConstellationLabels.setup();

	//
	mMusicExtensions.push_back( ".flac" );
	mMusicExtensions.push_back( ".ogg" );
	mMusicExtensions.push_back( ".wav" );
	mMusicExtensions.push_back( ".mp3" );

	mPlayMusic = false;

	// initialize the IrrKlang Sound Engine in a very safe way
	mSoundEngine = shared_ptr<ISoundEngine>( createIrrKlangDevice(), std::mem_fn( &ISoundEngine::drop ) );

	if( mSoundEngine ) {
		// play 3D Sun rumble
		mSound = createSound( getAssetPath( "" ) / "sound/low_rumble_loop.mp3" );
		if( mSound ) {
			mSound->setIsLooped( true );
			mSound->setMinDistance( 2.5f );
			mSound->setMaxDistance( 12.5f );
			mSound->setIsPaused( false );
		}

		// play background music (the first .mp3 file found in ./assets/music)
		const fs::path path = getFirstFile( getAssetPath( "" ) / "music" );
		playMusic( path );
	}

	//
	createShader();

	//
	// auto mesh = gl::VboMesh::create( geom::Sphere().radius( 1 ).subdivisions( 60 ) );
	// auto glsl = gl::GlslProg::create( loadAsset( "shaders/sun.vert" ), loadAsset( "shaders/sun.frag" ) );
	// mSun = gl::Batch::create( mesh, glsl );

	//
	mTimer.start();

#if( defined WIN32 && defined NDEBUG )
	forceHideCursor();
#else
	forceShowCursor();
#endif

	mTime = getElapsedSeconds();
}

void StarsApp::cleanup()
{
	if( mSoundEngine )
		mSoundEngine->stopAllSounds();
}

void StarsApp::update()
{
	const double elapsed = getElapsedSeconds() - mTime;
	mTime += elapsed;

	double time = getElapsedSeconds() / 200.0;
	if( mSoundEngine && mMusic && mPlayMusic )
		time = mMusic->getPlayPosition() / double( mMusic->getPlayLength() );

	// toggle constellations in demo mode
	if( mIsDemo && time > 0.5 ) {
		mIsDemo = false;
		mIsConstellationsVisible = true;
		mIsConstellationArtVisible = true;
	}

	// animate camera
	mCamera.setDistanceTime( time );
	mCamera.update( elapsed );

	// adjust content based on camera distance
	const float distance = length( mCamera.getCamera().getEyePoint() );
	mBackground.setCameraDistance( distance );
	// mLabels.setCameraDistance( distance );
	mConstellations.setCameraDistance( distance );
	mConstellationArt.setCameraDistance( distance );
	// mConstellationLabels.setCameraDistance( distance );
	mUserInterface.setCameraDistance( distance );

	//
	if( mSoundEngine ) {
		// send camera position to sound engine (for 3D sounds)
		vec3 pos = mCamera.getPosition();
		mSoundEngine->setListenerPosition( vec3df( pos.x, pos.y, pos.z ), vec3df( -pos.x, -pos.y, -pos.z ), vec3df( 0, 0, 0 ), vec3df( 0, 1, 0 ) );

		// if music has finished, play next track
		if( mPlayMusic && mMusic && mMusic->isFinished() ) {
			playMusic( getNextFile( mMusicPath ) );
		}
	}

	//
	if( mSun )
		mSun->getGlslProg()->uniform( "uTime", float( getElapsedSeconds() ) );
}

void StarsApp::draw()
{
	int w = getWindowWidth();
	int h = getWindowHeight();

	gl::clear( Color::black() );
#if 1
	if( mIsStereoscopic ) {
		gl::ScopedViewport viewport( 0, 0, w / 2, h );
		gl::pushMatrices();

		// render left eye
		mCamera.enableStereoLeft();

		gl::setMatrices( mCamera.getCamera() );
		render();

		// draw user interface
		if( mDrawUserInterface )
			mUserInterface.draw( "Stereoscopic Projection" );

		// render right eye
		mCamera.enableStereoRight();

		gl::viewport( w / 2, 0, w / 2, h );
		gl::setMatrices( mCamera.getCamera() );
		render();

		// draw user interface
		if( mDrawUserInterface )
			mUserInterface.draw( "Stereoscopic Projection" );

		gl::popMatrices();
	}
	else if( mIsCylindrical ) {
		// make sure we have a frame buffer to render to
		createFbo();

		// determine correct aspect ratio and vertical field of view for each of the 3 views
		w = mFbo->getWidth() / mSectionCount;
		h = mFbo->getHeight();

		const float aspect = float( w ) / float( h );
		// const float hFoV = mSectionFovDegrees;
		// const float vFoV = toDegrees( 2.0f * math<float>::atan( math<float>::tan( toRadians(hFoV) * 0.5f ) / aspect ) );
		const float vFoVDegrees = float( mCamera.getFov() );
		const float vFovRadians = glm::radians( vFoVDegrees );
		const float hFoVRadians = 2.0f * math<float>::atan( math<float>::tan( vFovRadians * 0.5f ) * aspect );
		const float hFovDegrees = glm::degrees( hFoVRadians );

		// bind the frame buffer object
		{
			gl::ScopedFramebuffer fbo( mFbo );

			// store viewport and matrices, so we can restore later
			gl::ScopedViewport viewport( ivec2( 0 ), mFbo->getSize() );
			gl::pushMatrices();

			gl::clear();

			// setup camera
			CameraStereo cam = mCamera.getCamera();
			cam.disableStereo();
			cam.setAspectRatio( aspect );
			cam.setFov( vFoVDegrees );

			vec3 right, up;
			cam.getBillboardVectors( &right, &up );
			const vec3 forward = cross( up, right );

			// render sections
			const float offset = 0.5f * ( mSectionCount - 1 );
			for( unsigned i = 0; i < mSectionCount; ++i ) {
				gl::ScopedViewport scpViewport( i * w, 0, w, h );

				cam.setViewDirection( glm::angleAxis( -mSectionOverlap * hFoVRadians * ( i - offset ), up ) * forward );
				cam.setWorldUp( up );
				gl::setMatrices( cam );
				render();
			}

			// draw user interface
			gl::setMatrices( mCamera.getCamera() );

			if( mDrawUserInterface )
				mUserInterface.draw( ( boost::format( "Cylindrical Projection (%d degrees)" ) % int( hFovDegrees + ( ( mSectionCount - 1 ) * mSectionOverlap ) * hFovDegrees ) ).str() );

			// restore states
			gl::popMatrices();
		}

		// draw frame buffer and perform cylindrical projection using a fragment shader
		if( mShader ) {
			gl::ScopedTextureBind tex0( mFbo->getColorTexture() );

			gl::ScopedGlslProg shader( mShader );
			mShader->uniform( "tex", 0 );
			mShader->uniform( "sides", float( mSectionCount ) );
			mShader->uniform( "radians", mSectionCount * hFoVRadians );
			mShader->uniform( "reciprocal", 0.5f / mSectionCount );

			const Rectf centered = Rectf( mFbo->getBounds() ).getCenteredFit( getWindowBounds(), false );
			gl::drawSolidRect( centered );
			// gl::draw( mFbo->getColorTexture(), centered );
		}
	}
	else {
		mCamera.disableStereo();

		gl::pushMatrices();
		gl::setMatrices( mCamera.getCamera() );
		render();
		gl::popMatrices();

		// draw user interface
		if( mDrawUserInterface )
			mUserInterface.draw( "Perspective Projection" );
	}

		/*// fade in at start of application
		gl::ScopedAlphaBlend blend(false);
		double t = math<double>::clamp( mTimer.getSeconds() / 3.0, 0.0, 1.0 );
		float a = ci::lerp<float>( 1.0f, 0.0f, (float) t );

		if( a > 0.0f ) {
		gl::color( ColorA( 0, 0, 0, a ) );
		gl::drawSolidRect( getWindowBounds() );
		}//*/
#endif

	if( mSun ) {
		gl::ScopedDepth       scpDepth( true );
		gl::ScopedFaceCulling scpCull( true );
		gl::ScopedMatrices    scpMatrices;

		gl::setMatrices( mCamera.getCamera() );
		mSun->draw();
	}
}

void StarsApp::render()
{
	// draw background
	mBackground.draw();

	// draw grid
	if( mIsGridVisible )
		mGrid.draw();

	// draw stars
	mStars.draw();

	// draw constellations
	if( mIsConstellationsVisible )
		mConstellations.draw();

	if( mIsConstellationArtVisible )
		mConstellationArt.draw();

	// draw labels
	if( mIsLabelsVisible ) {
		mLabels.draw();

		if( mIsConstellationsVisible || mIsConstellationArtVisible )
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
#if defined( CINDER_MSW )
	// allows the use of the media buttons on your Windows keyboard to control the music
	switch( event.getNativeKeyCode() ) {
	case VK_MEDIA_NEXT_TRACK:
		// play next music file
		playMusic( getNextFile( mMusicPath ) );
		return;
	case VK_MEDIA_PREV_TRACK:
		// play next music file
		playMusic( getPrevFile( mMusicPath ) );
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

	switch( event.getCode() ) {
	case KeyEvent::KEY_RETURN:
		if( mSun ) {
			try {
				const auto glsl = gl::GlslProg::create( loadAsset( "shaders/sun.vert" ), loadAsset( "shaders/sun.frag" ) );
				mSun->replaceGlslProg( glsl );
				console() << "Shader reloaded." << std::endl;
			}
			catch( const std::exception &exc ) {
				console() << exc.what() << std::endl;
			}
		}
		break;
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
	case KeyEvent::KEY_h:
		if( event.isShiftDown() ) {
			// toggle stars
			mStars.enableStars( !mStars.isStarsEnabled() );
		}
		else {
			// toggle halos
			mStars.enableHalos( !mStars.isHalosEnabled() );
		}
		break;
	case KeyEvent::KEY_l:
		// toggle labels
		mIsLabelsVisible = !mIsLabelsVisible;
		break;
	case KeyEvent::KEY_u:
		// toggle user interface
		mDrawUserInterface = !mDrawUserInterface;
		break;
	case KeyEvent::KEY_c:
		// toggle constellations / art
		if( event.isShiftDown() )
			mIsConstellationArtVisible = !mIsConstellationArtVisible;
		else
			mIsConstellationsVisible = !mIsConstellationsVisible;
		break;
	case KeyEvent::KEY_a:
		// toggle cursor arrow
		if( mIsCursorVisible )
			forceHideCursor();
		else
			forceShowCursor();
		break;
	case KeyEvent::KEY_s:
		// toggle stereoscopic view
		mIsStereoscopic = !mIsStereoscopic;
		mIsCylindrical = false;
		// adjust line width and aspect ratio
		mStars.setAspectRatio( mIsStereoscopic ? 0.5f : 1.0f );
		mGrid.setLineWidth( mIsCylindrical ? 3.0f : 1.5f );
		mConstellations.setLineWidth( mIsCylindrical ? 2.0f : 1.0f );
		mLabels.setScale( mIsStereoscopic ? vec2( 0.5f, 1 ) : vec2( 1 ) );
		mConstellationLabels.setScale( mIsStereoscopic ? vec2( 0.5f, 1 ) : vec2( 1 ) );
		break;
	case KeyEvent::KEY_d:
		// cylindrical panorama
		mIsCylindrical = !mIsCylindrical;
		mIsStereoscopic = false;
		// adjust line width and aspect ratio
		mStars.setAspectRatio( mIsStereoscopic ? 0.5f : 1.0f );
		mGrid.setLineWidth( mIsCylindrical ? 3.0f : 1.5f );
		mConstellations.setLineWidth( mIsCylindrical ? 2.0f : 1.0f );
		mLabels.setScale( mIsStereoscopic ? vec2( 0.5f, 1 ) : vec2( 1 ) );
		mConstellationLabels.setScale( mIsStereoscopic ? vec2( 0.5f, 1 ) : vec2( 1 ) );
		break;
	// case KeyEvent::KEY_RETURN:
	//	createShader();
	//	break;
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
	mStars.resize( getWindowSize() );
}

void StarsApp::fileDrop( FileDropEvent event )
{
	for( size_t i = 0; i < event.getNumFiles(); ++i ) {
		fs::path file = event.getFile( i );

		// skip if not a file
		if( !fs::is_regular_file( file ) )
			continue;

		if( std::find( mMusicExtensions.begin(), mMusicExtensions.end(), file.extension() ) != mMusicExtensions.end() )
			playMusic( file );
	}
}

void StarsApp::playMusic( const fs::path &path, bool loop )
{
	if( mSoundEngine && !path.empty() ) {
		// stop current music
		if( mMusic )
			mMusic->stop();

		// play music in a very safe way
		mMusic = shared_ptr<ISound>( mSoundEngine->play2D( path.string().c_str(), loop, true ), std::mem_fn( &ISound::drop ) );
		if( mMusic )
			mMusic->setIsPaused( false );

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
	shared_ptr<ISound> sound( mSoundEngine->play2D( path.string().c_str(), loop, true ), std::mem_fn( &ISound::drop ) );
	if( sound )
		sound->setIsPaused( false );
}

shared_ptr<ISound> StarsApp::createSound( const fs::path &path )
{
	shared_ptr<ISound> sound;

	if( mSoundEngine && !path.empty() ) {
		// create sound in a very safe way
		sound = shared_ptr<ISound>( mSoundEngine->play3D( path.string().c_str(), vec3df( 0, 0, 0 ), false, true ), std::mem_fn( &ISound::drop ) );
	}

	return sound;
}

void StarsApp::createShader()
{
	const fs::path vs = getAssetPath( "" ) / "shaders/cylindrical.vert";
	const fs::path fs = getAssetPath( "" ) / "shaders/cylindrical.frag";

	//
	try {
		mShader = gl::GlslProg::create( loadFile( vs ), loadFile( fs ) );
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
		mShader = gl::GlslProgRef();
	}
}

void StarsApp::createFbo()
{
	// determine the size of the frame buffer
	const int w = getWindowWidth() * 2;
	const int h = getWindowHeight() * 2;

	if( mFbo && mFbo->getSize() == ivec2( w, h ) )
		return;

	// create the FBO
	gl::Texture2d::Format tfmt;
	tfmt.setWrap( GL_REPEAT, GL_CLAMP_TO_BORDER );

	gl::Fbo::Format fmt;
	fmt.setColorTextureFormat( tfmt );

	mFbo = gl::Fbo::create( w, h, fmt );
}

void StarsApp::forceHideCursor()
{
// forces the cursor to hide
#ifdef WIN32
	while(::ShowCursor( false ) >= 0 )
		;
#else
	hideCursor();
#endif
	mIsCursorVisible = false;
}

void StarsApp::forceShowCursor()
{
// forces the cursor to show
#ifdef WIN32
	while(::ShowCursor( true ) < 0 )
		;
#else
	showCursor();
#endif
	mIsCursorVisible = true;
}

void StarsApp::constrainCursor( const ivec2 &pos )
{
	// keeps the cursor well within the window bounds,
	// so that we can continuously drag the mouse without
	// ever hitting the sides of the screen

	if( pos.x < 50 || pos.x > getWindowWidth() - 50 || pos.y < 50 || pos.y > getWindowHeight() - 50 ) {
#if defined( CINDER_MSW )
		POINT pt;
		mCursorPrevious.x = pt.x = getWindowWidth() / 2;
		mCursorPrevious.y = pt.y = getWindowHeight() / 2;

		const HWND hWnd = getRenderer()->getHwnd();
		::ClientToScreen( hWnd, &pt );
		::SetCursorPos( pt.x, pt.y );
#else
// on MacOS, the results seem to be a little choppy,
// which might have something to do with the OS
// suppressing events for a short while after warping
// the cursor.
// A call to "CGSetLocalEventsSuppressionInterval(0.0)"
// might remedy that.
// Uncomment the code below to try things out.
/*//
ivec2 pt;
mCursorPrevious.x = pt.x = getWindowWidth() / 2;
mCursorPrevious.y = pt.y = getWindowHeight() / 2;

CGPoint target = CGPointMake((float) pt.x, (float) pt.y);
// note: target should first be converted to screen position here
CGWarpMouseCursorPosition(target);
//*/
#endif
	}
}

fs::path StarsApp::getFirstFile( const fs::path &path )
{
	const fs::directory_iterator end_itr;
	for( fs::directory_iterator i( path ); i != end_itr; ++i ) {
		// skip if not a file
		if( !fs::is_regular_file( i->status() ) )
			continue;

		// skip if extension does not match
		if( std::find( mMusicExtensions.begin(), mMusicExtensions.end(), i->path().extension() ) == mMusicExtensions.end() )
			continue;

		// file matches, return it
		return i->path();
	}

	// failed, return empty path
	return fs::path();
}

fs::path StarsApp::getNextFile( const fs::path &current )
{
	if( !current.empty() ) {
		bool useNext = false;

		const fs::directory_iterator end_itr;
		for( fs::directory_iterator i( current.parent_path() ); i != end_itr; ++i ) {
			// skip if not a file
			if( !fs::is_regular_file( i->status() ) )
				continue;

			if( useNext ) {
				// skip if extension does not match
				if( std::find( mMusicExtensions.begin(), mMusicExtensions.end(), i->path().extension() ) == mMusicExtensions.end() )
					continue;

				// file matches, return it
				return i->path();
			}
			else if( i->path() == current ) {
				useNext = true;
			}
		}
	}

	// failed, return empty path
	return fs::path();
}

fs::path StarsApp::getPrevFile( const fs::path &current )
{
	if( !current.empty() ) {
		fs::path previous;

		const fs::directory_iterator end_itr;
		for( fs::directory_iterator i( current.parent_path() ); i != end_itr; ++i ) {
			// skip if not a file
			if( !fs::is_regular_file( i->status() ) )
				continue;

			if( i->path() == current ) {
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
				previous = i->path();
			}
		}
	}

	// failed, return empty path
	return fs::path();
}

// allow easy access to the application from outside
static StarsApp *starsAppPtr()
{
	return static_cast<StarsApp *>( App::get() );
}

CINDER_APP( StarsApp, RendererGl( RendererGl::Options().msaa( 16 ) ), &StarsApp::prepare )
