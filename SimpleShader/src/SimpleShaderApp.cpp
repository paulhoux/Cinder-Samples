#include "cinder/ImageIo.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class SimpleShaderApp : public AppBasic {
public:
	void setup();	
	void update();
	void draw();
private:
	gl::Texture		mTexture0;
	gl::Texture		mTexture1;

	gl::GlslProg	mShader;
};

void SimpleShaderApp::setup()
{
	try {
		// load the two textures
		mTexture0 = gl::Texture( loadImage( loadAsset("bottom.jpg") ) );
		mTexture1 = gl::Texture( loadImage( loadAsset("top.jpg") ) );
	
		// load and compile the shader
		mShader = gl::GlslProg( loadAsset("shader_vert.glsl"), loadAsset("shader_frag.glsl") );
	}
	catch( const std::exception &e ) {
		// if anything went wrong, show it in the output window
		console() << e.what() << std::endl;
	}
}

void SimpleShaderApp::update()
{
}

void SimpleShaderApp::draw()
{	
	gl::clear();

	mShader.bind();
	mShader.uniform("tex0", 0);	
	mShader.uniform("tex1", 1);

	// enable the use of textures
	gl::enable( GL_TEXTURE_2D );

	// bind them
	mTexture0.bind(0);
	mTexture1.bind(1);

	// now run the shader for every pixel in the window
	// by drawing a full screen rectangle
	gl::drawSolidRect( Rectf( getWindowBounds() ), false );
}

CINDER_APP_BASIC( SimpleShaderApp, RendererGl )
