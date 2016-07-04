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

#pragma once

#include "cinder/Cinder.h"
#include "cinder/gl/Texture.h"

#include "ph/ConcurrentDeque.h"
#include "ph/ConcurrentMap.h"

namespace ph {

typedef std::vector<std::unique_ptr<std::thread>> TextureStoreThreadPool;

class TextureStore {
  private:
	TextureStore( void );
	virtual ~TextureStore( void );

	struct CustomDeleter {
		void operator()( ci::gl::TextureBase *tex );
	};

  public:
	// singleton implementation
	static TextureStore &getInstance()
	{
		static TextureStore tm;
		return tm;
	};

	//! synchronously loads an image into a texture, stores it and returns it
	ci::gl::Texture2dRef load( const std::string &url, ci::gl::Texture2d::Format fmt = ci::gl::Texture2d::Format() );
	//! asynchronously loads an image into a texture, returns immediately
	ci::gl::Texture2dRef fetch( const std::string &url, ci::gl::Texture2d::Format fmt = ci::gl::Texture2d::Format() );
	//! remove url from the queue. Has no effect if image has already been loaded
	bool abort( const std::string &url );
	//! terminates all running threads and clears the cache. Call before terminating the application.
	void cleanup();

	//! returns a list of valid extensions for image files. ImageIO::getLoadExtensions() does not seem to work.
	std::vector<std::string> getLoadExtensions();

	//! returns TRUE if image is scheduled for loading but has not been turned into a Texture2d yet
	bool isLoading( const std::string &url );
	//! returns TRUE if image has been turned into a Texture2d
	bool isLoaded( const std::string &url );

	//! returns an empty texture. Override it to supply something else in case a texture was not available.
	virtual ci::gl::Texture2dRef empty() { return ci::gl::Texture2dRef(); };
  protected:
	volatile bool isInterrupted;

	//! list of created Textures
	std::map<std::string, std::weak_ptr<ci::gl::Texture2d>> mTextures;

	//! queue of textures to load asynchronously
	ConcurrentDeque<std::string> mQueue;
	ConcurrentDeque<std::string> mLoadingQueue;

	//!	container for the asynchronously loaded surfaces
	ConcurrentMap<std::string, ci::Surface> mSurfaces;

	//! one or more worker threads
	TextureStoreThreadPool mThreads;
	std::atomic<bool>      mShouldQuit;

  private:
	//! loads files in a separate thread and creates Surfaces. These are then passed to the main thread and turned into Textures.
	void threadLoad();

	//! Creates the actual texture and defines a custom deleter for it.
	ci::gl::Texture2dRef storeTexture( const std::string &url, const ci::gl::Texture2dRef &src );
};

// helper functions for easier access

//! synchronously loads an image into a texture, stores it and returns it
inline ci::gl::Texture2dRef loadTexture( const std::string &url, ci::gl::Texture2d::Format fmt = ci::gl::Texture2d::Format() )
{
	return TextureStore::getInstance().load( url, fmt );
};

//! asynchronously loads an image into a texture, returns immediately
inline ci::gl::Texture2dRef fetchTexture( const std::string &url, ci::gl::Texture2d::Format fmt = ci::gl::Texture2d::Format() )
{
	return TextureStore::getInstance().fetch( url, fmt );
};

} // namespace ph