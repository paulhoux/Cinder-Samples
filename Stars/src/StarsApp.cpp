#include "cinder/Font.h"
#include "cinder/ImageIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Surface.h"
#include "cinder/Utilities.h"
#include "cinder/Timer.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>

#include "Background.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class StarsApp : public AppBasic {
public:
	void prepareSettings(Settings *settings);
	void setup();
	void update();
	void draw();

	void mouseDown( MouseEvent event );	
	void mouseDrag( MouseEvent event );
	void keyDown( KeyEvent event );
	void resize( ResizeEvent event );
protected:
	void	loadStars();
	void	createGrid();

	void	forceHideCursor();
	void	forceShowCursor();

	void	enablePointSprites();
	void	disablePointSprites();

	Color	toColor(uint32_t hex);
	ColorA	toColorA(uint32_t hex);
	int		toInt(const std::string &str);
	double	toDouble(const std::string &str);
protected:
	Vec3f			mCameraEyePoint;

	CameraPersp		mCam;
	MayaCamUI		mMayaCam;

	gl::GlslProg	mStarShader;
	gl::Texture		mStarTexture;
	gl::VboMesh		mStarMesh;

	gl::VboMesh		mGridMesh;
	
	Background		mBackground;

	Font			mFont;
	std::string		mTextFormat;

	Timer			mTimer;

	bool			mEnableAnimation;
	bool			mEnableGrid;

	bool			mIsCursorVisible;

	double			mStartTime;

};

void StarsApp::prepareSettings(Settings *settings)
{
	settings->setFrameRate(100.0f);
	settings->setFullScreen(true);
}

void StarsApp::setup()
{
	// create the spherical grid mesh
	createGrid();

	// load the star database and create the VBO mesh
	loadStars();

	// create font
	mFont = Font( loadAsset("fonts/sdf.ttf"), 20.0f );
	mTextFormat = std::string("%.0f lightyears from the Sun");

	// initialize camera
	mCameraEyePoint = Vec3f(0, 0, 0);

	mCam.setFov(60.0f);
	mCam.setNearClip( 0.01f );
	mCam.setFarClip( 5000.0f );
	mCam.setEyePoint( mCameraEyePoint );
	mCam.setCenterOfInterestPoint( Vec3f(0, 0, 0) );
	
	mMayaCam.setCurrentCam( mCam );

	// 
	mBackground.setup();
	
	// load shader and point sprite texture
	try { mStarShader = gl::GlslProg( loadAsset("stars_vert.glsl"), loadAsset("stars_frag.glsl") ); }
	catch( const std::exception &e ) { console() << e.what() << std::endl; quit(); }	

	try { mStarTexture = gl::Texture( loadImage( loadAsset("particle.png") ) ); }
	catch( const std::exception &e ) { console() << e.what() << std::endl; quit(); }

	//
	mEnableAnimation = true;
	mEnableGrid = false;

	//
	forceHideCursor();

	//
	mTimer.start();

	mStartTime = getElapsedSeconds();
}

void StarsApp::update()
{	
	// a few constants
	static const float sqrt2pi = math<float>::sqrt(2.0f * (float) M_PI);

	// animate camera
	if(mEnableAnimation) {
		// calculate time 
		float t = (float) mTimer.getSeconds();

		// determine distance to the sun (in parsecs)
		float time = t * 0.005f;
		float t_frac = (time) - math<float>::floor(time);
		float n = sqrt2pi * t_frac;
		float f = cosf( n * n );
		float distance = 500.0f - 499.95f * f;

		// determine where to look
		float a = t * 0.029f;
		float b = t * 0.026f;
		float x = -cosf(a) * (0.5f + 0.499f * cosf(b));
		float y = 0.5f + 0.499f * sinf(b);
		float z = sinf(a) * (0.5f + 0.499f * cosf(b));

		mCameraEyePoint = distance * Vec3f(x, y, z).normalized();
	}

	mCam.setEyePoint( mCameraEyePoint );
	mCam.setCenterOfInterestPoint( Vec3f::zero() );
	mMayaCam.setCurrentCam( mCam );

	//
	mBackground.setCameraDistance( mCameraEyePoint.length() );
}

void StarsApp::draw()
{		
	glLineWidth( 2.0f );

	gl::clear( Color::black() ); 
	
	gl::pushMatrices();
	gl::setMatrices( mMayaCam.getCamera() );
	{
		gl::enableDepthRead();
		gl::enableDepthWrite();

		// draw grid
		if(mEnableGrid && mGridMesh) {
			gl::color( Color::black() );
			gl::draw( mGridMesh );
		}

		// draw background
		mBackground.draw();

		gl::disableDepthWrite();
		gl::disableDepthRead();

		// draw stars
		gl::enableAdditiveBlending();		
		enablePointSprites();

		gl::color( Color::white() );
		if(mStarMesh) gl::draw( mStarMesh );

		disablePointSprites();
		gl::disableAlphaBlending();
	}
	gl::popMatrices();

	// draw text
	gl::enableAlphaBlending();
	gl::color( Color::white() );

	Vec2f position = Vec2f(0.5f, 0.95f) * Vec2f(getWindowSize());
	gl::drawLine( position + Vec2f(-400, -10), position + Vec2f(400, -10) );
	gl::drawStringCentered( 
		// convert parsecs to lightyears and create string
		(boost::format(mTextFormat) % (mCameraEyePoint.length() * 3.261631f)).str(), 
		position, Color::white(), mFont );

	// fade in at start of application
	double t = math<double>::clamp( (getElapsedSeconds() - mStartTime) / 5.0, 0.0, 1.0 );
	float a = ci::lerp<float>(1.0f, 0.0f, (float) t);

	if( a > 0.0f ) {
		gl::color( ColorA(0,0,0,a) );
		gl::drawSolidRect( getWindowBounds() );
	}

	gl::disableAlphaBlending();
}

void StarsApp::mouseDown( MouseEvent event )
{
	// allow user to control camera
	mEnableAnimation = false;
	mEnableGrid = true;

	mMayaCam.mouseDown( event.getPos() );
	mCameraEyePoint = mMayaCam.getCamera().getEyePoint();
}

void StarsApp::mouseDrag( MouseEvent event )
{
	// allow user to control camera
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	mCameraEyePoint = mMayaCam.getCamera().getEyePoint();
}

void StarsApp::keyDown( KeyEvent event )
{
	switch( event.getCode() )
	{
	case KeyEvent::KEY_f:
		// toggle full screen
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_ESCAPE:
		// quit the application
		quit();
		break;
	case KeyEvent::KEY_SPACE:
		// enable animation
		mEnableAnimation = true;
		mEnableGrid = false;
		break;
	case KeyEvent::KEY_g:
		// toggle grid
		mEnableGrid = !mEnableGrid;
		break;
	case KeyEvent::KEY_c:
		// toggle cursor
		if(mIsCursorVisible) 
			forceHideCursor();
		else 
			forceShowCursor();
		break;
	}
}

void StarsApp::resize( ResizeEvent event )
{
	// update camera to the new aspect ratio
	mCam.setAspectRatio( event.getAspectRatio() );
	mMayaCam.setCurrentCam( mCam );
}

void StarsApp::loadStars()
{
	static const double	gamma = 2.2;
	static const double	pogson = pow(100.0, 0.2);
	static const double	brightness_amplifier = 15.0; 
	static const double	brightness_threshold = 1.0 / 255.0;	

	console() << "Loading star database, please wait..." << std::endl;

	// create color look up table
	//  see: http://www.vendian.org/mncharity/dir3/starcolor/details.html
	std::vector<ColorA> lookup(49);
	lookup[ 0] = toColorA(0xff9bb2ff);
	lookup[ 1] = toColorA(0xff9eb5ff);
	lookup[ 2] = toColorA(0xffa3b9ff);
	lookup[ 3] = toColorA(0xffaabfff);
	lookup[ 4] = toColorA(0xffb2c5ff);
	lookup[ 5] = toColorA(0xffbbccff);
	lookup[ 6] = toColorA(0xffc4d2ff);
	lookup[ 7] = toColorA(0xffccd8ff);
	lookup[ 8] = toColorA(0xffd3ddff);
	lookup[ 9] = toColorA(0xffdae2ff);
	lookup[10] = toColorA(0xffdfe5ff);
	lookup[11] = toColorA(0xffe4e9ff);
	lookup[12] = toColorA(0xffe9ecff);
	
	lookup[13] = toColorA(0xffeeefff);
	lookup[14] = toColorA(0xfff3f2ff);
	lookup[15] = toColorA(0xfff8f6ff);
	lookup[16] = toColorA(0xfffef9ff);
	lookup[17] = toColorA(0xfffff9fb);
	lookup[18] = toColorA(0xfffff7f5);
	lookup[19] = toColorA(0xfffff5ef);
	lookup[20] = toColorA(0xfffff3ea);
	lookup[21] = toColorA(0xfffff1e5);
	lookup[22] = toColorA(0xffffefe0);
	lookup[23] = toColorA(0xffffeddb);
	lookup[24] = toColorA(0xffffebd6);
	lookup[25] = toColorA(0xffffe9d2);
	
	lookup[26] = toColorA(0xffffe8ce);
	lookup[27] = toColorA(0xffffe6ca);
	lookup[28] = toColorA(0xffffe5c6);
	lookup[29] = toColorA(0xffffe3c3);
	lookup[30] = toColorA(0xffffe2bf);
	lookup[31] = toColorA(0xffffe0bb);
	lookup[32] = toColorA(0xffffdfb8);
	lookup[33] = toColorA(0xffffddb4);
	lookup[34] = toColorA(0xffffdbb0);
	lookup[35] = toColorA(0xffffdaad);
	lookup[36] = toColorA(0xffffd8a9);
	lookup[37] = toColorA(0xffffd6a5);
	lookup[38] = toColorA(0xffffd5a1);
	
	lookup[39] = toColorA(0xffffd29c);
	lookup[40] = toColorA(0xffffd096);
	lookup[41] = toColorA(0xffffcc8f);
	lookup[42] = toColorA(0xffffc885);
	lookup[43] = toColorA(0xffffc178);
	lookup[44] = toColorA(0xffffb765);
	lookup[45] = toColorA(0xffffa94b);
	lookup[46] = toColorA(0xffff9523);
	lookup[47] = toColorA(0xffff7b00);
	lookup[48] = toColorA(0xffff5200);

	// create empty buffers for the data
	std::vector< Vec3f > vertices;
	std::vector< Vec2f > texcoords;
	std::vector< Color > colors;

	// load the star database
	std::string	stars = loadString( loadAsset("hygxyz.csv") );

	// use boost tokenizer to parse the file
	std::vector<std::string> tokens;
	boost::split_iterator<std::string::iterator> lineItr, endItr;
	for (lineItr=boost::make_split_iterator(stars, boost::token_finder(boost::is_any_of("\n\r")));lineItr!=endItr;++lineItr) {
		// retrieve a single, trimmed line
		std::string line = boost::algorithm::trim_copy( boost::copy_range<std::string>(*lineItr) );
		if(line.empty()) continue;

		// split into tokens   
		boost::algorithm::split( tokens, line, boost::is_any_of(";"), boost::token_compress_off );

		// skip if data was incomplete
		if(tokens.size() < 23)  continue;

		// 
		try {
			// absolute magnitude of the star
			double abs_mag = toDouble(tokens[14]);
			// apparent magnitude of the star
			double mag = toDouble(tokens[13]);

			//double brightness = pow(pogson, -mag) * brightness_amplifier;
			//brightness = pow(brightness, 1.0 / gamma); // gamma correction
			//if(brightness < brightness_threshold) continue;

			// color (spectrum) of the star
			double colorindex = (tokens.size() > 6) ? toDouble(tokens[16]) : 0.0;
			uint32_t index = (int) ((colorindex + 0.40) / 0.05);
			uint32_t next_index = index + 1;
			float t = (float) ((colorindex + 0.40) / 0.05) - index;
			if(index <= 0) {
				index = 0; 
				next_index = 0;
				t = 0.0f;
			}
			else if(index >= 48) {
				index = 48;
				next_index = 48;
				t = 0.0f;
			}
			ColorA color = (1.0f - t)  * lookup[index] + t * lookup[next_index];

			// position
			double ra = toDouble(tokens[7]);
			double dec = toDouble(tokens[8]);
			double distance = toDouble(tokens[9]);

			double alpha = toRadians( ra * 15.0 );
			double delta = toRadians( dec );

			// convert to world (universe) coordinates
			vertices.push_back( distance * Vec3f((float) (sin(alpha) * cos(delta)), (float) sin(delta), (float) (cos(alpha) * cos(delta))) );
			// put extra data (absolute magnitude and distance to Earth) in texture coordinates
			texcoords.push_back( Vec2f( (float) abs_mag, (float) distance) );
			// put color in color attribute
			colors.push_back( color );
		}
		catch(...) {
			// some of the data was invalid, ignore 
			continue;
		}
		
#ifdef _DEBUG
		// only process the 4500 brightest stars (faster loading)
		if(vertices.size() >= 4500) break;
#endif
	}

	// create VboMesh
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticTexCoords2d();
	layout.setStaticColorsRGB();

	mStarMesh = gl::VboMesh(vertices.size(), 0, layout, GL_POINTS);
	mStarMesh.bufferPositions( &(vertices.front()), vertices.size() );
	mStarMesh.bufferTexCoords2d( 0, texcoords );
	mStarMesh.bufferColorsRGB( colors );
}

void StarsApp::createGrid()
{
	const int segments = 60;
	const int rings = 15;
	const int subdiv = 10;

	const float radius = 2000.0f;

	const float theta_step = toRadians(90.0f) / (rings * subdiv);
	const float phi_step = toRadians(360.0f) / (segments * subdiv);
	
	std::vector< Vec3f > vertices;

	// start with the rings, connected at phi=0
	float x, y, z;
	for(int theta=1-rings;theta<rings;++theta) {
		float tr = theta * theta_step * subdiv;

		y = sinf(tr);

		for(int phi=0;phi<=segments;++phi) {
			for(int div=0;div<subdiv;++div) {
				float pr = (phi * subdiv + div) * phi_step;

				x = cosf(tr) * cosf(pr);
				z = cosf(tr) * sinf(pr);
				vertices.push_back( radius * Vec3f(x, y, z) );

				pr += phi_step;

				x = cosf(tr) * cosf(pr);
				z = cosf(tr) * sinf(pr);
				vertices.push_back( radius * Vec3f(x, y, z) );
			}
		}
	}

	// the draw the segments
	for(int phi=0;phi<segments;++phi) {
		float pr = phi * phi_step * subdiv;

		for(int theta=1-rings;theta<rings-1;++theta) {
			for(int div=0;div<subdiv;++div) {
				float tr = (theta * subdiv + div) * theta_step;

				x = cosf(tr) * cosf(pr);
				y = sinf(tr);
				z = cosf(tr) * sinf(pr);
				vertices.push_back( radius * Vec3f(x, y, z) );

				tr += theta_step;

				x = cosf(tr) * cosf(pr);
				y = sinf(tr);
				z = cosf(tr) * sinf(pr);
				vertices.push_back( radius * Vec3f(x, y, z) );
			}
		}
	}
	
	//
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();

	mGridMesh = gl::VboMesh(vertices.size(), 0, layout, GL_LINES);
	mGridMesh.bufferPositions( &(vertices.front()), vertices.size() );	
}

void StarsApp::enablePointSprites()
{
	// store current OpenGL state
	glPushAttrib( GL_POINT_BIT | GL_ENABLE_BIT );

	// enable point sprites and initialize it
	gl::enable( GL_POINT_SPRITE_ARB );
	glPointParameterfARB( GL_POINT_FADE_THRESHOLD_SIZE_ARB, 1.0f );
	glPointParameterfARB( GL_POINT_SIZE_MIN_ARB, 0.1f );
	glPointParameterfARB( GL_POINT_SIZE_MAX_ARB, 200.0f );

	// allow vertex shader to change point size
	gl::enable( GL_VERTEX_PROGRAM_POINT_SIZE );

	// bind sprite texture
	mStarTexture.enableAndBind();

	// bind shader
	mStarShader.bind();
	mStarShader.uniform("tex0", 0);
	mStarShader.uniform("time", (float) getElapsedSeconds() );
}

void StarsApp::disablePointSprites()
{
	// unbind shader and texture
	mStarShader.unbind();
	mStarTexture.unbind();
	
	// restore OpenGL state
	glPopAttrib();
}

void StarsApp::forceHideCursor()
{
	// forces the cursor to hide
#ifdef WIN32
	while( ::ShowCursor(false) >= 0 );
#else
	hideCursor();
#endif
	mIsCursorVisible = false;
}

void StarsApp::forceShowCursor()
{
	// forces the cursor to show
#ifdef WIN32
	while( ::ShowCursor(true) < 0 );
#else
	showCursor();
#endif
	mIsCursorVisible = true;
}

///// UTILITY FUNCTIONS /////

Color StarsApp::toColor(uint32_t hex)
{
	float r = ((hex & 0x00FF0000) >> 16) / 255.0f;
	float g = ((hex & 0x0000FF00) >> 8) / 255.0f;
	float b = ((hex & 0x000000FF)) / 255.0f;

	return Color(r, g, b);
}

ColorA StarsApp::toColorA(uint32_t hex)
{
	float a = ((hex & 0xFF000000) >> 24) / 255.0f;
	float r = ((hex & 0x00FF0000) >> 16) / 255.0f;
	float g = ((hex & 0x0000FF00) >> 8) / 255.0f;
	float b = ((hex & 0x000000FF)) / 255.0f;

	return ColorA(r, g, b, a);
}

int StarsApp::toInt(const std::string &str)
{
	int x;
	std::istringstream i(str);

	if (!(i >> x)) throw std::exception();

	return x;
}

double StarsApp::toDouble(const std::string &str)
{
	double x;
	std::istringstream i(str);

	if (!(i >> x)) throw std::exception();

	return x;
}

CINDER_APP_BASIC( StarsApp, RendererGl )