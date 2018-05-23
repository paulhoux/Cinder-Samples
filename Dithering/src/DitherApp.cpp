#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DitherApp : public App {
  public:
	void setup() override;
	void draw() override;

	void resize() override;
	void keyDown( KeyEvent event ) override;

	void renderGradient( const Rectf &rect ) const;
	void renderInterface() const;

  private:
	Font            mFont;                      //
	ci::Color       mColor{ 0.1f, 0.1f, 0.1f }; //
	gl::GlslProgRef mShader;                    //
	gl::FboRef      mBuffer8Bit;                //
	gl::FboRef      mBuffer10Bit;               //
	int             mBitDepth{ 5 };             //
	bool            mShaderEnabled{ true };     //
};

void DitherApp::setup()
{
	// Load font.
	mFont = Font( "Verdana", 18 );

	// Compile dithering shader.
	mShader = gl::GlslProg::create( loadAsset( "dither.vert" ), loadAsset( "dither.frag" ) );
}

void DitherApp::draw()
{
	gl::clear();

	// Render gradient to 8-bit frame buffer.
	if( mBuffer8Bit ) {
		gl::ScopedFramebuffer scpFbo( mBuffer8Bit );
		gl::ScopedViewport    scpViewport( mBuffer8Bit->getSize() );
		gl::ScopedColor       scpColor( 1, 1, 1 );
		gl::ScopedMatrices    scpMatrices;
		gl::setMatricesWindow( mBuffer8Bit->getSize() );

		renderGradient( mBuffer8Bit->getBounds() );
	}

	// Render gradient to 10-bit frame buffer.
	if( mBuffer10Bit ) {
		gl::ScopedFramebuffer scpFbo( mBuffer10Bit );
		gl::ScopedViewport    scpViewport( mBuffer10Bit->getSize() );
		gl::ScopedColor       scpColor( 1, 1, 1 );
		gl::ScopedMatrices    scpMatrices;
		gl::setMatricesWindow( mBuffer10Bit->getSize() );

		renderGradient( mBuffer10Bit->getBounds() );
	}

	// Blit results.
	if( mBuffer8Bit )
		gl::draw( mBuffer8Bit->getTexture2d( GL_COLOR_ATTACHMENT0 ), vec2( 0, 0 ) );

	if( mBuffer10Bit ) {
		if( mShader && mShaderEnabled ) {
			// Use dithering.
			gl::ScopedGlslProg scpGlsl( mShader );
			mShader->uniform( "uColorDepth", glm::pow( 2.0f, float( mBitDepth ) ) - 1.0f );
			mShader->uniform( "uSourceTex", 0 );

			gl::ScopedTextureBind scpTex0( mBuffer10Bit->getTexture2d( GL_COLOR_ATTACHMENT0 ), 0 );
			gl::drawSolidRect( Area( getWindowWidth() / 2, 0, getWindowWidth(), getWindowHeight() ) );
		}
		else {
			// Do not use dithering.
			gl::draw( mBuffer10Bit->getTexture2d( GL_COLOR_ATTACHMENT0 ), vec2( mBuffer8Bit->getWidth(), 0 ) );
		}
	}

	// Render interface.
	renderInterface();
}

void DitherApp::resize()
{
	// Create our frame buffers.
	const auto texture8Bit = gl::Texture2d::create( getWindowWidth() / 2, getWindowHeight(), gl::Texture2d::Format().internalFormat( GL_RGB8 ).dataType( GL_UNSIGNED_INT ) );
	const auto texture10Bit = gl::Texture2d::create( getWindowWidth() / 2, getWindowHeight(), gl::Texture2d::Format().internalFormat( GL_RGB10_A2 ).dataType( GL_UNSIGNED_INT_10_10_10_2 ) );
	const auto fmt8Bit = gl::Fbo::Format().attachment( GL_COLOR_ATTACHMENT0, texture8Bit );
	const auto fmt10Bit = gl::Fbo::Format().attachment( GL_COLOR_ATTACHMENT0, texture10Bit );

	try {
		mBuffer8Bit = gl::Fbo::create( getWindowWidth() / 2, getWindowHeight(), fmt8Bit );
		mBuffer10Bit = gl::Fbo::create( getWindowWidth() / 2, getWindowHeight(), fmt10Bit );
	}
	catch( const std::exception &exc ) {
		CI_LOG_E( "Failed to create frame buffers: " << exc.what() );
	}
}

void DitherApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	case KeyEvent::KEY_UP:
		mBitDepth = clamp( mBitDepth + 1, 1, 10 );
		break;
	case KeyEvent::KEY_DOWN:
		mBitDepth = clamp( mBitDepth - 1, 1, 10 );
		break;
	case KeyEvent::KEY_c:
		mColor.r = Rand::randFloat( 0.3f );
		mColor.g = Rand::randFloat( 0.3f );
		mColor.b = Rand::randFloat( 0.3f );
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_ESCAPE:
		if( isFullScreen() )
			setFullScreen( false );
		else
			quit();
		break;
	case KeyEvent::KEY_SPACE:
		mShaderEnabled = !mShaderEnabled;
		break;
	default:
		return;
	}
}

void DitherApp::renderGradient( const Rectf &rect ) const
{
	gl::begin( GL_TRIANGLE_STRIP );
	gl::color( 0, 0, 0 );
	gl::vertex( rect.getUpperLeft() );
	gl::vertex( rect.getUpperRight() );
	gl::color( mColor );
	gl::vertex( rect.getLowerLeft() );
	gl::vertex( rect.getLowerRight() );
	gl::end();
}

void DitherApp::renderInterface() const
{
	gl::ScopedColor scpColor( 1, 1, 1 );

	gl::drawLine( vec2( getWindowWidth() / 2, 0 ), vec2( getWindowWidth() / 2, 16 ) );
	gl::drawLine( vec2( getWindowWidth() / 2, getWindowHeight() - 16 ), vec2( getWindowWidth() / 2, getWindowHeight() ) );
	gl::drawLine( vec2( getWindowWidth() / 2, getWindowHeight() / 2 - 8 ), vec2( getWindowWidth() / 2, getWindowHeight() / 2 + 8 ) );
	gl::drawStringRight( "8-bit source, no dithering", vec2( getWindowWidth() / 2 - 16, 16 ), Color::white(), mFont );

	std::stringstream ss;
	ss << "10-bit source, ";
	if( mShaderEnabled )
		ss << "dithered to " << mBitDepth << "-bit depth";
	else
		ss << "no dithering";
	gl::drawString( ss.str(), vec2( getWindowWidth() / 2 + 16, 16 ), Color::white(), mFont );
}

CINDER_APP( DitherApp, RendererGl )
