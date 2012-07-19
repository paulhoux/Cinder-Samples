#include "cinder/ImageIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Surface.h"
#include "cinder/Utilities.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>

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

	gl::GlslProg	mShader;
	gl::Texture		mTexture;
	gl::VboMesh		mVboMesh;

	bool			mEnableAnimation;
	bool			mEnablePointSprites;
	bool			mEnableAutoPointSprites;
};

void StarsApp::prepareSettings(Settings *settings)
{
	settings->setFrameRate(100.0f);
	settings->setFullScreen(true);
}

void StarsApp::setup()
{
	// load the star database and create the VBO mesh
	loadStars();

	// initialize camera
	mCameraEyePoint = Vec3f(1, 0, 0);

	mCam.setFov(60.0f);
	mCam.setNearClip( 0.1f );
	mCam.setFarClip( 10000.0f );
	mCam.setEyePoint( mCameraEyePoint );
	mCam.setCenterOfInterestPoint( Vec3f(0, 0, 0) );
	
	mMayaCam.setCurrentCam( mCam );
	
	// load shader and point sprite texture

	/*// DISABLED: not necessary - fixed function pipeline is sufficient
	try { mShader = gl::GlslProg( loadAsset("stars_vert.glsl"), loadAsset("stars_frag.glsl") ); }
	catch( const std::exception &e ) { console() << e.what() << std::endl; }	//*/

	try { mTexture = gl::Texture( loadImage( loadAsset("particle.png") ) ); }
	catch( const std::exception &e ) { console() << e.what() << std::endl; }

	//
	mEnablePointSprites = true;
	mEnableAnimation = true;
}

void StarsApp::update()
{	
	// animate camera
	if(mEnableAnimation) {
		float f = math<float>::clamp(cosf( (float) getElapsedSeconds() * 0.02f ) * 1.2f + 0.2f, -1.0f, 1.0f);
		float d = 500.0f - 499.9999f * f;
		float t = (float) getElapsedSeconds() * 0.03f;
		float x = d * cosf(t) * (0.5f + 0.45f * cosf(t));
		float y = d * (0.5f + 0.45f * sinf(t));
		float z = d * sinf(t) * (0.5f + 0.45f * cosf(t));

		mCameraEyePoint = Vec3f(x, y, z);
	}

	mCam.setEyePoint( mCameraEyePoint );
	mCam.setCenterOfInterestPoint( Vec3f(0, 0, 0) );

	mMayaCam.setCurrentCam( mCam );

	// by experimentation, I found that a distance of 760 lightyears (210 parsecs) 
	// gave no visible change when going from point sprites to points (at point size of 40).
	// To reduce overdraw, and increase the frame rate, automatically switch between them.
	if(mEnableAutoPointSprites)
		mEnablePointSprites = ( mCameraEyePoint.length() < 210.0f );
}

void StarsApp::draw()
{
	gl::clear(); 

	gl::pushMatrices();
	gl::setMatrices( mMayaCam.getCamera() );
	{
		gl::enableAdditiveBlending();	
		{
			if(mEnablePointSprites) enablePointSprites();
			{
				gl::color( Color::white() );
				if(mVboMesh) gl::draw( mVboMesh );
			}
			if(mEnablePointSprites) disablePointSprites();
		}
		gl::disableAlphaBlending();
	}
	gl::popMatrices();

	gl::enableAlphaBlending();
	std::string fmt("%5.0f lightyears from Earth");
	gl::drawString( (boost::format(fmt) % (mCameraEyePoint.length() * 3.261631f)).str(), Vec2f::zero());
	gl::disableAlphaBlending();
}

void StarsApp::mouseDown( MouseEvent event )
{
	// allow user to control camera
	mEnableAnimation = false;

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
		break;
	case KeyEvent::KEY_p:
		// toggle point sprites
		mEnablePointSprites = !mEnablePointSprites;
		mEnableAutoPointSprites = mEnablePointSprites;
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

	std::vector< Vec3f > vertices;
	std::vector< Vec2f > texcoords;
	std::vector< Color > colors;

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

	// load the star database
	std::string	stars = loadString( loadAsset("hyg.csv") );

	double max_brightness = 0.0;

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
		if(tokens.size() < 6)  continue;

		// 
		try {
			// brightness (magnitude) of the star
			double mag = toDouble(tokens[5]);
			double brightness = pow(pogson, -mag) * brightness_amplifier;
			brightness = pow(brightness, 1.0 / gamma); // gamma correction
			if(brightness < brightness_threshold) continue;

			// color (spectrum) of the star
			double colorindex = (tokens.size() > 6) ? toDouble(tokens[6]) : 0.0;
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
			double ra = toDouble(tokens[2]);
			double dec = toDouble(tokens[3]);
			double distance = toDouble(tokens[4]);

			double alpha = toRadians( ra * 15.0 );
			double delta = toRadians( dec );

			float size = math<float>::pow( (float) (brightness / brightness_amplifier), 0.1f );

			vertices.push_back( distance * Vec3f((float) (sin(alpha) * cos(delta)), (float) sin(delta), (float) (cos(alpha) * cos(delta))) );
			texcoords.push_back( Vec2f( size, (float) distance) );
			colors.push_back( color * (float) brightness );
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

	mVboMesh = gl::VboMesh(vertices.size(), 0, layout, GL_POINTS);
	mVboMesh.bufferPositions( &(vertices.front()), vertices.size() );
	mVboMesh.bufferTexCoords2d( 0, texcoords );
	mVboMesh.bufferColorsRGB( colors );
}

void StarsApp::enablePointSprites()
{
	glPushAttrib( GL_POINT_BIT | GL_ENABLE_BIT );
	gl::enable( GL_POINT_SPRITE_ARB );
	gl::enable( GL_VERTEX_PROGRAM_POINT_SIZE );
	gl::enableAdditiveBlending();

	mTexture.enableAndBind();
	
	if(mShader) {
		mShader.bind();
		mShader.uniform("size", 24.0f);
	}
	else {
		// use fixed-function pipeline
		float quadratic[] =  { 0.95f, 0.02f, 0.0f };
		glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION_ARB, quadratic );
		glPointParameterfARB( GL_POINT_FADE_THRESHOLD_SIZE_ARB, 1.0f );
		glPointParameterfARB( GL_POINT_SIZE_MIN_ARB, 0.1f );
		glPointParameterfARB( GL_POINT_SIZE_MAX_ARB, 24.0f );
		glPointSize( 24.0f );

		glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE );
	}
}

void StarsApp::disablePointSprites()
{
	mShader.unbind();
	mTexture.unbind();
	
	glPopAttrib();
}

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
