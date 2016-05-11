#include "cinder/Camera.h"
#include "cinder/CameraUi.h"
#include "cinder/Rand.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DepthOfFieldApp : public App {
public:
	DepthOfFieldApp()
		: mAperture( 0.95f )
		, mFocalDistance( 2.2f )
		, mFocalLength( 1.0f )
		, mFoV(30.0f)
		, mMaxCoCRadiusPixels( 5 )
		, mFarRadiusRescale( 1.0f )
		, mDebugOption( 0 )
		, mResized( true )
	{
	}

	void setup() override;
	void update() override;
	void draw() override;

	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;

	void resize() override;

private:
	CameraPersp mCamera;
	CameraUi    mCameraUi;

	gl::VboRef     mInstances;
	gl::BatchRef   mTeapots, mBackground;
	gl::TextureRef mTexGold, mTexClay;

	gl::FboRef mFboSource;
	gl::FboRef mFboBlur[2];

	gl::GlslProgRef mGlslBlur[2];
	gl::GlslProgRef mGlslComposite;

	params::InterfaceGlRef mParams;
	float                  mAperture;
	float                  mFocalDistance;
	float                  mFocalLength;
	float                  mFoV;
	int                    mMaxCoCRadiusPixels;
	float                  mFarRadiusRescale;
	int                    mDebugOption;

	bool mResized;
};

void DepthOfFieldApp::setup()
{
	// Load the texture.
	mTexGold = gl::Texture2d::create( loadImage( loadAsset( "gold.png" ) ) );
	mTexClay = gl::Texture2d::create( loadImage( loadAsset( "clay.png" ) ) );

	// Initialize instance matrices (one for each instance).
	std::vector<mat4> matrices;
	for( int z = -4; z <= 4; z++ ) {
		for( int y = -4; y <= 4; y++ ) {
			for( int x = -4; x <= 4; x++ ) {
				vec3  axis = Rand::randVec3();
				float angle = Rand::randFloat( -180.0f, 180.0f );

				mat4 transform = glm::translate( vec3( x, y, z ) * 5.0f );
				transform *= glm::rotate( glm::radians( angle ), axis );

				matrices.emplace_back( transform );
			}
		}
	}

	// Setup per-instance data buffer.
	geom::BufferLayout layout;
	layout.append( geom::Attrib::CUSTOM_0, sizeof( mat4 ) / sizeof( float ), sizeof( mat4 ), 0, 1 /* per instance */ );

	mInstances = gl::Vbo::create( GL_ARRAY_BUFFER, matrices.size() * sizeof( mat4 ), matrices.data(), GL_STATIC_DRAW );

	// Create mesh and append per-instance data.
	auto mesh = gl::VboMesh::create( geom::Teapot().subdivisions( 8 ) );
	mesh->appendVbo( layout, mInstances );

	// Load shader.
	auto glsl = gl::GlslProg::create( loadAsset( "instanced.vert" ), loadAsset( "scene.frag" ) );
	glsl->uniform( "uTex", 0 );
	glsl->uniform( "uMaxCoCRadiusPixels", mMaxCoCRadiusPixels );

	// Create batch.
	mTeapots = gl::Batch::create( mesh, glsl, { { geom::Attrib::CUSTOM_0, "vInstanceMatrix" } } );

	//
	mesh = gl::VboMesh::create( geom::Sphere().subdivisions( 60 ).radius(50.0f) );

	glsl = gl::GlslProg::create( loadAsset( "single.vert" ), loadAsset( "scene.frag" ) );
	glsl->uniform( "uTex", 0 );
	glsl->uniform( "uMaxCoCRadiusPixels", mMaxCoCRadiusPixels );

	mBackground = gl::Batch::create( mesh, glsl );

	// Load DoF shaders.
	try {
		{
			auto fmt = gl::GlslProg::Format().vertex( loadAsset( "blur.vert" ) ).fragment( loadAsset( "blur.frag" ) ).define( "HORIZONTAL", "1" );

			mGlslBlur[0] = gl::GlslProg::create( fmt );
			mGlslBlur[0]->uniform( "blurSourceBuffer", 0 );
			mGlslBlur[0]->uniform( "maxCoCRadiusPixels", mMaxCoCRadiusPixels );
			mGlslBlur[0]->uniform( "nearBlurRadiusPixels", 2 );
			mGlslBlur[0]->uniform( "invNearBlurRadiusPixels", 1.0f / 2 );
		}
		{
			auto fmt = gl::GlslProg::Format().vertex( loadAsset( "blur.vert" ) ).fragment( loadAsset( "blur.frag" ) ).define( "HORIZONTAL", "0" );

			mGlslBlur[1] = gl::GlslProg::create( fmt );
			mGlslBlur[1]->uniform( "nearSourceBuffer", 0 );
			mGlslBlur[1]->uniform( "blurSourceBuffer", 1 );
			mGlslBlur[1]->uniform( "maxCoCRadiusPixels", mMaxCoCRadiusPixels );
			mGlslBlur[1]->uniform( "nearBlurRadiusPixels", 2 );
			// mGlslBlur[1]->uniform( "invNearBlurRadiusPixels", 1.0f / 2 );
		}
		{
			auto fmt = gl::GlslProg::Format().vertex( loadAsset( "composite.vert" ) ).fragment( loadAsset( "composite.frag" ) );

			mGlslComposite = gl::GlslProg::create( fmt );
			mGlslComposite->uniform( "packedBuffer", 0 );
			mGlslComposite->uniform( "blurBuffer", 2 );
			mGlslComposite->uniform( "nearBuffer", 1 );
			mGlslComposite->uniform( "shift", vec2( 0 ) );
			mGlslComposite->uniform( "farRadiusRescale", 1.0f );
			mGlslComposite->uniform( "debugOption", 0 );
		}
	}
	catch( const std::exception &exc ) {
		console() << exc.what() << std::endl;
	}

	// Setup the camera.
	mCamera.setPerspective( 30.0f, 1.0f, 0.05f, 500.0f );
	mCamera.lookAt( vec3( 0, 2, 5 ), vec3( 0 ) );
	mCameraUi.setCamera( &mCamera );

	// Setup interface.
	mParams = params::InterfaceGl::create( "Parameters", ivec2( 300, 200 ) );
	mParams->addParam( "Max. CoC Radius", &mMaxCoCRadiusPixels ).min( 1 ).max( 20 ).step( 1 );
	mParams->addParam( "Far Radius Rescale", &mFarRadiusRescale ).min( 0.1f ).max( 20.0f ).step( 0.1f );
	mParams->addParam( "Debug Option", &mDebugOption ).min( 0 ).max( 7 ).step( 1 );
	mParams->addSeparator();
	mParams->addParam( "Aperture", &mAperture, false ).min( 1.0f / 64 ).max( 1.0f ).step( 1.0f / 64 );
	mParams->addParam( "Field of View", &mFoV ).min( 5.0f ).max( 90.0f ).step( 1.0f );
	mParams->addParam( "Focal Distance", &mFocalDistance, false ).min( 0.1f ).max( 100.0f ).step( 0.1f );
	mParams->addParam( "Focal Length", &mFocalLength, true );

	// Note: the Fbo's will be created in the resize() function.
}

void DepthOfFieldApp::update()
{
	// Create or resize Fbo's.
	if( mResized ) {
		mResized = false;

		int width = getWindowWidth();
		int height = getWindowHeight();

		auto fmt = gl::Fbo::Format()
			.attachment( GL_COLOR_ATTACHMENT0, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_RGBA16F ) /*.target( GL_TEXTURE_RECTANGLE )*/ ) )
			.attachment( GL_DEPTH_ATTACHMENT, gl::Renderbuffer::create( width, height, GL_DEPTH24_STENCIL8 ) );
		mFboSource = gl::Fbo::create( width, height, fmt );

		width >>= 2;

		fmt = gl::Fbo::Format()
			.attachment( GL_COLOR_ATTACHMENT0, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_RGBA16F ) /*.target( GL_TEXTURE_RECTANGLE )*/ ) )
			.attachment( GL_COLOR_ATTACHMENT1, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_RGBA16F ) /*.target( GL_TEXTURE_RECTANGLE )*/ ) );
		mFboBlur[0] = gl::Fbo::create( width, height, fmt );

		height >>= 2;

		fmt = gl::Fbo::Format()
			.attachment( GL_COLOR_ATTACHMENT0, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_RGBA16F ) /*.target( GL_TEXTURE_RECTANGLE )*/ ) )
			.attachment( GL_COLOR_ATTACHMENT1, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_RGBA16F ) /*.target( GL_TEXTURE_RECTANGLE )*/ ) );
		mFboBlur[1] = gl::Fbo::create( width, height, fmt );
	}

	// Adjust camera.
	auto distance = glm::clamp( mCamera.getPivotDistance(), 5.0f, 45.0f );
	auto target = mCamera.getPivotPoint();
	auto eye = target - distance * mCamera.getViewDirection();
	mCamera.lookAt( eye, target );

	mCamera.setFov( mFoV );
	mFocalLength = mCamera.getFocalLength();
	mFocalDistance = glm::max( mFocalDistance, mFocalLength );

	//
	Rand::randSeed( 1612112 );

	auto ptr = (mat4*)mInstances->mapReplace();
	for( int z = -4; z <= 4; z++ ) {
		for( int y = -4; y <= 4; y++ ) {
			for( int x = -4; x <= 4; x++ ) {
				vec3  axis = Rand::randVec3();
				float angle = Rand::randFloat( -180.0f, 180.0f ) + 90.0f * float( getElapsedSeconds() );

				mat4 transform = glm::translate( vec3( x, y, z ) * 5.0f );
				transform *= glm::rotate( glm::radians( angle ), axis );

				( *ptr++ ) = transform;
			}
		}
	}
	mInstances->unmap();
}

void DepthOfFieldApp::draw()
{
	gl::clear();

	// Render RGB and normalized CoC (in alpha channel) to Fbo.
	if( true ) {
		gl::ScopedFramebuffer scpFbo( mFboSource );
		gl::ScopedViewport    scpViewport( mFboSource->getSize() );

		gl::clear( ColorA( 0, 0, 0, 0 ) );

		gl::ScopedMatrices scpMatrices;
		gl::setMatrices( mCamera );

		gl::ScopedDepth       scpDepth( true );
		gl::ScopedBlend       scpBlend( false );

		{
			gl::ScopedFaceCulling scpCull( true );
			gl::ScopedColor       scpColor( 1, 1, 1 );

			gl::ScopedTextureBind scpTex0( mTexGold );
			gl::ScopedGlslProg    scpGlsl( mTeapots->getGlslProg() );
			mTeapots->getGlslProg()->uniform( "uAperture", mAperture );
			mTeapots->getGlslProg()->uniform( "uFocalDistance", mFocalDistance );
			mTeapots->getGlslProg()->uniform( "uFocalLength", mFocalLength );
			mTeapots->getGlslProg()->uniform( "uMaxCoCRadiusPixels", mMaxCoCRadiusPixels );

			mTeapots->drawInstanced( 9 * 9 * 9 );
		}

		{
			gl::ScopedFaceCulling scpCull( true, GL_FRONT );
			gl::ScopedColor       scpColor( Color::gray( 0.1f ) );

			gl::ScopedTextureBind scpTex0( mTexClay );
			gl::ScopedGlslProg    scpGlsl( mBackground->getGlslProg() );
			mBackground->getGlslProg()->uniform( "uAperture", mAperture );
			mBackground->getGlslProg()->uniform( "uFocalDistance", mFocalDistance );
			mBackground->getGlslProg()->uniform( "uFocalLength", mFocalLength );
			mBackground->getGlslProg()->uniform( "uMaxCoCRadiusPixels", mMaxCoCRadiusPixels );

			mBackground->draw();
		}
	}

	// Perform horizontal blur and downsampling. Output 2 targets.
	if( true ) {
		gl::ScopedFramebuffer scpFbo( mFboBlur[0] );
		gl::ScopedViewport    scpViewport( mFboBlur[0]->getSize() );

		gl::clear( ColorA( 0, 0, 0, 0 ) );

		gl::ScopedMatrices scpMatrices;
		gl::setMatricesWindow( mFboBlur[0]->getSize() );

		gl::ScopedColor scpColor( 1, 1, 1 );
		gl::ScopedBlendPremult scpBlend;

		gl::ScopedTextureBind scpTex0( mFboSource->getColorTexture() );
		gl::ScopedGlslProg    scpGlsl( mGlslBlur[0] );
		mGlslBlur[0]->uniform( "maxCoCRadiusPixels", mMaxCoCRadiusPixels );

		gl::drawSolidRect( mFboBlur[0]->getBounds() );
	}

	// Perform vertical blur.
	if( true ) {
		gl::ScopedFramebuffer scpFbo( mFboBlur[1] );
		gl::ScopedViewport    scpViewport( mFboBlur[1]->getSize() );

		gl::clear( ColorA( 0, 0, 0, 0 ) );

		gl::ScopedMatrices scpMatrices;
		gl::setMatricesWindow( mFboBlur[1]->getSize() );

		gl::ScopedColor scpColor( 1, 1, 1 );
		gl::ScopedBlendPremult scpBlend;

		gl::ScopedTextureBind scpTex0( mFboBlur[0]->getTexture2d( GL_COLOR_ATTACHMENT0 ), 0 );
		gl::ScopedTextureBind scpTex1( mFboBlur[0]->getTexture2d( GL_COLOR_ATTACHMENT1 ), 1 );
		gl::ScopedGlslProg    scpGlsl( mGlslBlur[1] );
		mGlslBlur[1]->uniform( "maxCoCRadiusPixels", mMaxCoCRadiusPixels );

		gl::drawSolidRect( mFboBlur[1]->getBounds() );
	}

	// Perform compositing.
	if( true ) {
		gl::ScopedColor scpColor( 1, 1, 1 );
		gl::ScopedBlendPremult scpBlend;

		gl::ScopedTextureBind scpTex0( mFboSource->getColorTexture(), 0 );
		gl::ScopedTextureBind scpTex1( mFboBlur[1]->getTexture2d( GL_COLOR_ATTACHMENT0 ), 1 );
		gl::ScopedTextureBind scpTex2( mFboBlur[1]->getTexture2d( GL_COLOR_ATTACHMENT1 ), 2 );
		gl::ScopedGlslProg    scpGlsl( mGlslComposite );
		mGlslComposite->uniform( "packedBufferInvSize", 1.0f / vec2( mFboSource->getSize() ) );
		mGlslComposite->uniform( "farRadiusRescale", mFarRadiusRescale );
		mGlslComposite->uniform( "debugOption", mDebugOption );

		gl::drawSolidRect( getWindowBounds() );
	}

	mParams->draw();
}

void DepthOfFieldApp::mouseDown( MouseEvent event )
{
	mCameraUi.mouseDown( event );
}

void DepthOfFieldApp::mouseDrag( MouseEvent event )
{
	mCameraUi.mouseDrag( event );
}

void DepthOfFieldApp::resize()
{
	mCamera.setAspectRatio( getWindowAspectRatio() );
	mResized = true;
}

CINDER_APP( DepthOfFieldApp, RendererGl )
