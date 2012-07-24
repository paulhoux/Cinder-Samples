/*
Copyright (C) 2010-2012 Paul Houx

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// get rid of a very annoying warning caused by boost::thread::sleep()
#pragma warning(push)
#pragma warning(disable: 4244)

#include "cinder/app/App.h"
#include "cinder/ip/Resize.h"
#include "ph/TextureStore.h"

namespace ph {

using namespace ci;
using namespace ci::app;
using namespace std;

TextureStore::TextureStore(void)
{
	// initialize buffers
	mTextures.clear();
	mSurfaces.clear();
	mThreads.clear();

	// create worker thread for each CPU
	unsigned int numThreads = boost::thread::hardware_concurrency();
	for(unsigned int i=0;i<numThreads;++i)
		mThreads.push_back( boost::shared_ptr<boost::thread>(new boost::thread(&TextureStore::threadLoad, this)) );
}

TextureStore::~TextureStore(void)
{
	// stop loader threads and wait for them to finish
	TextureStoreThreadPool::const_iterator itr;
	for(itr=mThreads.begin();itr!=mThreads.end();++itr) {
		(*itr)->interrupt();
		(*itr)->join();
	}

	// clear buffers
	mThreads.clear();
	mSurfaces.clear();
	mTextures.clear();
}

gl::Texture TextureStore::load(const string &url, gl::Texture::Format fmt)
{
	// if texture already exists, return it immediately
	if (mTextures.find( url ) != mTextures.end())
		return mTextures[ url ];

	// otherwise, check if the image has loaded and create a texture for it
	ci::Surface surface;
	if( mSurfaces.try_pop(url, surface) ) {
		// done loading
		mLoadingQueue.erase(url);

		// perform garbage collection to make room for new textures
		garbageCollect();
		
#ifdef _DEBUG 
		console() << getElapsedSeconds() << ": creating Texture for '" << url << "'." << endl;
#endif
		ph::Texture tex( surface, fmt );
		if(tex) {
			mTextures[ url ] = tex;
			return tex;
		}
	}

	// perform garbage collection to make room for new textures
	garbageCollect();

	// load texture and add to TextureList
#ifdef _DEBUG 
	console() << "Loading Texture '" << url << "'." << endl;
#endif
	try
	{
		ImageSourceRef img = loadImage(url);
		ph::Texture texture = ph::Texture( img, fmt );
		mTextures[ url ] = texture;

		return texture;
	}
	catch(...){}

	try
	{
		ImageSourceRef img = loadImage( loadUrl( Url(url) ) );
		ph::Texture texture = ph::Texture( img, fmt );
		mTextures[ url ] = texture;

		return texture;
	}
	catch(...){}

	// did not succeed
#ifdef _DEBUG 
	console() << getElapsedSeconds() << ": error loading texture '" << url << "'!" << endl;
#endif
	return gl::Texture();
}

gl::Texture TextureStore::fetch(const string &url, gl::Texture::Format fmt)
{
	// if texture already exists, return it immediately
	if (mTextures.find( url ) != mTextures.end())
		return mTextures[ url ];

	// otherwise, check if the image has loaded and create a texture for it
	ci::Surface surface;
	if( mSurfaces.try_pop(url, surface) ) {
		// done loading
		mLoadingQueue.erase(url);

		// perform garbage collection to make room for new textures
		garbageCollect();
		
#ifdef _DEBUG 
		console() << getElapsedSeconds() << ": creating Texture for '" << url << "'." << endl;
#endif
		ph::Texture tex( surface, fmt );
		if(tex) {
			mTextures[ url ] = tex;
			return tex;
		}
	}

	// add to list of currently loading/scheduled files 
	if( mLoadingQueue.push_back(url, true) ) {	
		// hand over to threaded loader
		if( mQueue.push_back(url, true) ) {
#ifdef _DEBUG 
	console() << getElapsedSeconds() << ": queueing Texture '" << url << "' for loading." << endl;
#endif
		}
	}

	return empty();
}

bool TextureStore::abort(const string &url)
{
	mLoadingQueue.erase_all(url);
	return mQueue.erase_all(url);
}

vector<string> TextureStore::getLoadExtensions()
{
	// TODO: ImageIO::getLoadExtensions() doesn't work, but use that instead once it does

	vector<string> result;
	result.push_back(".jpg");
	result.push_back(".png");

	return result;
}

bool TextureStore::isLoading(const string &url)
{
	return mLoadingQueue.contains(url);
}

bool TextureStore::isLoaded(const string &url)
{
	return (mTextures.find( url ) != mTextures.end());
}

void TextureStore::garbageCollect()
{
	for(std::map<std::string, ph::Texture>::iterator itr=mTextures.begin();itr!=mTextures.end();)
	{
		if(itr->second.getUseCount() < 2)
		{
#ifdef _DEBUG 
			console() << getElapsedSeconds() << ": removing texture '" << itr->first << "' because it is no longer in use." << endl;
#endif
			//itr = mTextures.erase(itr); //no return type for std::map erase();
            //this should work
            mTextures.erase(itr++);
		}
		else
			++itr;
	}
}

//

void TextureStore::threadLoad()
{
	bool			succeeded;
	Surface			surface;
	ImageSourceRef	image;
	string			url;

	// run until interrupted
	while(true) {
		mQueue.wait_and_pop_front(url);

		// try to load image
		succeeded = false;

		// try to load from FILE (fastest)
		if(!succeeded) try { 
			image = ci::loadImage( ci::loadFile( url ) ); 
			succeeded = true;
		} catch(...) {}

		// try to load from ASSET (fast)
		if(!succeeded) try { 
			image = ci::loadImage( ci::app::loadAsset( url ) ); 
			succeeded = true;
		} catch(...) {}

		// try to load from URL (slow)
		if(!succeeded) try { 
			image = ci::loadImage( ci::loadUrl( Url(url) ) ); 
			succeeded = true;
		} catch(...) {}

		// do NOT continue if not succeeded (yeah, it's confusing, I know)
		if(!succeeded) continue;

		// succeeded, check if thread was interrupted
		try { boost::this_thread::interruption_point(); }
		catch(boost::thread_interrupted) { break; }

		// create Surface from the image
		surface = Surface(image);

		// check if thread was interrupted
		try { boost::this_thread::interruption_point(); }
		catch(boost::thread_interrupted) { break; }

		// resize image if larger than 4096 px
		Area source = surface.getBounds();
		Area dest(0, 0, 4096, 4096);
		Area fit = Area::proportionalFit(source, dest, false, false);
				
		if(source.getSize() != fit.getSize()) 
			surface = ci::ip::resizeCopy(surface, source, fit.getSize());

		// check if thread was interrupted
		try { boost::this_thread::interruption_point(); }
		catch(boost::thread_interrupted) { break; }

		// copy to main thread
		mSurfaces.push(url, surface);
	}
}

} // namespace ph

//
#pragma warning(pop)
