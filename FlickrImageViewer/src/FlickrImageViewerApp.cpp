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

#include "cinder/CinderMath.h"
#include "cinder/Xml.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "ph/TextureStore.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class FlickrImageViewerApp : public AppBasic {
public:
	void prepareSettings( Settings *settings );
	void setup();	
	void update();
	void draw();

	void mouseDown( MouseEvent event ){};
	void mouseDrag( MouseEvent event ){};
	void mouseUp( MouseEvent event ){};

	void keyDown( KeyEvent event );
protected:
	vector<string>	mUrls;
	gl::Texture		mFront;
	gl::Texture		mBack;

	size_t			mIndex;

	double			mTimeSwapped;
	double			mTimeView;
	double			mTimeFade;
	double			mTimeDuration;

	bool			mAsynchronous;
};

void FlickrImageViewerApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize(600, 600);
	settings->setTitle("Flickr Image Viewer");
	settings->setFrameRate(60.0f);

	// run in windowed mode and watch the Output messages window to learn
	// more about asynchronous loading. Press 'F" to toggle full screen.
	settings->setFullScreen(false);

}

void FlickrImageViewerApp::setup()
{
	// which image are we viewing next?
	mIndex = 0;

	// at what time did we last swap the images?
	mTimeSwapped = 0.0;
	// view each image at least 10 seconds
	mTimeView = 5.0;
	// crossfade images during 2.5 seconds
	mTimeFade = 1.5;
	// how long was the previous image visible?
	mTimeDuration = 0.0;

	// toggle this using the 'A' key to see the advantage of asynchronous loading
	mAsynchronous = true;
}

void FlickrImageViewerApp::update()
{
	// instead of in setup(), I download the feed here so it doesn't take so long for our window to show up
	if(mUrls.empty()) {
		// connect to Flickr and load a set of images. To view your own set,
		// find the "RSS feed" link on the Flickr page and copy the url.
		XmlTree doc( loadUrl("http://api.flickr.com/services/feeds/photoset.gne?set=726262&nsid=14684343@N00") );
		XmlTree::Iter itr = doc.begin("feed/entry/link");
		while(itr!=doc.end())
		{
			// check if link contains an image (type == 'image/jpeg')
			if(itr->getAttributeValue<string>("type") == "image/jpeg") {
				// retrieve and store url
				if( itr->hasAttribute("href") )
					mUrls.push_back( itr->getAttributeValue<string>("href") );
			}

			++itr;
		}

		// make sure we actually have something to show
		if(mUrls.empty()) return;
	}	

	// calculate elapsed time in seconds (since last swap)
	double elapsed = ( getElapsedSeconds() - mTimeSwapped );

	// if there is no front image yet, load it right away
	if(!mFront) {
		if(mAsynchronous) {
			// load the texture asynchronously using the TextureManager. The call
			// will return an empty texture if not ready yet.
			mFront = ph::fetchTexture( mUrls[mIndex] );
		} 
		else {
			// load the texture synchronously using the TextureManager. The call
			// will load and return the texture, but your application will have
			// to wait for it to finish. Returns empty texture if load did not succeed.
			mFront = ph::loadTexture( mUrls[mIndex] );
		}

		// if texture was loaded...
		if(mFront) {
			// start fading in
			mTimeSwapped = getElapsedSeconds();
			// proceed to next texture
			mIndex = (mIndex + 1) % mUrls.size();
		}
	}
	else if(elapsed > mTimeFade) {
		if(mAsynchronous) {
			// as soon as the front image has been faded in, 
			// start loading the back image asynchronously using the TextureManager. 
			// The call will return an empty texture while not ready yet.
			mBack = ph::fetchTexture( mUrls[mIndex] );
		}
		else {
			// load the texture synchronously using the TextureManager. The call
			// will load and return the texture, but your application will have
			// to wait for it to finish. Returns empty texture if load did not succeed.
			mBack = ph::loadTexture( mUrls[mIndex] );
		}

		// if texture has been loaded and enough time has passed...
		if(mBack && elapsed > (mTimeFade + mTimeView)) {
			// swap the two textures
			gl::Texture temp = mFront; mFront = mBack; mBack = temp;
			// keep track of how long the previous image was shown,
			// so there will be no visual indication that we swapped the images
			mTimeDuration = elapsed;
			// start fading in
			mTimeSwapped = getElapsedSeconds();
			// proceed to next texture
			mIndex = (mIndex + 1) % mUrls.size();
		}
	}
}

void FlickrImageViewerApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );

	// calculate elapsed time in seconds (since last swap)
	double elapsed = ( getElapsedSeconds() - mTimeSwapped );
	// calculate crossfade alpha value
	double fade = ci::math<double>::clamp(elapsed / mTimeFade, 0.0, 1.0);
	// calculate zoom factor 
	float zoom = getWindowWidth() / 600.0f * 0.025f;

	// draw back image
	if(mBack) {
		Rectf image = mBack.getBounds();
		Rectf window = getWindowBounds();
		float sx = window.getWidth() / image.getWidth();	// scale width to fit window
		float sy = window.getHeight() / image.getHeight();	// scale height to fit window
		float s = ci::math<float>::max(sx, sy) 
			+ zoom * (float) (elapsed + mTimeDuration);	// fit window and zoom over time
		float w = image.getWidth() * s;						// resulting width
		float h = image.getHeight() * s;					// resulting height
		float ox = -0.5f * (w - window.getWidth());			// horizontal position to keep image centered
		float oy = -0.5f * (h - window.getHeight());		// vertical position to keep image centered
		image.set(ox, oy, ox+w, oy+h);

		gl::color( Color(1, 1, 1) );
		gl::draw( mBack, image );
	}

	// draw front image
	if(mFront) {
		Rectf image = mFront.getBounds();
		Rectf window = getWindowBounds();
		float sx = window.getWidth() / image.getWidth();
		float sy = window.getHeight() / image.getHeight();
		float s = ci::math<float>::max(sx, sy) + zoom * (float) elapsed;
		float w = image.getWidth() * s;
		float h = image.getHeight() * s;
		float ox = -0.5f * (w - window.getWidth());
		float oy = -0.5f * (h - window.getHeight());
		image.set(ox, oy, ox+w, oy+h);
		
		gl::color( ColorA(1, 1, 1, (float) fade) );
		gl::enableAlphaBlending();
		gl::draw( mFront, image );
		gl::disableAlphaBlending();
	}

}

void FlickrImageViewerApp::keyDown( KeyEvent event )
{
	switch(event.getCode())
	{
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_a:
		// toggle synchronous and asynchronous loading
		mAsynchronous = !mAsynchronous;
		if(mAsynchronous) 
			console() << "Asynchronous loading ENABLED." << std::endl;
		else console() << "Asynchronous loading DISABLED." << std::endl;
		break;
	case KeyEvent::KEY_f:
		// toggle full screen
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_v:
		gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
		break;
	}
}

CINDER_APP_BASIC( FlickrImageViewerApp, RendererGl )