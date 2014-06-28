#include "SMAA.h"

#include "AreaTex.h"
#include "SearchTex.h"

using namespace ci;
using namespace std;

void SMAA::setup()
{
	// Load and compile our shaders
	mSMAAFirstPass = Shader::create("smaa_1st");
	mSMAASecondPass = Shader::create("smaa_2nd");
	mSMAAThirdPass = Shader::create("smaa_3rd");

	// Create lookup textures
	gl::Texture::Format fmt;
	fmt.setMinFilter( GL_LINEAR );
	fmt.setMagFilter( GL_LINEAR );
	fmt.setWrap( GL_CLAMP, GL_CLAMP );

	// Search Texture (Grayscale, 8 bits unsigned)
	fmt.setInternalFormat( GL_LUMINANCE );
	mSearchTex = gl::Texture::create(searchTexBytes, GL_LUMINANCE, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, fmt);

	// Area Texture (Red+Green Channels, 8 bits unsigned)
	fmt.setInternalFormat( GL_RG );
	mAreaTex = gl::Texture::create(areaTexBytes, GL_RG, AREATEX_WIDTH, AREATEX_HEIGHT, fmt);
}

void SMAA::apply(ci::gl::Fbo& destination, ci::gl::Fbo& source)
{
	// Source and destination should have the same size
	assert(destination.getWidth() == source.getWidth());
	assert(destination.getHeight() == source.getHeight());
	
	// Create or resize frame buffers
	int w = source.getWidth();
	int h = source.getHeight();
	
	gl::Fbo::Format fmt;
	fmt.setMinFilter( GL_LINEAR );
	fmt.setMagFilter( GL_LINEAR );

	if(!mFboEdgePass || mFboEdgePass.getWidth() != w || mFboEdgePass.getHeight() != h)
	{
		fmt.setColorInternalFormat( GL_RG );
		mFboEdgePass = gl::Fbo( w, h, fmt );
		mFboEdgePass.getTexture().setFlipped(true);
	}

	if(!mFboBlendPass || mFboBlendPass.getWidth() != w || mFboBlendPass.getHeight() != h)
	{
		fmt.setColorInternalFormat( GL_RGBA );
		mFboBlendPass = gl::Fbo( w, h, fmt );
		mFboBlendPass.getTexture().setFlipped(true);
	}

	// Apply first two passes
	doEdgePass(source);
	doBlendPass();

	// Apply SMAA
	destination.bindFramebuffer();

	mFboBlendPass.getTexture().bind(1);

	mSMAAThirdPass->prog().bind();
	mSMAAThirdPass->prog().uniform("uColorTex", 0);
	mSMAAThirdPass->prog().uniform("uBlendTex", 1);
	mSMAAThirdPass->prog().uniform("SMAA_RT_METRICS", Vec4f(1.0f/w, 1.0f/h, (float)w, (float)h));
	{
		gl::clear();
		gl::color( Color::white() );

		gl::draw( source.getTexture(), destination.getBounds() );
	}
	mSMAAThirdPass->prog().unbind();

	mFboBlendPass.getTexture().unbind();

	// Disable frame buffer
	destination.unbindFramebuffer();
}

gl::Texture& SMAA::getEdgePass()
{
	return mFboEdgePass.getTexture();
}

gl::Texture& SMAA::getBlendPass()
{
	return mFboBlendPass.getTexture();
}

void SMAA::doEdgePass(ci::gl::Fbo& source)
{
	int w = mFboEdgePass.getWidth();
	int h = mFboEdgePass.getHeight();

	// Enable frame buffer
	mFboEdgePass.bindFramebuffer();

	mSMAAFirstPass->prog().bind();
	mSMAAFirstPass->prog().uniform("uColorTex", 0);
	mSMAAFirstPass->prog().uniform("SMAA_RT_METRICS", Vec4f(1.0f/w, 1.0f/h, (float)w, (float)h));
	{
		gl::clear();
		gl::color( Color::white() );

		gl::draw( source.getTexture(), mFboEdgePass.getBounds() );
	}
	mSMAAFirstPass->prog().unbind();

	// Disable frame buffer
	mFboEdgePass.unbindFramebuffer();
}

void SMAA::doBlendPass()
{
	int w = mFboBlendPass.getWidth();
	int h = mFboBlendPass.getHeight();

	// Enable frame buffer
	mFboBlendPass.bindFramebuffer();

	mAreaTex->bind(1);
	mSearchTex->bind(2);

	mSMAASecondPass->prog().bind();
	mSMAASecondPass->prog().uniform("uEdgesTex", 0);
	mSMAASecondPass->prog().uniform("uAreaTex", 1);
	mSMAASecondPass->prog().uniform("uSearchTex", 2);
	mSMAASecondPass->prog().uniform("SMAA_RT_METRICS", Vec4f(1.0f/w, 1.0f/h, (float)w, (float)h));
	{
		gl::clear();
		gl::color( Color::white() );

		gl::draw( mFboEdgePass.getTexture(), mFboBlendPass.getBounds() );
	}
	mSMAASecondPass->prog().unbind();

	mSearchTex->unbind();
	mAreaTex->unbind();

	// Disable frame buffer
	mFboBlendPass.unbindFramebuffer();
}