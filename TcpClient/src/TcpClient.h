/*
 Copyright (C) 2012 Paul Houx

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

#pragma once

// TODO: use implicitly shared object

// defines the value of _WIN32_WINNT needed by boost asio (WINDOWS ONLY)
#ifdef WIN32
    #include <sdkddkver.h>
#endif

//#include <map>
#include <string>
#include <iostream>
//#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/algorithm/string.hpp>
//#include <boost/thread.hpp>
#include <boost/signals2.hpp>

#include "cinder/app/AppBasic.h"
#include "cinder/Url.h"
#include "cinder/Utilities.h"

namespace ph
{

typedef boost::shared_ptr<class TcpClient> TcpClientRef;

class TcpClient
{
public:
	TcpClient();
	TcpClient( const std::string &heartbeat );
	virtual ~TcpClient(void);

	virtual void update();
	
	virtual void write(const std::string &msg);

	virtual void connect(const std::string &ip, unsigned short port);
	virtual void connect(const ci::Url &url, const std::string &protocol);
	virtual void connect(boost::asio::ip::tcp::endpoint& endpoint);

	virtual void disconnect();

	bool isConnected(){ return mIsConnected; };
	
	std::string getDelimiter(){ return mDelimiter; };
	void setDelimiter(const std::string &delimiter){ mDelimiter = delimiter; };
public:
	// signals
	boost::signals2::signal<void(const boost::asio::ip::tcp::endpoint&)>	sConnected;
	boost::signals2::signal<void(const boost::asio::ip::tcp::endpoint&)>	sDisconnected;
	
	boost::signals2::signal<void(const std::string&)>						sMessage;
protected:
	virtual void read();
	virtual void close();

	// callbacks
	virtual void handle_connect(const boost::system::error_code& error);
	virtual void handle_read(const boost::system::error_code& error);
	virtual void handle_write(const boost::system::error_code& error);

	virtual void do_write(const std::string &msg);
	virtual void do_close();

	virtual void do_reconnect(const boost::system::error_code& error);
	virtual void do_heartbeat(const boost::system::error_code& error);
protected:
	bool							mIsConnected;
	bool							mIsClosing;

	boost::asio::ip::tcp::endpoint	mEndPoint;

	boost::asio::io_service			mIos;
	boost::asio::ip::tcp::socket	mSocket;

	boost::asio::streambuf			mBuffer;

	//! to be written to server
	std::deque<std::string>			mMessages;	

	boost::asio::deadline_timer		mHeartBeatTimer;
	boost::asio::deadline_timer		mReconnectTimer;

	std::string						mDelimiter;
	std::string						mHeartBeat;
};

} // namespace

