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

// To make sure boost::asio is loaded first, include our header file first
#include "TcpClient.h"

#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/Font.h"
#include "cinder/Text.h"

using namespace ci;
using namespace ci::app;
using namespace std;

//
class TcpClientApp : public AppBasic {
public:
	// 
	void setup();
	void update();
	void draw();

	void resize();

	// slots that are called by the server
	void onConnected(const boost::asio::ip::tcp::endpoint&);
	void onDisconnected(const boost::asio::ip::tcp::endpoint&);
	void onMessage(const std::string&);
protected:
	//! our Boost ASIO TCP Client
	ph::TcpClientRef			mTcpClientRef;

	shared_ptr<ci::TextBox>		mTextBoxRef;
	gl::Texture					mTexture;
};

void TcpClientApp::setup()
{
	// setup the TextBox
	mTextBoxRef = shared_ptr<TextBox>( new TextBox() );
	mTextBoxRef->setColor( Color::white() );
	mTextBoxRef->setBackgroundColor( Color::black() );

	// create TcpClient and set heartbeat message to "QUIT"
	mTcpClientRef = ph::TcpClientRef( new ph::TcpClient("QUIT") );

	// listen to the TcpClient's signals
	mTcpClientRef->sConnected.connect( boost::bind( &TcpClientApp::onConnected, this, boost::arg<1>::arg() ) );
	mTcpClientRef->sDisconnected.connect( boost::bind( &TcpClientApp::onDisconnected, this, boost::arg<1>::arg() ) );
	mTcpClientRef->sMessage.connect( boost::bind( &TcpClientApp::onMessage, this, boost::arg<1>::arg() ) );

	// connect to the Lycos mail server (as a test)
	mTcpClientRef->setDelimiter("\r\n");
	mTcpClientRef->connect(Url("pop.mail.lycos.com"), "pop3");
}

void TcpClientApp::update()
{
	// call the TcpClient's update function to read and write messages
	mTcpClientRef->update();
}

void TcpClientApp::draw()
{
	if(mTexture) gl::draw(mTexture, Vec2f(10, 10));
}

void TcpClientApp::resize()
{	
	if(mTextBoxRef) mTextBoxRef->setSize( getWindowSize() - Vec2i(20, 20) );
}
	
void TcpClientApp::onConnected(const boost::asio::ip::tcp::endpoint &endpoint)
{
	console() << "Connected to:" << endpoint.address().to_string() << std::endl;
}

void TcpClientApp::onDisconnected(const boost::asio::ip::tcp::endpoint &endpoint)
{
	console() << "Disconnected from:" << endpoint.address().to_string() << std::endl;
	console() << "Trying to reconnect in 5 seconds." << std::endl;
}

void TcpClientApp::onMessage(const std::string &msg)
{
	if(mTextBoxRef) {
		mTextBoxRef->appendText(msg + "\n");
		mTexture = gl::Texture( mTextBoxRef->render() );
	}
}

CINDER_APP_BASIC( TcpClientApp, RendererGl )