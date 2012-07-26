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
#include <deque>

namespace ph {

template<typename Data>
class ConcurrentDeque
{
public:
	ConcurrentDeque(void){};
	~ConcurrentDeque(void){};

	void clear()
	{
		std::mutex::scoped_lock lock(mMutex);
		mDeque.clear();
	}

	bool contains(Data const& data)
	{
		std::mutex::scoped_lock lock(mMutex);

		typename std::deque<Data>::iterator itr = std::find(mDeque.begin(), mDeque.end(), data);
		return (itr != mDeque.end());
	}

	bool erase(Data const& data)
	{
		std::mutex::scoped_lock lock(mMutex);

		typename std::deque<Data>::iterator itr = std::find(mDeque.begin(), mDeque.end(), data);
		if(itr != mDeque.end()) {
			mDeque.erase(itr);
			return true;
		}

		return false;
	}

	bool erase_all(Data const& data)
	{
		std::mutex::scoped_lock lock(mMutex);

		typename std::deque<Data>::iterator itr;
		do {
			itr = std::find(mDeque.begin(), mDeque.end(), data);
			if(itr != mDeque.end()) mDeque.erase(itr);
		} while(itr != mDeque.end());

		return true;
	}
	
    bool push_back(Data const& data, bool unique=false)
    {
        std::mutex::scoped_lock lock(mMutex);

		if(unique) {
			typename std::deque<Data>::iterator itr = std::find(mDeque.begin(), mDeque.end(), data);
			if(itr == mDeque.end()) {
				mDeque.push_back(data);
				lock.unlock();
				mCondition.notify_one();

				return true;
			}
		}
		else {
			mDeque.push_back(data);
			lock.unlock();
			mCondition.notify_one();

			return true;
		}

		return false;
    }

    bool empty() const
    {
        std::mutex::scoped_lock lock(mMutex);
        return mDeque.empty();
    }

    bool pop_front(Data& popped_value)
    {
        std::mutex::scoped_lock lock(mMutex);
        if(mDeque.empty())
        {
            return false;
        }
        
        popped_value=mDeque.front();
        mDeque.pop_front();
        return true;
    }

    void wait_and_pop_front(Data& popped_value)
    {
        std::mutex::scoped_lock lock(mMutex);
        while(mDeque.empty())
        {
            mCondition.wait(lock);
        }
        
        popped_value=mDeque.front();
        mDeque.pop_front();
    }
private:
    std::deque<Data>			mDeque;
    mutable std::mutex		mMutex;
    std::condition_variable	mCondition;
};

} // namespace ph
