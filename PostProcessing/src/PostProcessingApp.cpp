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

#pragma comment(lib, "QTMLClient.lib")
#pragma comment(lib, "CVClient.lib")

#include "cinder/Filesystem.h"
#include "cinder/ImageIo.h"
#include "cinder/Matrix.h"
#include "cinder/Surface.h"
#include "cinder/Utilities.h"

#include "cinder/app/AppBasic.h"

#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/qtime/QuickTime.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PostProcessingApp : public AppBasic {
public:
	void prepareSettings( Settings *settings );
	void setup();
	void update();
	void draw();

	void keyDown( KeyEvent event );
	void fileDrop( FileDropEvent event );

	void play( const fs::path &path );
	void playNext();
protected:
	gl::Texture				mImage;
	gl::GlslProg			mShader;
	qtime::MovieSurface		mMovie;
	fs::path				mFile;
};

void PostProcessingApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize(1024, 768);
	settings->setFrameRate(30.0f);
	settings->setTitle("Post-processing Video Player");
}

void PostProcessingApp::setup()
{
	// load test image
	try { mImage = gl::Texture( loadImage( loadAsset("test.png") ) ); }
	catch( const std::exception &e ) { console() << "Could not load image: " << e.what() << std::endl; }

	// load post-processing shader
	//  adapted from a shader by Iñigo Quílez ( http://www.iquilezles.org/ )
	try { mShader = gl::GlslProg( loadAsset("post_process_vert.glsl"), loadAsset("post_process_frag.glsl") ); }
	catch( const std::exception &e ) { console() << "Could not load & compile shader: " << e.what() << std::endl; quit(); }
}

void PostProcessingApp::update()
{
	// update movie texture if necessary
	if(mMovie) { 
		// get movie surface
		Surface surf = mMovie.getSurface();

		// copy surface into texture
		if(surf)
			mImage = gl::Texture( surf );

		// play next movie in directory when done
		if( mMovie.isDone() ) 
			playNext();
	}
}

void PostProcessingApp::draw()
{
	// clear window
	gl::clear();

	// bind shader and set shader variables
	mShader.bind();
	mShader.uniform( "tex0", 0 );
	mShader.uniform( "time", (float)getElapsedSeconds() );

	// draw image or video
	gl::color( Color::white() );
	gl::draw( mImage, getWindowBounds() );

	// unbind shader
	mShader.unbind();
}

void PostProcessingApp::keyDown( KeyEvent event )
{	
	switch( event.getCode() ) {
		case KeyEvent::KEY_ESCAPE:
			quit();
			break;
		case KeyEvent::KEY_f:
			setFullScreen( !isFullScreen() );
			break;
	}
}

void PostProcessingApp::fileDrop( FileDropEvent event )
{	
	// use the last of the dropped files
	mFile = event.getFile( event.getNumFiles() - 1 );

	try { 
		// try loading image file
		mImage = gl::Texture( loadImage( mFile ) );
	}
	catch(...) {
		// otherwise, try loading QuickTime video
		play( mFile );
	}	
}

void PostProcessingApp::play( const fs::path &path ) 
{
	try {
		// try loading QuickTime movie
		mMovie = qtime::MovieSurface( path );
		mMovie.play();
	}
	catch(...) {}

	// keep track of file
	mFile = path;
}

void PostProcessingApp::playNext()
{
	// get directory
	fs::path path = mFile.parent_path();

	// list *.mov files
	vector<string> files;
	string filter( ".mov" );

	fs::directory_iterator end_itr;
	for(fs::directory_iterator itr(path);itr!=end_itr;++itr) {
		// skip if not a file
		if( !boost::filesystem::is_regular_file( itr->status() ) ) continue;
		
		// skip if no match
		if( itr->path().filename().string().find( filter ) == string::npos ) continue;

		// file matches, store it
		files.push_back( itr->path().string() );
	}

	// check if playable files are found
	if( files.empty() ) return;

	// play next file
	vector<string>::iterator itr = find(files.begin(), files.end(), mFile);
	if( itr == files.end() ) {
		play( files[0] );
	}
	else {
		++itr;
		if( itr == files.end() ) 
			play( files[0] );
		else play( *itr );
	}
}

CINDER_APP_BASIC( PostProcessingApp, RendererGl )
