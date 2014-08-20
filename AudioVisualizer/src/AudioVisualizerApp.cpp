/*
 Copyright (c) 2014, Paul Houx - All rights reserved.
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

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"
#include "cinder/Camera.h"
#include "cinder/Channel.h"
#include "cinder/ImageIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Rand.h"

#include "FMOD.hpp"

// Channel callback function used by FMOD to notify us of channel events
FMOD_RESULT F_CALLBACK channelCallback(FMOD_CHANNEL *channel, FMOD_CHANNEL_CALLBACKTYPE type, void *commanddata1, void *commanddata2);


using namespace ci;
using namespace ci::app;
using namespace std;

class AudioVisualizerApp : public AppNative {
public:
	void prepareSettings( Settings* settings );

	void setup();
	void shutdown();
	void update();
	void draw();

	void mouseDown( MouseEvent event );	
	void mouseDrag( MouseEvent event );	
	void mouseUp( MouseEvent event );	
	void keyDown( KeyEvent event );
	void resize();

	// lists all audio files in the given directory
	void		listAudio( const fs::path& directory, vector<fs::path>&	list );
	// show the open file dialog and let the user select an audio file
	fs::path	openAudio( const fs::path& directory );
	// find the first audio file in a given directory
	fs::path	findAudio( const fs::path& directory );
	// find the previous audio file relative to the given file
	fs::path	prevAudio( const fs::path& file );
	// find the next audio file relative to the given file
	fs::path	nextAudio( const fs::path& file );
	// play the audio file
	void		playAudio( const fs::path& file );
	// stop playing the current audio file
	void		stopAudio();

private:
	// width and height of our mesh
	static const int kWidth = 512;
	static const int kHeight = 512;

	// number of frequency bands of our spectrum
	static const int kBands = 1024;
	static const int kHistory = 128;

	Channel32f			mChannelLeft;
	Channel32f			mChannelRight;
	CameraPersp			mCamera;
	MayaCamUI			mMayaCam;
	gl::GlslProg		mShader;
	gl::Texture			mTextureLeft;
	gl::Texture			mTextureRight;
	gl::Texture::Format	mTextureFormat;
	gl::VboMesh			mMesh;
	uint32_t			mOffset;

	FMOD::System*		mFMODSystem;
	FMOD::Sound*		mFMODSound;
	FMOD::Channel*		mFMODChannel;

	bool				mIsMouseDown;
	bool				mIsAudioPlaying;
	double				mMouseUpTime;
	double				mMouseUpDelay;

	vector<string>		mAudioExtensions;
	fs::path			mAudioPath;

public:
	bool				signalChannelEnd;
};

void AudioVisualizerApp::prepareSettings(Settings* settings)
{
	settings->setFullScreen(false);
	settings->setWindowSize(1280, 720);
}

void AudioVisualizerApp::setup()
{
	// initialize signals
	signalChannelEnd = false;

	// make a list of valid audio file extensions and initialize audio variables
	const char* extensions[] = {"mp3", "wav", "ogg"};
	mAudioExtensions = vector<string>(extensions, extensions+2);
	mAudioPath = getAssetPath("");
	mIsAudioPlaying = false;

	// setup camera
	mCamera.setPerspective(50.0f, 1.0f, 1.0f, 10000.0f);
	mCamera.setEyePoint( Vec3f(-kWidth/4, kHeight/2, -kWidth/8) );
	mCamera.setCenterOfInterestPoint( Vec3f(kWidth/4, -kHeight/8, kWidth/4) );

	// create channels from which we can construct our textures
	mChannelLeft = Channel32f(kBands, kHistory);
	mChannelRight = Channel32f(kBands, kHistory);
	memset(	mChannelLeft.getData(), 0, mChannelLeft.getRowBytes() * kHistory );
	memset(	mChannelRight.getData(), 0, mChannelRight.getRowBytes() * kHistory );

	// create texture format (wrap the y-axis, clamp the x-axis)
	mTextureFormat.setWrapS( GL_CLAMP );
	mTextureFormat.setWrapT( GL_REPEAT );
	mTextureFormat.setMinFilter( GL_LINEAR );
	mTextureFormat.setMagFilter( GL_LINEAR );

	// compile shader
	try {
		mShader = gl::GlslProg( loadAsset("shaders/spectrum.vert"), loadAsset("shaders/spectrum.frag") );
	}
	catch( const std::exception& e ) {
		console() << e.what() << std::endl;
		quit();
		return;
	}

	// create static mesh (all animation is done in the vertex shader)
	std::vector<Vec3f>      vertices;
	std::vector<Colorf>     colors;
	std::vector<Vec2f>      coords;
	std::vector<uint32_t>	indices;
	
	for(size_t h=0;h<kHeight;++h)
	{
		for(size_t w=0;w<kWidth;++w)
		{
			// add polygon indices
			if(h < kHeight-1 && w < kWidth-1)
			{
				size_t offset = vertices.size();

				indices.push_back(offset);
				indices.push_back(offset+kWidth);
				indices.push_back(offset+kWidth+1);
				indices.push_back(offset);
				indices.push_back(offset+kWidth+1);
				indices.push_back(offset+1);
			}

			// add vertex
			vertices.push_back( Vec3f(float(w), 0, float(h)) );

			// add texture coordinates
			// note: we only want to draw the lower part of the frequency bands,
			//  so we scale the coordinates a bit
			const float part = 0.5f;
			float s = w / float(kWidth-1);
			float t = h / float(kHeight-1);
			coords.push_back( Vec2f(part - part * s, t) );

			// add vertex colors
			colors.push_back( Color(CM_HSV, s, 0.5f, 0.75f) );
		}
	}

	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticColorsRGB();
	layout.setStaticIndices();
	layout.setStaticTexCoords2d();

	mMesh = gl::VboMesh(vertices.size(), indices.size(), layout, GL_TRIANGLES);
	mMesh.bufferPositions(vertices);
	mMesh.bufferColorsRGB(colors);
	mMesh.bufferIndices(indices);
	mMesh.bufferTexCoords2d(0, coords);

	// play audio using the Cinder FMOD block
	FMOD::System_Create( &mFMODSystem );
	mFMODSystem->init( 32, FMOD_INIT_NORMAL | FMOD_INIT_ENABLE_PROFILE, NULL );
	mFMODSound = nullptr;
	mFMODChannel = nullptr;

	playAudio( findAudio( mAudioPath ) );
	
	mIsMouseDown = false;
	mMouseUpDelay = 30.0;
	mMouseUpTime = getElapsedSeconds() - mMouseUpDelay;

	// the texture offset has two purposes:
	//  1) it tells us where to upload the next spectrum data
	//  2) we use it to offset the texture coordinates in the shader for the scrolling effect
	mOffset = 0;
}

void AudioVisualizerApp::shutdown()
{
	// properly shut down FMOD
	stopAudio();

	if(mFMODSystem)
		mFMODSystem->release();
}

void AudioVisualizerApp::update()
{
	// update FMOD so it can notify us of events
	mFMODSystem->update();

	// handle signal: if audio has ended, play next file
	if(mIsAudioPlaying && signalChannelEnd)
		playAudio( nextAudio( mAudioPath ) );

	// reset FMOD signals
	signalChannelEnd= false;

	// get spectrum for left and right channels and copy it into our channels
	float* pDataLeft = mChannelLeft.getData() + kBands * mOffset;
	float* pDataRight = mChannelRight.getData() + kBands * mOffset;

	mFMODSystem->getSpectrum( pDataLeft, kBands, 0, FMOD_DSP_FFT_WINDOW_HANNING );	
	mFMODSystem->getSpectrum( pDataRight, kBands, 1, FMOD_DSP_FFT_WINDOW_HANNING );

	// increment texture offset
	mOffset = (mOffset+1) % kHistory;

	// clear the spectrum for this row to avoid old data from showing up
	pDataLeft = mChannelLeft.getData() + kBands * mOffset;
	pDataRight = mChannelRight.getData() + kBands * mOffset;
	memset( pDataLeft, 0, kBands * sizeof(float) );
	memset( pDataRight, 0, kBands * sizeof(float) );

	// animate camera if mouse has not been down for more than 30 seconds
	if(!mIsMouseDown && (getElapsedSeconds() - mMouseUpTime) > mMouseUpDelay)
	{
		float t = float( getElapsedSeconds() );
		float x = 0.5f + 0.5f * math<float>::cos( t * 0.07f );
		float y = 0.1f - 0.2f * math<float>::sin( t * 0.09f );
		float z = 0.25f * math<float>::sin( t * 0.05f ) - 0.25f;
		Vec3f eye = Vec3f(kWidth * x, kHeight * y, kHeight * z);

		x = 1.0f - x;
		y = -0.3f;
		z = 0.6f + 0.2f *  math<float>::sin( t * 0.12f );
		Vec3f interest = Vec3f(kWidth * x, kHeight * y, kHeight * z);

		// gradually move to eye position and center of interest
		mCamera.setEyePoint( eye.lerp(0.995f, mCamera.getEyePoint()) );
		mCamera.setCenterOfInterestPoint( interest.lerp(0.990f, mCamera.getCenterOfInterestPoint()) );
	}
}

void AudioVisualizerApp::draw()
{
	gl::clear();

	// use camera
	gl::pushMatrices();
	gl::setMatrices(mCamera);
	{
		// bind shader
		mShader.bind();
		mShader.uniform("uTexOffset", mOffset / float(kHistory));
		mShader.uniform("uLeftTex", 0);
		mShader.uniform("uRightTex", 1);

		// create textures from our channels and bind them
		mTextureLeft = gl::Texture(mChannelLeft, mTextureFormat);
		mTextureRight = gl::Texture(mChannelRight, mTextureFormat);

		mTextureLeft.enableAndBind();
		mTextureRight.bind(1);

		// draw mesh using additive blending
		gl::enableAdditiveBlending();
		gl::color( Color(1, 1, 1) );
		gl::draw( mMesh );
		gl::disableAlphaBlending();

		// unbind textures and shader
		mTextureRight.unbind();
		mTextureLeft.unbind();
		mShader.unbind();
	}
	gl::popMatrices();
}

void AudioVisualizerApp::mouseDown( MouseEvent event )
{
	// handle mouse down
	mIsMouseDown = true;

	mMayaCam.setCurrentCam(mCamera);
	mMayaCam.mouseDown( event.getPos() );
}

void AudioVisualizerApp::mouseDrag( MouseEvent event )
{
	// handle mouse drag
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	mCamera = mMayaCam.getCamera();
}

void AudioVisualizerApp::mouseUp( MouseEvent event )
{
	// handle mouse up
	mMouseUpTime = getElapsedSeconds();
	mIsMouseDown = false;
}

void AudioVisualizerApp::keyDown( KeyEvent event )
{
	// handle key down
	switch( event.getCode() )
	{
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_F4:
		if( event.isAltDown() )
			quit();
		break;
	case KeyEvent::KEY_LEFT:
		playAudio( prevAudio( mAudioPath ) );
		break;
	case KeyEvent::KEY_RIGHT:
		playAudio( nextAudio( mAudioPath ) );
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_o:
		playAudio( openAudio( mAudioPath ) );
		break;
	case KeyEvent::KEY_p:
		playAudio( mAudioPath );
		break;
	case KeyEvent::KEY_s:
		stopAudio();
		break;
	}
}

void AudioVisualizerApp::resize()
{
	// handle resize
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

void AudioVisualizerApp::listAudio(const fs::path& directory, vector<fs::path>& list)
{
	// clear the list
	list.clear();

	if(directory.empty() || !fs::is_directory(directory))
		return;

	// make a list of all audio files in the directory
	fs::directory_iterator end_itr;
	for( fs::directory_iterator i( directory ); i != end_itr; ++i )
	{
		// skip if not a file
		if( !fs::is_regular_file( i->status() ) ) continue;

		// skip if extension does not match
		string extension = i->path().extension().string();
		extension.erase(0, 1);
		if( std::find( mAudioExtensions.begin(), mAudioExtensions.end(), extension ) == mAudioExtensions.end() )
			continue;

		// file matches
		list.push_back(i->path());
	}
}

fs::path AudioVisualizerApp::openAudio(const fs::path& directory)
{	
	// only works if not full screen	
	bool wasFullScreen = isFullScreen();
	setFullScreen( false );

	fs::path file = getOpenFilePath( directory, mAudioExtensions );

	setFullScreen( wasFullScreen );

	return file;
}

fs::path AudioVisualizerApp::findAudio( const fs::path& directory )
{
	vector<fs::path> files;
	listAudio(directory, files);

	// if available, return the first audio file
	if(!files.empty())
		return files.front();

	// failed, let user select file using dialog
	return openAudio( directory );
}

fs::path AudioVisualizerApp::prevAudio(const fs::path& file)
{
	if(file.empty() || !fs::is_regular_file(file))
		return fs::path();

	fs::path directory = file.parent_path();

	// make a list of all audio files in the directory
	vector<fs::path> files;
	listAudio(directory, files);

	// return if there are no audio files in the directory
	if(files.empty())
		return fs::path();

	// find current audio file
	auto itr = std::find( files.begin(), files.end(), file );

	// if not found, or if it is the first audio file, simply return last audio file
	if(itr == files.end() || itr == files.begin())
		return files.back();

	// return previous file
	return *(--itr);
}

fs::path AudioVisualizerApp::nextAudio(const fs::path& file)
{
	if(file.empty() || !fs::is_regular_file(file))
		return fs::path();

	fs::path directory = file.parent_path();

	// make a list of all audio files in the directory
	vector<fs::path> files;
	listAudio(directory, files);

	// return if there are no audio files in the directory
	if(files.empty())
		return fs::path();

	// find current audio file
	auto itr = std::find( files.begin(), files.end(), file );

	// if not found, or if it is the last audio file, simply return first audio file
	if(itr == files.end() || *itr == files.back())
		return files.front();

	// return next file
	return *(++itr);
}

void AudioVisualizerApp::playAudio(const fs::path& file)
{
	FMOD_RESULT err;

	// ignore if this is not a file
	if(file.empty() || !fs::is_regular_file( file ))
		return;

	// if audio is already playing, stop it first
	stopAudio();

	// stream the audio
	err = mFMODSystem->createStream( file.string().c_str(), FMOD_SOFTWARE, NULL, &mFMODSound );
	err = mFMODSystem->playSound( FMOD_CHANNEL_FREE, mFMODSound, false, &mFMODChannel );

	// we want to be notified of channel events
	err = mFMODChannel->setCallback( channelCallback );
	
	// keep track of the audio file
	mAudioPath = file;
	mIsAudioPlaying = true;

	// 
	console() << "Now playing:" << mAudioPath.filename().string() << std::endl;
}

void AudioVisualizerApp::stopAudio()
{	
	FMOD_RESULT err;

	mIsAudioPlaying = false;

	if(!mFMODChannel || !mFMODSound)
		return;

	// we don't want to be notified of channel events any longer
	mFMODChannel->setCallback(0);

	bool isPlaying;
	err = mFMODChannel->isPlaying(&isPlaying);
	if(isPlaying)
		err = mFMODChannel->stop();

	err = mFMODSound->release();

	mFMODSound = nullptr;
	mFMODChannel = nullptr;
}

// Channel callback function used by FMOD to notify us of channel events
FMOD_RESULT F_CALLBACK channelCallback(FMOD_CHANNEL *channel, FMOD_CHANNEL_CALLBACKTYPE type, void *commanddata1, void *commanddata2)
{
	// we first need access to the application instance
	AudioVisualizerApp* pApp = static_cast<AudioVisualizerApp*>( App::get() );

	// now handle the callback
	switch(type)
	{
	case FMOD_CHANNEL_CALLBACKTYPE_END:
		// we can't call a function directly, because we are inside the FMOD thread,
		// so let's notify the application instead by setting a boolean (which is thread safe).
		pApp->signalChannelEnd = true;
		break;
    default:
        break;
	}

	return FMOD_OK;
}

CINDER_APP_NATIVE( AudioVisualizerApp, RendererGl )
