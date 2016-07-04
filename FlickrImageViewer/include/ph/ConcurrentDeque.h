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

 Based on the excellent article by Anthony Williams:
 http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
*/

#pragma once

#include <deque>

namespace ph {

template <typename Data>
class ConcurrentDeque {
  public:
	ConcurrentDeque( void )
	    : mInvalidated( false ){};
	~ConcurrentDeque( void ){};

	void clear()
	{
		std::lock_guard<std::mutex> lock( mMutex );
		mDeque.clear();
	}

	bool contains( Data const &data )
	{
		std::lock_guard<std::mutex> lock( mMutex );

		typename std::deque<Data>::iterator itr = std::find( mDeque.begin(), mDeque.end(), data );
		return ( itr != mDeque.end() );
	}

	bool erase( Data const &data )
	{
		std::lock_guard<std::mutex> lock( mMutex );

		typename std::deque<Data>::iterator itr = std::find( mDeque.begin(), mDeque.end(), data );
		if( itr != mDeque.end() ) {
			mDeque.erase( itr );
			return true;
		}

		return false;
	}

	bool erase_all( Data const &data )
	{
		std::lock_guard<std::mutex> lock( mMutex );

		typename std::deque<Data>::iterator itr;
		do {
			itr = std::find( mDeque.begin(), mDeque.end(), data );
			if( itr != mDeque.end() )
				mDeque.erase( itr );
		} while( itr != mDeque.end() );

		return true;
	}

	bool push_back( Data const &data, bool unique = false )
	{
		std::unique_lock<std::mutex> lock( mMutex );

		if( unique ) {
			typename std::deque<Data>::iterator itr = std::find( mDeque.begin(), mDeque.end(), data );
			if( itr == mDeque.end() ) {
				mDeque.push_back( data );
				lock.unlock();
				mCondition.notify_one();

				return true;
			}
		}
		else {
			mDeque.push_back( data );
			lock.unlock();
			mCondition.notify_one();

			return true;
		}

		return false;
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> lock( mMutex );
		return mDeque.empty();
	}

	bool pop_front( Data &popped_value )
	{
		std::lock_guard<std::mutex> lock( mMutex );
		if( mDeque.empty() || mInvalidated ) {
			return false;
		}

		popped_value = mDeque.front();
		mDeque.pop_front();
		return true;
	}

	bool wait_and_pop_front( Data &popped_value )
	{
		std::unique_lock<std::mutex> lock( mMutex );
		while( mDeque.empty() && !mInvalidated ) {
			mCondition.wait( lock );
		}
		if( mInvalidated )
			return false;

		popped_value = mDeque.front();
		mDeque.pop_front();

		return true;
	}

	void invalidate()
	{
		mInvalidated = true;
		mCondition.notify_all();
	}

  private:
	std::deque<Data>        mDeque;
	mutable std::mutex      mMutex;
	std::condition_variable mCondition;
	std::atomic<bool>       mInvalidated;
};

} // namespace ph