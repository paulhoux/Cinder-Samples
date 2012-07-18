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

#include <queue>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

namespace ph {

template<typename Data>
class ConcurrentQueue
{
public:
	ConcurrentQueue(void){};
	~ConcurrentQueue(void){};
	
    void push(Data const& data)
    {
        boost::mutex::scoped_lock lock(mMutex);
        mQueue.push(data);
        lock.unlock();
        mCondition.notify_one();
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(mMutex);
        return mQueue.empty();
    }

    bool try_pop(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(mMutex);
        if(mQueue.empty())
        {
            return false;
        }
        
        popped_value=mQueue.front();
        mQueue.pop();
        return true;
    }

    void wait_and_pop(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(mMutex);
        while(mQueue.empty())
        {
            mCondition.wait(lock);
        }
        
        popped_value=mQueue.front();
        mQueue.pop();
    }
private:
    std::queue<Data>			mQueue;
    mutable boost::mutex		mMutex;
    boost::condition_variable	mCondition;
};

} // namespace ph

