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

#pragma warning(push)
#pragma warning(disable: 4996) // _CRT_SECURE_NO_WARNINGS

#include "cinder/app/App.h"
#include "cinder/app/Platform.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Environment.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/ConcurrentCircularBuffer.h"
#include "cinder/ImageIo.h"
#include "cinder/Log.h"
#include "cinder/Unicode.h"
#include "cinder/Utilities.h"

#include <time.h>

using namespace ci;
using namespace ci::app;
using namespace std;

class ShaderToyApp : public App {
public:
	static void prepare( Settings* settings );

	void setup() override;
	void cleanup() override;

	void update() override;
	void draw() override;

	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;

	void keyDown( KeyEvent event ) override;

	void resize() override;
	void fileDrop( FileDropEvent event ) override;

	void random();
private:
	//! Sets the ShaderToy uniform variables.
	void setUniforms();

	//! Initializes the loader thread and the shared OpenGL context.
	bool setupLoader();
	//! Shuts down the loader thread and its associated OpenGL context.
	void shutdownLoader();

	//! Our loader thread. Pass context by value, so the thread obtains ownership.
	void loader( gl::ContextRef ctx );

private:
	//! Time in seconds at which the transition to the next shader starts.
	double          mTransitionTime;
	//! Duration in seconds of the transition to the next shader.
	double          mTransitionDuration;
	//! Shader that will perform the transition to the next shader.
	gl::GlslProgRef mShaderTransition;
	//! Currently active shader.
	gl::GlslProgRef mShaderCurrent;
	//! Shader that has just been loaded and we are transitioning to.
	gl::GlslProgRef mShaderNext;
	//! Buffer containing the rendered output of the currently active shader.
	gl::FboRef      mBufferCurrent;
	//! Buffer containing the rendered output of the shader we are transitioning to.
	gl::FboRef      mBufferNext;
	//! Texture slots for our shader, based on ShaderToy.
	gl::TextureRef  mChannel0;
	gl::TextureRef  mChannel1;
	gl::TextureRef  mChannel2;
	gl::TextureRef  mChannel3;
	//! Our mouse position: xy = current position while mouse down, zw = last click position.
	vec4            mMouse;
	//! Keep track of the current path.
	fs::path        mPathCurrent;
	//! Keep track of the next path.
	fs::path        mPathNext;

	//! We will use this structure to pass data from one thread to another.
	struct LoaderData {
		LoaderData() {}
		LoaderData( const fs::path& path, gl::GlslProgRef shader )
			: path( path ), shader( shader )
		{
		}

		//! This constructor allows implicit conversion from path to LoaderData.
		LoaderData( const fs::path& path )
			: path( path )
		{
		}

		fs::path path;
		gl::GlslProgRef shader;
	};
	//! The main thread will push data to this buffer, to be picked up by the loading thread.
	ConcurrentCircularBuffer<LoaderData>* mRequests;
	//! The loading thread will push data to this buffer, to be picked up by the main thread.
	ConcurrentCircularBuffer<LoaderData>* mResponses;
	//! Our loading thread, sharing an OpenGL context with the main thread.
	std::shared_ptr<std::thread>          mThread;
	//! Signals if the loading thread should abort.
	bool                                  mThreadAbort;
};

void ShaderToyApp::prepare( Settings* settings )
{
	// Do not allow resizing our window. Feel free to remove this limitation.
	settings->setResizable( false );
}

void ShaderToyApp::setup()
{
	// Create our thread communication buffers.
	mRequests = new ConcurrentCircularBuffer<LoaderData>( 100 );
	mResponses = new ConcurrentCircularBuffer<LoaderData>( 100 );

	// Start the loading thread.
	if( !setupLoader() ) {
		CI_LOG_E( "Failed to create the loader thread and context." );
		quit(); return;
	}

	// Load our textures and transition shader in the main thread.
	try {
		gl::Texture::Format fmt;
		fmt.setWrap( GL_REPEAT, GL_REPEAT );

		mChannel0 = gl::Texture::create( loadImage( loadAsset( "presets/tex16.png" ) ), fmt );
		mChannel1 = gl::Texture::create( loadImage( loadAsset( "presets/tex06.jpg" ) ), fmt );
		mChannel2 = gl::Texture::create( loadImage( loadAsset( "presets/tex09.jpg" ) ), fmt );
		mChannel3 = gl::Texture::create( loadImage( loadAsset( "presets/tex02.jpg" ) ), fmt );

		mShaderTransition = gl::GlslProg::create( loadAsset( "common/shadertoy.vert" ), loadAsset( "common/shadertoy.frag" ) );
	}
	catch( const std::exception& e ) {
		// Quit if anything went wrong.
		CI_LOG_EXCEPTION( "Failed to load common textures and shaders:", e );
		quit(); return;
	}

	// Tell our loading thread to load the first shader. The path is converted to LoaderData implicitly.
	mRequests->pushFront( getAssetPath( "hell.frag" ) );
}

void ShaderToyApp::cleanup()
{
	// Properly shut down the loading thread.
	shutdownLoader();

	// Properly destroy the buffers.
	if( mResponses ) delete mResponses;
	mResponses = nullptr;
	if( mRequests ) delete mRequests;
	mRequests = nullptr;
}

void ShaderToyApp::update()
{
	LoaderData data;

	// If we are ready for the next shader, take it from the buffer.
	if( !mShaderNext && mResponses->isNotEmpty() ) {
		mResponses->popBack( &data );

		mPathNext = data.path;
		mShaderNext = data.shader;

		// Start the transition.
		mTransitionTime = getElapsedSeconds();
		mTransitionDuration = 2.0;

		// Update the window title.
		getWindow()->setTitle( std::string( "ShaderToyApp - Fading from " ) + mPathCurrent.filename().string() + " to " + mPathNext.filename().string() );
	}
}

void ShaderToyApp::draw()
{
	// Bind textures.
	if( mChannel0 ) mChannel0->bind( 0 );
	if( mChannel1 ) mChannel1->bind( 1 );
	if( mChannel2 ) mChannel2->bind( 2 );
	if( mChannel3 ) mChannel3->bind( 3 );

	// Render the current shader to a frame buffer.
	if( mShaderCurrent && mBufferCurrent ) {
		gl::ScopedFramebuffer fbo( mBufferCurrent );

		// Bind shader.
		gl::ScopedGlslProg shader( mShaderCurrent );
		setUniforms();

		// Clear buffer and draw full screen quad (flipped).
		gl::clear();
		gl::drawSolidRect( Rectf( 0, (float)getWindowHeight(), (float)getWindowWidth(), 0 ) );
	}

	// Render the next shader to a frame buffer.
	if( mShaderNext && mBufferNext ) {
		gl::ScopedFramebuffer fbo( mBufferNext );

		// Bind shader.
		gl::ScopedGlslProg shader( mShaderNext );
		setUniforms();

		// Clear buffer and draw full screen quad (flipped).
		gl::clear();
		gl::drawSolidRect( Rectf( 0, (float)getWindowHeight(), (float)getWindowWidth(), 0 ) );
	}

	// Perform a cross-fade between the two shaders.
	double time = getElapsedSeconds() - mTransitionTime;
	double fade = math<double>::clamp( time / mTransitionDuration, 0.0, 1.0 );

	if( fade <= 0.0 ) {
		// Transition has not yet started. Keep drawing current buffer.
		gl::draw( mBufferCurrent->getColorTexture(), getWindowBounds() );
	}
	else if( fade < 1.0 ) {
		// Transition is in progress.
		// Use a transition shader to avoid having to draw one buffer on top of another.
		gl::ScopedTextureBind tex0( mBufferCurrent->getColorTexture(), 0 );
		gl::ScopedTextureBind tex1( mBufferNext->getColorTexture(), 1 );

		gl::ScopedGlslProg shader( mShaderTransition );
		mShaderTransition->uniform( "iSrc", 0 );
		mShaderTransition->uniform( "iDst", 1 );
		mShaderTransition->uniform( "iFade", (float)fade );

		gl::drawSolidRect( getWindowBounds() );
	}
	else if( mShaderNext ) {
		// Transition is done. Swap shaders.
		gl::draw( mBufferNext->getColorTexture(), getWindowBounds() );

		mShaderCurrent = mShaderNext;
		mShaderNext.reset();

		mPathCurrent = mPathNext;
		mPathNext.clear();

		getWindow()->setTitle( std::string( "ShaderToyApp - Showing " ) + mPathCurrent.filename().string() );
	}
	else {
		// No transition in progress.
		gl::draw( mBufferCurrent->getColorTexture(), getWindowBounds() );
	}
}

void ShaderToyApp::mouseDown( MouseEvent event )
{
	mMouse.x = (float)event.getPos().x;
	mMouse.y = (float)event.getPos().y;
	mMouse.z = (float)event.getPos().x;
	mMouse.w = (float)event.getPos().y;
}

void ShaderToyApp::mouseDrag( MouseEvent event )
{
	mMouse.x = (float)event.getPos().x;
	mMouse.y = (float)event.getPos().y;
}

void ShaderToyApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
		case KeyEvent::KEY_ESCAPE:
			quit();
			break;
		case KeyEvent::KEY_SPACE:
			random();
			break;
	}
}

void ShaderToyApp::resize()
{
	// Create/resize frame buffers (no multisampling)
	mBufferCurrent = gl::Fbo::create( getWindowWidth(), getWindowHeight() );
	mBufferNext = gl::Fbo::create( getWindowWidth(), getWindowHeight() );
}

void ShaderToyApp::fileDrop( FileDropEvent event )
{
	// Send all file requests to the loading thread.
	size_t count = event.getNumFiles();
	for( size_t i = 0; i < count && mRequests->isNotFull(); ++i )
		mRequests->pushFront( event.getFile( i ) );
}

void ShaderToyApp::random()
{
	const fs::path assets = getAssetPath( "" );

	// Find all *.frag files.
	std::vector<fs::path> shaders;
	for( fs::recursive_directory_iterator it( assets ), end; it != end; ++it ) {
		if( fs::is_regular_file( it->path() ) )
			if( it->path().extension() == ".frag" )
				shaders.push_back( it->path() );
	}

	if( shaders.empty() )
		return;

	// Load random *.frag file, but make sure it is different from the current shader.
	size_t idx = getElapsedFrames() % shaders.size();
	if( shaders.at( idx ) == mPathCurrent )
		idx = ( idx + 1 ) % shaders.size();

	if( mRequests->isNotFull() )
		mRequests->pushFront( shaders.at( idx ) );
}

void ShaderToyApp::setUniforms()
{
	auto shader = gl::context()->getGlslProg();
	if( !shader )
		return;

	// Calculate shader parameters.
	vec3  iResolution( vec2( getWindowSize() ), 1 );
	float iGlobalTime = (float)getElapsedSeconds();
	float iChannelTime0 = (float)getElapsedSeconds();
	float iChannelTime1 = (float)getElapsedSeconds();
	float iChannelTime2 = (float)getElapsedSeconds();
	float iChannelTime3 = (float)getElapsedSeconds();
	vec3  iChannelResolution0 = mChannel0 ? vec3( mChannel0->getSize(), 1 ) : vec3( 1 );
	vec3  iChannelResolution1 = mChannel1 ? vec3( mChannel1->getSize(), 1 ) : vec3( 1 );
	vec3  iChannelResolution2 = mChannel2 ? vec3( mChannel2->getSize(), 1 ) : vec3( 1 );
	vec3  iChannelResolution3 = mChannel3 ? vec3( mChannel3->getSize(), 1 ) : vec3( 1 );

	time_t now = time( 0 );
	tm*    t = gmtime( &now );
	vec4   iDate( float( t->tm_year + 1900 ),
				  float( t->tm_mon + 1 ),
				  float( t->tm_mday ),
				  float( t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec ) );
	
	// Set shader uniforms.
	shader->uniform( "iResolution", iResolution );
	shader->uniform( "iGlobalTime", iGlobalTime );
	shader->uniform( "iChannelTime[0]", iChannelTime0 );
	shader->uniform( "iChannelTime[1]", iChannelTime1 );
	shader->uniform( "iChannelTime[2]", iChannelTime2 );
	shader->uniform( "iChannelTime[3]", iChannelTime3 );
	shader->uniform( "iChannelResolution[0]", iChannelResolution0 );
	shader->uniform( "iChannelResolution[1]", iChannelResolution1 );
	shader->uniform( "iChannelResolution[2]", iChannelResolution2 );
	shader->uniform( "iChannelResolution[3]", iChannelResolution3 );
	shader->uniform( "iMouse", mMouse );
	shader->uniform( "iChannel0", 0 );
	shader->uniform( "iChannel1", 1 );
	shader->uniform( "iChannel2", 2 );
	shader->uniform( "iChannel3", 3 );
	shader->uniform( "iDate", iDate );
}

bool ShaderToyApp::setupLoader()
{
	auto ctx = gl::env()->createSharedContext( gl::context() );
	if( !ctx )
		return false;

	// If succeeded, start the loading thread.
	mThreadAbort = false;
	mThread = std::make_shared<std::thread>( &ShaderToyApp::loader, this, ctx );

	return true;
}

void ShaderToyApp::shutdownLoader()
{
	// Tell the loading thread to abort, then wait for it to stop.
	mThreadAbort = true;
	if( mThread )
		mThread->join();
}

void ShaderToyApp::loader( gl::ContextRef ctx )
{
	// This only works if we can make the render context current.
	ctx->makeCurrent();

	// Loading loop.
	while( !mThreadAbort ) {
		// Wait for a request.
		if( mRequests->isNotEmpty() ) {
			// Take the request from the buffer.
			LoaderData data;
			mRequests->popBack( &data );

			// Try to load, parse and compile the shader.
			try {
				std::string vs = loadString( loadAsset( "common/shadertoy.vert" ) );
				std::string fs = loadString( loadAsset( "common/shadertoy.inc" ) ) + loadString( loadFile( data.path ) );

				data.shader = gl::GlslProg::create( gl::GlslProg::Format().vertex( vs ).fragment( fs ) );

				// If the shader compiled successfully, pass it to the main thread.
				mResponses->pushFront( data );
			}
			catch( const std::exception& e ) {
				// Uhoh, something went wrong, but it's not fatal.
				CI_LOG_EXCEPTION( "Failed to compile the shader: ", e );
			}
		}
		else {
			// Allow the CPU to do other things for a while.
			std::chrono::milliseconds duration( 100 );
			std::this_thread::sleep_for( duration );
		}
	}
}

#pragma warning(pop) // _CRT_SECURE_NO_WARNINGS

CINDER_APP( ShaderToyApp, RendererGl, &ShaderToyApp::prepare )
