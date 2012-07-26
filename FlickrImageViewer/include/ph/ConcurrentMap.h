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

Based on the excellent article by Anthony Williams:
http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
*/

#pragma once

#include "cinder/Thread.h"
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
		std::mutex::scoped_lock lock(mMutex);
		mQueue.clear();
	}

	bool contains(Key const& key)
	{
		std::mutex::scoped_lock lock(mMutex);

		std::map<Key, Data>::iterator itr = mQueue.find(key);
		return (itr != mQueue.end());
	}

	bool erase(Key const& key)
	{
		std::mutex::scoped_lock lock(mMutex);

		size_t n = mQueue.erase(key);

		return (n > 0);
	}

    void push(Key const& key, Data const& data)
    {
        std::mutex::scoped_lock lock(mMutex);
        mQueue[key] = data;
        lock.unlock();
        mCondition.notify_one();
    }

    bool empty() const
    {
        std::mutex::scoped_lock lock(mMutex);
        return mQueue.empty();
    }

	bool get(Key const& key, Data& popped_value)
	{
		std::mutex::scoped_lock lock(mMutex);

		typename std::map<Key, Data>::iterator itr = mQueue.find(key);
		if (itr == mQueue.end())
			return false;
        
        popped_value = mQueue[key];

        return true;
    }

    bool try_pop(Key const& key, Data& popped_value)
    {
        std::mutex::scoped_lock lock(mMutex);

		typename std::map<Key, Data>::iterator itr = mQueue.find(key);
		if (itr == mQueue.end())
			return false;
        
        popped_value = mQueue[key];
        mQueue.erase(key);

        return true;
    }

    void wait_and_pop(Key const& key, Data& popped_value)
    {
        std::mutex::scoped_lock lock(mMutex);
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
    mutable std::mutex		mMutex;
    std::condition_variable	mCondition;
};

} // namespace ph