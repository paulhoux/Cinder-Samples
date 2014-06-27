#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Camera.h"
#include "cinder/Rand.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// Create a class that can draw a single box
struct Box
{		
	Box(float x, float z)
		: offset( Rand::randFloat(0.0f, 10.0f) )
		, color( CM_HSV, Rand::randFloat(0.0f, 0.15f), 1.0f, Rand::randFloat(0.5f, 1.0f) )
		, position( Vec3f(x, 0.0f, z) )
	{}

	void draw(float time)
	{
		float t = offset + time;
		float height = 55.0f + 45.0f * math<float>::sin(t);

		gl::color( color );
		gl::drawCube( position + Vec3f(0, 0.5f * height, 0), Vec3f(10.0f, height, 10.0f) );
	}

	float offset;
	Colorf color;
	Vec3f position;
};
typedef std::shared_ptr<Box> BoxRef;

class FXAAApp : public AppNative {
public:
	void setup();
	void update();
	void draw();

	void mouseDown( MouseEvent event );	
	void mouseDrag( MouseEvent event );	

	void keyDown( KeyEvent event );

	void resize();
private:
	CameraPersp         mCamera;
	gl::Fbo				mFbo;
	gl::GlslProg        mShader;
	gl::GlslProg        mFXAA;
	std::vector<BoxRef>	mBoxes;

	Timer				mTimer;
	double				mTime;
	double				mTimeOffset;

	bool				mEnableFXAA;
};

void FXAAApp::setup()
{
	// Load and compile our shader, which makes our boxes look prettier
	try { 
		mShader = gl::GlslProg( loadAsset("phong_vert.glsl"), loadAsset("phong_frag.glsl") ); 
		mFXAA = gl::GlslProg( loadAsset("fxaa_vert.glsl"), loadAsset("fxaa_frag.glsl") ); 
	}
	catch( const std::exception& e ) { console() << e.what() << std::endl; quit(); }

	// Create the boxes
	for(int x=-50; x<=50; x+=10)
		for(int z=-50; z<=50; z+=10)
			mBoxes.push_back( std::make_shared<Box>( float(x), float(z) ) );

	//
	mTimeOffset = 0.0;
	mTimer.start();

	mEnableFXAA = true;
}

void FXAAApp::update()
{
	// Note: this function is only called once per frame

	// Keep track of time
	mTime = mTimer.getSeconds() + mTimeOffset;

	// Animate our camera
	double t = mTime / 10.0;

	float phi = (float) t;
	float theta = 3.14159265f * (0.25f + 0.2f * math<float>::sin(phi * 0.9f));
	float x = 150.0f * math<float>::cos(phi) * math<float>::cos(theta);
	float y = 150.0f * math<float>::sin(theta);
	float z = 150.0f * math<float>::sin(phi) * math<float>::cos(theta);

	mCamera.setEyePoint( Vec3f(x, y, z) );
	mCamera.setCenterOfInterestPoint( Vec3f(1, 50, 0) );
	mCamera.setAspectRatio( getWindowAspectRatio() );
	mCamera.setFov( 60.0f );
}

void FXAAApp::draw()
{
	// enable frame buffer
	mFbo.bindFramebuffer();

	// draw scene
	gl::clear();

	gl::enableDepthRead();
	gl::enableDepthWrite();
	{
		gl::pushMatrices();
		gl::setMatrices( mCamera );
		{
			mShader.bind();
			mShader.uniform("bLuminanceInAlpha", mEnableFXAA);
			{
				for(auto &box : mBoxes)
					box->draw((float) mTime);
			}
			mShader.unbind();
		}
		gl::popMatrices();
	}
	gl::disableDepthWrite();
	gl::disableDepthRead();

	// disable frame buffer
	mFbo.unbindFramebuffer();

	// draw the frame buffer while applying FXAA
	mFXAA.bind();
	mFXAA.uniform("COLOR", 0);
	mFXAA.uniform("buffersize", Vec2f( mFbo.getSize() ));
	
	gl::clear();
	gl::draw( mFbo.getTexture() );

	mFXAA.unbind();
}

void FXAAApp::mouseDown( MouseEvent event )
{
}

void FXAAApp::mouseDrag( MouseEvent event )
{
}

void FXAAApp::keyDown( KeyEvent event )
{
	switch( event.getCode() )
	{
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_f:
		mEnableFXAA = !mEnableFXAA;
		break;
	case KeyEvent::KEY_t:
		if(mTimer.isStopped())
		{
			mTimeOffset += mTimer.getSeconds();
			mTimer.start();
		}
		else
			mTimer.stop();
		break;
	}
}

void FXAAApp::resize()
{
	gl::Fbo::Format fmt;
	fmt.setColorInternalFormat( GL_RGBA16F );
	mFbo = gl::Fbo( getWindowWidth(), getWindowHeight(), fmt );
	mFbo.getTexture().setFlipped(true);
}

CINDER_APP_NATIVE( FXAAApp, RendererGl( RendererGl::AA_NONE ) )
