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

// get rid of a very annoying warning caused by boost::thread::sleep()
#pragma warning(push)
#pragma warning(disable: 4244)

#include "cinder/app/App.h"
#include "cinder/ip/Resize.h"
#include "cinder/Log.h"

#include "ph/TextureStore.h"

namespace ph {

using namespace ci;
using namespace ci::app;
using namespace std;

TextureStore::TextureStore( void )
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

gl::TextureRef TextureStore::load( const string &url, gl::Texture::Format fmt )
{
	// if texture already exists, return it immediately
	auto itr = mTextures.find( url );
	if( itr != mTextures.end() ) {
		if( !itr->second.expired() )
			return itr->second.lock();
	}

	// otherwise, check if the image has loaded and create a texture for it
	ci::Surface surface;
	if( mSurfaces.try_pop( url, surface ) ) {
		// done loading
		mLoadingQueue.erase( url );

		CI_LOG_V( "Creating texture for '" << url << "'." );
		fmt.setDeleter( CustomDeleter() );
		ci::gl::TextureRef tex = gl::Texture::create( surface, fmt );
		return storeTexture( url, tex );
	}

	// load texture and add to TextureList
	CI_LOG_V( "Loading Texture '" << url << "'." );
	try {
		ImageSourceRef img = loadImage( url );
		fmt.setDeleter( CustomDeleter() );
		ci::gl::TextureRef tex = ci::gl::Texture::create( img, fmt );
		return storeTexture( url, tex );
	}
	catch( ... ) {}

	try {
		ImageSourceRef img = loadImage( loadUrl( Url( url ) ) );
		fmt.setDeleter( CustomDeleter() );
		ci::gl::TextureRef tex = ci::gl::Texture::create( img, fmt );
		return storeTexture( url, tex );
	}
	catch( ... ) {}

	// did not succeed
	CI_LOG_V( "Error loading texture '" << url << "'!" );
	return empty();
}

gl::TextureRef TextureStore::fetch( const string &url, gl::Texture::Format fmt )
{
	// if texture already exists, return it immediately
	auto itr = mTextures.find( url );
	if( itr != mTextures.end() ) {
		if( !itr->second.expired() )
			return itr->second.lock();
	}

	// otherwise, check if the image has loaded and create a texture for it
	ci::Surface surface;
	if( mSurfaces.try_pop( url, surface ) ) {
		// done loading
		mLoadingQueue.erase( url );

		CI_LOG_V( "Creating Texture for '" << url << "'." );
		fmt.setDeleter( CustomDeleter() );
		ci::gl::TextureRef tex = gl::Texture::create( surface, fmt );
		return storeTexture( url, tex );
	}

	// add to list of currently loading/scheduled files 
	if( mLoadingQueue.push_back( url, true ) ) {
		// hand over to threaded loader
		if( mQueue.push_back( url, true ) ) {
			CI_LOG_V( "Queueing Texture '" << url << "' for loading." );
		}
	}

	return empty();
}

bool TextureStore::abort( const string &url )
{
	mLoadingQueue.erase_all( url );
	return mQueue.erase_all( url );
}

vector<string> TextureStore::getLoadExtensions()
{
	// TODO: ImageIO::getLoadExtensions() doesn't work, but use that instead once it does

	vector<string> result;
	result.push_back( ".jpg" );
	result.push_back( ".png" );

	return result;
}

bool TextureStore::isLoading( const string &url )
{
	return mLoadingQueue.contains( url );
}

bool TextureStore::isLoaded( const string &url )
{
	return ( mTextures.find( url ) != mTextures.end() );
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

ci::gl::TextureRef TextureStore::storeTexture( const std::string& url, const ci::gl::TextureRef& src )
{
	mTextures[url] = std::weak_ptr<ci::gl::Texture>( src );

	CI_LOG_V( "Texture stored! " << mTextures.size() << " textures in total." );

	return src;
}

void TextureStore::CustomDeleter::operator()( ci::gl::TextureBase* ptr )
{
	// We know one of our textures has just been deleted. Find it in the map and remove it.
	if( ptr ) {
		auto& map = TextureStore::getInstance().mTextures;
		for( auto itr = map.begin(); itr != map.end(); ++itr ) {
			if( itr->second.expired() ) {
				map.erase( itr );
				break;
			}
		}

		CI_LOG_V( "Texture Deleted! " << map.size() << " textures remaining." );

		delete ptr;
	}
}


} // namespace ph
//
#pragma warning(pop)