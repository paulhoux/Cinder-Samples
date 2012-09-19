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

#include "TcpClient.h"

using namespace ci;
using namespace ci::app;

using namespace ph;

TcpClient::TcpClient()
	: mIsConnected(false), mIsClosing(false),
	mSocket(mIos), mHeartBeatTimer(mIos), mReconnectTimer(mIos), mHeartBeat("PING")
{	
	// for servers that terminate their messages with a null-byte
	mDelimiter = "\0"; 
}

TcpClient::TcpClient( const std::string &heartbeat )
	: mIsConnected(false), mIsClosing(false),
	mSocket(mIos), mHeartBeatTimer(mIos), mReconnectTimer(mIos), mHeartBeat(heartbeat)
{	
	// for servers that terminate their messages with a null-byte
	mDelimiter = "\0"; 
}

TcpClient::~TcpClient(void)
{
	disconnect();
}

void TcpClient::update()
{
	// calls the poll() function to process network messages
	mIos.poll();
}

void TcpClient::connect(const std::string &ip, unsigned short port)
{
	// connect socket
	try {
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);

		connect(endpoint);
	}
	catch(const std::exception &e) {
		ci::app::console() << "Server exception:" << e.what() << std::endl;
	}
}

void TcpClient::connect(const Url &url, const std::string &protocol)
{
	// On Windows, see './Windows/system32/drivers/etc/services' for a list of supported protocols.
	//  You can also explicitly pass a port, like "8080"
	boost::asio::ip::tcp::resolver::query query( url.str(), protocol );
	boost::asio::ip::tcp::resolver resolver( mIos );

	try {
		boost::asio::ip::tcp::resolver::iterator destination = resolver.resolve(query);

		boost::asio::ip::tcp::endpoint endpoint;
		while ( destination != boost::asio::ip::tcp::resolver::iterator() ) 
			endpoint = *destination++;

		connect(endpoint);
	}
	catch(const std::exception &e) {
		ci::app::console() << "Server exception:" << e.what() << std::endl;
	}
}

void TcpClient::connect(boost::asio::ip::tcp::endpoint& endpoint)
{
	if(mIsConnected) return;
	if(mIsClosing) return;

	mEndPoint = endpoint;

	ci::app::console() << "Trying to connect to port " << endpoint.port() << " @ " << endpoint.address().to_string() << std::endl;

	// try to connect, then call handle_connect
	mSocket.async_connect(endpoint,
        boost::bind(&TcpClient::handle_connect, this, boost::asio::placeholders::error));
}

void TcpClient::disconnect()
{		
	// tell socket to close the connection
	close();
	
	// tell the IO service to stop
	mIos.stop();

	mIsConnected = false;
	mIsClosing = false;
}

void TcpClient::write(const std::string &msg)
{
	if(!mIsConnected) return;
	if(mIsClosing) return;

	// safe way to request the client to write a message
	mIos.post(boost::bind(&TcpClient::do_write, this, msg));
}

void TcpClient::close()
{
	if(!mIsConnected) return;

	// safe way to request the client to close the connection
	mIos.post(boost::bind(&TcpClient::do_close, this));
}

void TcpClient::read()
{
	if(!mIsConnected) return;
	if(mIsClosing) return;

	// wait for a message to arrive, then call handle_read
	boost::asio::async_read_until(mSocket, mBuffer, mDelimiter,
          boost::bind(&TcpClient::handle_read, this, boost::asio::placeholders::error));
}

// callbacks

void TcpClient::handle_connect(const boost::system::error_code& error) 
{
	if(mIsClosing) return;
	
	if (!error) {
		// we are connected!
		mIsConnected = true;

		// let listeners know
		sConnected(mEndPoint);

		// start heartbeat timer (optional)	
		mHeartBeatTimer.expires_from_now(boost::posix_time::seconds(5));
		mHeartBeatTimer.async_wait(boost::bind(&TcpClient::do_heartbeat, this, boost::asio::placeholders::error));

		// await the first message
		read();
	}
	else {
		// there was an error :(
		mIsConnected = false;

		//
		ci::app::console() << "Server error:" << error.message() << std::endl;

		// schedule a timer to reconnect after 5 seconds		
		mReconnectTimer.expires_from_now(boost::posix_time::seconds(5));
		mReconnectTimer.async_wait(boost::bind(&TcpClient::do_reconnect, this, boost::asio::placeholders::error));
	}
}

void TcpClient::handle_read(const boost::system::error_code& error)
{
	if (!error)
	{
		std::string msg;
		std::istream is(&mBuffer);
		std::getline(is, msg); 
		
		if(msg.empty()) return;

		ci::app::console() << "Server message:" << msg << std::endl;

		// TODO: you could do some message processing here, like breaking it up
		//       into smaller parts, rejecting unknown messages or handling the message protocol

		// create signal to notify listeners
		sMessage(msg);

		// restart heartbeat timer (optional)	
		mHeartBeatTimer.expires_from_now(boost::posix_time::seconds(5));
		mHeartBeatTimer.async_wait(boost::bind(&TcpClient::do_heartbeat, this, boost::asio::placeholders::error));

		// wait for the next message
		read();
	}
	else
	{
		// try to reconnect if external host disconnects
		if(error.value() != 0) {
			mIsConnected = false;

			// let listeners know
			sDisconnected(mEndPoint); 
			
			// schedule a timer to reconnect after 5 seconds
			mReconnectTimer.expires_from_now(boost::posix_time::seconds(5));
			mReconnectTimer.async_wait(boost::bind(&TcpClient::do_reconnect, this, boost::asio::placeholders::error));
		}
		else
			do_close();
	}
}

void TcpClient::handle_write(const boost::system::error_code& error)
{
	if(!error && !mIsClosing)
	{
		// write next message
		mMessages.pop_front();
		if (!mMessages.empty())
		{
			ci::app::console() << "Client message:" << mMessages.front() << std::endl;

			boost::asio::async_write(mSocket,
				boost::asio::buffer(mMessages.front()),
				boost::bind(&TcpClient::handle_write, this, boost::asio::placeholders::error));
		}
		else {
			// restart heartbeat timer (optional)	
			mHeartBeatTimer.expires_from_now(boost::posix_time::seconds(5));
			mHeartBeatTimer.async_wait(boost::bind(&TcpClient::do_heartbeat, this, boost::asio::placeholders::error));
		}
	}
}

void TcpClient::do_write(const std::string &msg)
{
	if(!mIsConnected) return;

	bool write_in_progress = !mMessages.empty();
	mMessages.push_back(msg + mDelimiter);

	if (!write_in_progress && !mIsClosing)
	{
		ci::app::console() << "Client message:" << mMessages.front() << std::endl;

		boost::asio::async_write(mSocket,
			boost::asio::buffer(mMessages.front()),
			boost::bind(&TcpClient::handle_write, this, boost::asio::placeholders::error));
	}
}

void TcpClient::do_close()
{
	if(mIsClosing) return;
	
	mIsClosing = true;

	mSocket.close();
}

void TcpClient::do_reconnect(const boost::system::error_code& error)
{
	if(mIsConnected) return;
	if(mIsClosing) return;

	// close current socket if necessary
	mSocket.close();

	// try to reconnect, then call handle_connect
	mSocket.async_connect(mEndPoint,
        boost::bind(&TcpClient::handle_connect, this, boost::asio::placeholders::error));
}

void TcpClient::do_heartbeat(const boost::system::error_code& error)
{
	// here you can regularly send a message to the server to keep the connection alive,
	// I usualy send a PING and then the server replies with a PONG

	// for now, send a QUIT to disconnect so we can see how this class can auto-reconnect
	if(!error) write( mHeartBeat );
}

