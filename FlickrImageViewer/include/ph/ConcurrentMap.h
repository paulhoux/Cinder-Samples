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

// use the boost thread library instead of Cinder's
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>

#include <map>

namespace ph {

template<typename Key, typename Data>
class ConcurrentMap
{
public:
	ConcurrentMap(void){};
	~ConcurrentMap(void){};

	void clear()
	{
		boost::mutex::scoped_lock lock(mMutex);
		mQueue.clear();
	}

	bool contains(Key const& key)
	{
		boost::mutex::scoped_lock lock(mMutex);

		typename std::map<Key, Data>::iterator itr = mQueue.find(key);
		return (itr != mQueue.end());
	}

	bool erase(Key const& key)
	{
		boost::mutex::scoped_lock lock(mMutex);

		size_t n = mQueue.erase(key);

		return (n > 0);
	}

    void push(Key const& key, Data const& data)
    {
        boost::mutex::scoped_lock lock(mMutex);
        mQueue[key] = data;
        lock.unlock();
        mCondition.notify_one();
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(mMutex);
        return mQueue.empty();
    }

	bool get(Key const& key, Data& popped_value)
	{
		boost::mutex::scoped_lock lock(mMutex);

		typename std::map<Key, Data>::iterator itr = mQueue.find(key);
		if (itr == mQueue.end())
			return false;
        
        popped_value = mQueue[key];

        return true;
    }

    bool try_pop(Key const& key, Data& popped_value)
    {
        boost::mutex::scoped_lock lock(mMutex);

		typename std::map<Key, Data>::iterator itr = mQueue.find(key);
		if (itr == mQueue.end())
			return false;
        
        popped_value = mQueue[key];
        mQueue.erase(key);

        return true;
    }

    void wait_and_pop(Key const& key, Data& popped_value)
    {
        boost::mutex::scoped_lock lock(mMutex);
		typename std::map<Key, Data>::iterator itr;
        while(mQueue.find(key) == mQueue.end())
        {
            mCondition.wait(lock);
        }
        
        popped_value = mQueue[key];
        mQueue.erase(key);
    }
private:
    std::map<Key, Data>			mQueue;
    mutable boost::mutex		mMutex;
    boost::condition_variable	mCondition;
};

} // namespace ph
