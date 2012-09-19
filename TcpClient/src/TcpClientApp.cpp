#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/Font.h"
#include "cinder/Text.h"
#include "TcpClient.h"

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

	void resize( ResizeEvent event );

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

void TcpClientApp::resize( ResizeEvent event )
{	
	if(mTextBoxRef) mTextBoxRef->setSize( event.getSize() - Vec2i(20, 20) );
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