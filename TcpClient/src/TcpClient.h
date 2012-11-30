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

// TODO: use implicitly shared object

// defines the value of _WIN32_WINNT needed by boost asio (WINDOWS ONLY)
#ifdef WIN32
    #include <sdkddkver.h>
#endif

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
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

