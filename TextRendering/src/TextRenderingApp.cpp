#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

#include "text/FontStore.h"
#include "text/TextBox.h"

using namespace ci;
using namespace ci::app;
using namespace ph::text;
using namespace std;

class TextRenderingApp : public AppBasic {
public:
	void prepareSettings( Settings *settings );
	void setup();
	void shutdown();
	void update();
	void draw();

	void mouseDown( MouseEvent event );	
	void mouseDrag( MouseEvent event );	
	void mouseUp( MouseEvent event );
	void mouseWheel( MouseEvent event );

	void keyDown( KeyEvent event );
	void keyUp( KeyEvent event );

	void resize( ResizeEvent event );
	void fileDrop( FileDropEvent event );
protected:
	Vec3f	constrainAnchor( const Vec3f &pt ) const;
	void	updateWindowTitle();
protected:
	bool			mShowBounds;
	bool			mShowWireframe;

	Color			mFrontColor;
	Color			mBackColor;

	//! 
	TextBox			mTextBox;

	//! textbox transformation members
	Vec3f			mAnchor;
	Quatf			mOrientation;
	float			mScale;
	Matrix44f		mTransform;

	//! textbox animation members
	Vec3f			mAnchorTarget;
	Vec3f			mAnchorSpeed;
	float			mScaleSpeed;

	//! interaction members
	Vec3f			mInitialAnchor;
	Quatf			mInitialOrientation;
	float			mInitialScale;

	Vec2f			mInitialMouse;
	Vec2f			mCurrentMouse;

	uint32_t		mMouseDownFrame;
	uint32_t		mMouseDragFrame;

};

void TextRenderingApp::prepareSettings(Settings *settings)
{
	// set maximum frame rate quite high, so we can measure performance if vertical sync is disabled
	settings->setFrameRate(500.0f);

	// set initial window size
	settings->setWindowSize(1280,720);//(550, 750);
}

void TextRenderingApp::setup()
{
	try { 
		// load fonts using the FontStore
		fonts().loadFont( loadAsset("fonts/Walter Turncoat Regular.sdff") ); 

		// create a text box (rectangular text area)
		mTextBox = TextBox( 400, 500 );
		// set font and font size
		mTextBox.setFont( fonts().getFont("Walter Turncoat Regular") );
		mTextBox.setFontSize( 14.0f );
		// break lines between words
		mTextBox.setBoundary( Text::WORD );
		// adjust space between lines
		mTextBox.setLineSpace( 1.5f );

		// load a text and hand it to the text box
		mTextBox.setText( loadString( loadAsset("fonts/readme.txt") ) );
	}
	catch( const std::exception & e ) { 
		console() << e.what() << std::endl; 
	}
	
	// initialize member variables
	mShowBounds = false;
	mShowWireframe= false;

	mFrontColor = Color(0.1f, 0.1f, 0.1f);
	mBackColor = Color(0.9f, 0.9f, 0.9f);

	// set initial text position and scale
	mScale = 1.0f;
	mScaleSpeed = 1.0f;
	mAnchor.x = 0.0f;
	mAnchor.y = 0.0f;
	mAnchorTarget = mAnchor = constrainAnchor( mAnchor );
	mAnchorSpeed = Vec3f::zero();

	// update the window title
	updateWindowTitle();
}

void TextRenderingApp::shutdown()
{
	// no clean up necessary
}

void TextRenderingApp::update()
{	
	// adjust scale
	mScale *= mScaleSpeed;

	// if scrolling, gradually reduce speed and adjust target
	mAnchorSpeed *= 0.95f;
	mAnchorTarget -= mAnchorSpeed;
	mAnchorTarget = constrainAnchor( mAnchorTarget );

	// adjust anchor accordingly
	mAnchor += 0.2f * (mAnchorTarget - mAnchor);

	// calculate transformation matrix
	mTransform.setToIdentity();
	mTransform.translate( Vec3f( 0.5f * Vec2f( getWindowSize() ), 0.0f ) );	// position
	mTransform *= mOrientation.toMatrix44();		// orientation
	mTransform.scale( Vec3f(mScale, mScale, mScale) );						// scale
	mTransform.translate( -mAnchor );				// anchor
}

void TextRenderingApp::draw()
{
	// note: when drawing text to an FBO, 
	//  make sure to enable maximum quality multisampling (16x is best)!

	// clear out the window
	gl::clear( mBackColor );

	// setup render states
	gl::color( mFrontColor );
	gl::enableAlphaBlending();

	// apply transformations
	gl::pushModelView();
	gl::multModelView( mTransform );

	// draw the text
	mTextBox.draw();

	// draw mesh as wireframes if enabled
	if(mShowWireframe) 
		mTextBox.drawWireframe();
	
	// restore render states
	gl::popModelView();
	gl::disableAlphaBlending();
}

void TextRenderingApp::mouseDown( MouseEvent event )
{
	// keep track of where we clicked the mouse
	mCurrentMouse = mInitialMouse = event.getPos();

	// keep track of current transformations
	mInitialAnchor = mAnchor;
	mInitialOrientation = mOrientation;
	mInitialScale = mScale;

	// make sure auto-scrolling is disabled
	mAnchorTarget = mAnchor;	
	mAnchorSpeed = Vec3f::zero();
	mScaleSpeed = 1.0f;

	// keep track of current frame, so we can calculate average drag speed in mouseUp()
	mMouseDownFrame = getElapsedFrames();
}

void TextRenderingApp::mouseDrag( MouseEvent event )
{
	mCurrentMouse = event.getPos();

	if( event.isLeftDown() )
	{
		// drag text up and down, limited by the window dimensions
		Vec2f d( (mCurrentMouse.x - mInitialMouse.x) / mScale, (mCurrentMouse.y - mInitialMouse.y) / mScale );
		mAnchor.x = mInitialAnchor.x - d.x;
		mAnchor.y = mInitialAnchor.y - d.y;
		mAnchorTarget = mAnchor = constrainAnchor( mAnchor );

		// if anchor was limited, adjust initial anchor for better mouse response
		mInitialAnchor.x = mAnchor.x + d.x;
		mInitialAnchor.y = mAnchor.y + d.y;
	
		// keep track of current frame, so we can calculate average drag speed in mouseUp()
		mMouseDragFrame = getElapsedFrames();
	}
	else if( event.isRightDown() )
	{
		// scale text
		float d0 = mInitialMouse.length();
		float d1 = mCurrentMouse.length();
		mScale = mInitialScale * (d1 / d0);

		// make sure auto-scrolling is disabled
		mAnchorTarget = mAnchor = constrainAnchor( mAnchorTarget );
		mAnchorSpeed = Vec3f::zero();
	}
}

void TextRenderingApp::mouseUp( MouseEvent event )
{
	// don't calculate average speed if not dragged since more than 5 frames, or not dragged at all
	if( (getElapsedFrames() - mMouseDragFrame) > 5 || mMouseDragFrame <= mMouseDownFrame )
		mAnchorSpeed = Vec3f::zero();
	else
	{
		mCurrentMouse = event.getPos();

		// calculate average drag distance per frame
		Vec2f d( (mCurrentMouse.x - mInitialMouse.x) / mScale, (mCurrentMouse.y - mInitialMouse.y) / mScale );
		Vec2f avg = d / float( mMouseDragFrame - mMouseDownFrame );

		mAnchorSpeed = Vec3f( avg, 0.0f );
	}
}

void TextRenderingApp::mouseWheel( MouseEvent event )
{		
	// use the scroll wheel to scroll the text, two lines per event
	mAnchorTarget.y -= event.getWheelIncrement() * mTextBox.getLeading() * 2.0f;
	mAnchorSpeed = Vec3f::zero();
}

void TextRenderingApp::keyDown( KeyEvent event )
{
	switch(event.getCode()) {
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_d:
		// load a very long text and hand it to the text box
		mTextBox.setText( loadString( loadAsset("text/345.txt") ) );
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_v:
		gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
		break;
	case KeyEvent::KEY_w:
		mShowWireframe = !mShowWireframe;
		break;
	case KeyEvent::KEY_RETURN:
		mScale = 1.0f;
		break;
	case KeyEvent::KEY_SPACE:
		{ Color temp = mFrontColor; mFrontColor = mBackColor; mBackColor = temp; }
		break;
	case KeyEvent::KEY_LEFTBRACKET:
		if(mTextBox.getFontSize() > 0.5f)
			mTextBox.setFontSize( mTextBox.getFontSize() - 0.5f );
		break;
	case KeyEvent::KEY_RIGHTBRACKET:
		mTextBox.setFontSize( mTextBox.getFontSize() + 0.5f );
		break;
	case KeyEvent::KEY_LEFT:
		/*if( mTextBox.getAlignment() == TextBox::JUSTIFIED )
			mTextBox.setAlignment( TextBox::RIGHT );
		else*/ if( mTextBox.getAlignment() == TextBox::RIGHT )
			mTextBox.setAlignment( TextBox::CENTER );
		else if( mTextBox.getAlignment() == TextBox::CENTER )
			mTextBox.setAlignment( TextBox::LEFT );
		break;
	case KeyEvent::KEY_RIGHT:
		if( mTextBox.getAlignment() == TextBox::LEFT )
			mTextBox.setAlignment( TextBox::CENTER );
		else if( mTextBox.getAlignment() == TextBox::CENTER )
			mTextBox.setAlignment( TextBox::RIGHT );
		/*else if( mTextBox.getAlignment() == TextBox::RIGHT )
			mTextBox.setAlignment( TextBox::JUSTIFIED );*/
		break;
	case KeyEvent::KEY_UP:
		if( mTextBox.getLineSpace() > 0.2f )
			mTextBox.setLineSpace( mTextBox.getLineSpace() - 0.1f );
		break;
	case KeyEvent::KEY_DOWN:
		if( mTextBox.getLineSpace() < 5.0f )
			mTextBox.setLineSpace( mTextBox.getLineSpace() + 0.1f );
		break;
	case KeyEvent::KEY_PAGEDOWN:
		// TODO: don't scroll if mouse down
		mAnchorTarget.y += (getWindowHeight() / mScale) - mTextBox.getLeading();
		mAnchorSpeed = Vec3f::zero();
		break;
	case KeyEvent::KEY_PAGEUP:
		// TODO: don't scroll if mouse down
		mAnchorTarget.y -= (getWindowHeight() / mScale) - mTextBox.getLeading();
		mAnchorSpeed = Vec3f::zero();
		break;
	case KeyEvent::KEY_HOME:
		mAnchorTarget.x = 0.0f;
		mAnchorTarget.y = 0.0f;
		mAnchorSpeed = Vec3f::zero();
		break;
	case KeyEvent::KEY_END:
		mAnchorTarget.x = mTextBox.getBounds().getWidth();
		mAnchorTarget.y = mTextBox.getBounds().getHeight();
		mAnchorSpeed = Vec3f::zero();
		break;
	case KeyEvent::KEY_KP_MINUS:
		mScaleSpeed = 0.995f;
		break;
	case KeyEvent::KEY_KP_PLUS:
		mScaleSpeed = 1.005f;
		break;
	}

	//
	updateWindowTitle();
}

void TextRenderingApp::keyUp( KeyEvent event )
{	
	switch(event.getCode()) {
	case KeyEvent::KEY_KP_MINUS:
	case KeyEvent::KEY_KP_PLUS:
		mScaleSpeed = 1.0f;
		break;
	}
}

void TextRenderingApp::resize( ResizeEvent event )
{
	// resize text box with a margin of 20 pixels on each side
	mTextBox.setSize( event.getWidth() - 40.0f, 0.0f );
}

void TextRenderingApp::fileDrop( FileDropEvent event )
{
	if( event.getNumFiles() == 1) {
		// you dropped 1 file, let's try to load it as a SDFF font file
		fs::path file = event.getFile(0);

		if(file.extension() == ".sdff") {
			try {
				// create a new font and read the file
				ph::text::FontRef font( new ph::text::Font() );
				font->read( loadFile(file) );
				// add font to font manager
				fonts().addFont( font );
				// set the text font
				mTextBox.setFont( font );
			}
			catch( const std::exception &e ) {
				console() << e.what() << std::endl;
			}
		}
		else {
			// try to render the file as a text
			mTextBox.setText( loadString( loadFile(file) ) );
		}
	}
	else if( event.getNumFiles() == 2 ) {
		// you dropped 2 files, let's try to create a new SDFF font file
		fs::path fileA = event.getFile(0);
		fs::path fileB = event.getFile(1);

		if(fileA.extension() == ".txt" && fileB.extension() == ".png") {
			try {
				// create a new font from an image and a metrics file
				ph::text::FontRef font( new ph::text::Font() );
				font->create( loadFile(fileB), loadFile(fileA) );
				// create a compact SDFF file
				font->write( writeFile( fileB.parent_path() / (font->getFamily() + ".sdff") ) );
				// add font to font manager
				fonts().addFont( font );
				// set the text font
				mTextBox.setFont( font );
			}
			catch( const std::exception &e ) {
				console() << e.what() << std::endl;
			}
		}
		else if(fileB.extension() == ".txt" && fileA.extension() == ".png") {
			try {
				// create a new font from an image and a metrics file
				ph::text::FontRef font( new ph::text::Font() );
				font->create( loadFile(fileA), loadFile(fileB) );
				// create a compact SDFF file
				font->write( writeFile( fileA.parent_path() / (font->getFamily() + ".sdff") ) );
				// add font to font manager
				fonts().addFont( font );
				// set the text font
				mTextBox.setFont( font );
			}
			catch( const std::exception &e ) {
				console() << e.what() << std::endl;
			}
		}
	}

	updateWindowTitle();
}

Vec3f TextRenderingApp::constrainAnchor( const Vec3f &pt ) const
{
	Vec2f	margin = Vec2f(20, 20) / mScale;
	Vec2f	size = mTextBox.getBounds().getSize();
	Vec2f	center = 0.5f / mScale * Vec2f(getWindowSize());

	float x = math<float>::min( center.x - margin.x, 0.5f * size.x );
	float y = math<float>::min( center.y - margin.y, 0.5f * size.y );

	float minx = math<float>::min( size.x - x, x );
	float maxx = math<float>::max( size.x - x, x );
	float miny = math<float>::min( size.y - y, y );
	float maxy = math<float>::max( size.y - y, y );

	Vec3f result;
	result.x = math<float>::clamp( pt.x, minx, maxx );
	result.y = math<float>::clamp( pt.y, miny, maxy );
	result.z = pt.z;
	
	return result;
}

void TextRenderingApp::updateWindowTitle()
{
#ifdef WIN32
	std::wstringstream str;
	str << "TextRenderingApp -";
	str << " Font family: " << toUtf16( mTextBox.getFontFamily() );
	str << " (" << mTextBox.getFontSize() << ")";

	HWND hWnd = getRenderer()->getHwnd();
	::SetWindowText( hWnd, str.str().c_str() );
#endif
}

CINDER_APP_BASIC( TextRenderingApp, RendererGl )
