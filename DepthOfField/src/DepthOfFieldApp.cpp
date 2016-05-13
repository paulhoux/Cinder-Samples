#include "cinder/Camera.h"
#include "cinder/CameraUi.h"
#include "cinder/Rand.h"
#include "cinder/Sphere.h"
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
	    : mAperture( 1 )
	    , mFocalStop( 8 )
	    , mFocalPlane( 10 )
	    , mFocalLength( 1.0f )
	    , mFoV( 25 )
	    , mMaxCoCRadiusPixels( 8 )
	    , mFarRadiusRescale( 1.0f )
	    , mDebugOption( 0 )
	    , mTime( 0 )
	    , mPaused( false )
	    , mResized( true )
	{
	}

	static void prepare( Settings *settings );

	void setup() override;
	void update() override;
	void update( double timestep );
	void draw() override;

	void mouseMove( MouseEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;

	void keyDown( KeyEvent event ) override;
	void keyUp( KeyEvent event ) override { mShiftDown = event.isShiftDown(); }

	void resize() override;

	void reload();

  private:
	CameraPersp mCamera;
	CameraUi    mCameraUi;

	Sphere mBounds;

	gl::VboRef     mInstances;
	gl::BatchRef   mTeapots, mBackground, mSpheres;
	gl::TextureRef mTexGold, mTexClay;

	gl::FboRef mFboSource;
	gl::FboRef mFboBlur[2];

	gl::GlslProgRef mGlslBlur[2];
	gl::GlslProgRef mGlslComposite;

	params::InterfaceGlRef mParams;
	float                  mAperture;
	int                    mFocalStop;
	float                  mFocalPlane;
	float                  mFocalLength;
	float                  mFoV;
	float                  mCoCScale, mCoCBias;
	int                    mMaxCoCRadiusPixels;
	float                  mFarRadiusRescale;
	int                    mDebugOption;

	double mTime;

	bool mPaused;
	bool mResized;
	bool mShiftDown;

	vec2 mMousePos;
};

void DepthOfFieldApp::prepare( Settings *settings )
{
	settings->setWindowSize( 960, 540 );
}

void DepthOfFieldApp::setup()
{
	// Create dummy shader. Actual shaders will be loaded in the reload()
	// function.
	auto glsl = gl::getStockShader( gl::ShaderDef() );

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
	AxisAlignedBox bounds;

	auto mesh = gl::VboMesh::create( geom::Teapot().subdivisions( 8 ) >> geom::Bounds( &bounds ) );
	mesh->appendVbo( layout, mInstances );

	mBounds.setCenter( bounds.getCenter() );
	mBounds.setRadius( 0.5f * glm::length( bounds.getExtents() ) ); // Scale down for a better fit.

	// Create batch.
	mTeapots = gl::Batch::create( mesh, glsl, { { geom::Attrib::CUSTOM_0, "vInstanceMatrix" } } );

	// mesh = gl::VboMesh::create( geom::WireCube().size( bounds.getExtents() * 2.0f ) >> geom::Translate( bounds.getCenter() ) );
	mesh = gl::VboMesh::create( geom::WireSphere().center( mBounds.getCenter() ).radius( mBounds.getRadius() ) );
	mesh->appendVbo( layout, mInstances );

	mSpheres = gl::Batch::create( mesh, glsl, { { geom::Attrib::CUSTOM_0, "vInstanceMatrix" } } );

	// Create background.
	mesh = gl::VboMesh::create( geom::Sphere().subdivisions( 60 ).radius( 50.0f ) );
	mBackground = gl::Batch::create( mesh, glsl );

	// Setup the camera.
	mCamera.setPerspective( 30.0f, 1.0f, 0.05f, 100.0f );
	mCamera.lookAt( vec3( 1, 2, 5 ), vec3( 0 ) );
	mCameraUi.setCamera( &mCamera );

	// Setup interface.
	mParams = params::InterfaceGl::create( "Parameters", ivec2( 320, 240 ) );
	mParams->setOptions( "", "valueswidth=120" );
	mParams->setOptions( "", "refresh=0.05" );
	mParams->addParam( "Focal Distance", &mFocalPlane, false ).min( 0.1f ).max( 100.0f ).step( 0.1f );
	mParams->addParam( "F-stop", { "0.7", "0.8", "1.0", "1.2", "1.4", "1.7", "2.0", "2.4", "2.8", "3.3", "4.0", "4.8", "5.6", "6.7", "8.0", "9.5", "11.0" }, &mFocalStop, false );
	mParams->addParam( "Field of View", &mFoV ).min( 5.0f ).max( 90.0f ).step( 1.0f );
	mParams->addSeparator();
	mParams->addParam( "Aperture", &mAperture, true );
	mParams->addParam( "Focal Length", &mFocalLength, true );
	mParams->addSeparator();
	mParams->addParam( "Max. CoC Radius", &mMaxCoCRadiusPixels ).min( 1 ).max( 20 ).step( 1 );
	mParams->addParam( "Far Radius Rescale", &mFarRadiusRescale ).min( 0.1f ).max( 20.0f ).step( 0.1f );
	mParams->addParam( "Debug Option", { "Off", "Show CoC", "Show Region", "Show Near", "Show Blurry", "Show Input", "Show Mid & Far", "Show Signed CoC" }, &mDebugOption );
	mParams->addSeparator();
	mParams->addButton( "Pause", [&]() { mPaused = !mPaused; } );
	mParams->addText( "Hold SHIFT to auto-focus." );

	// Note: the Fbo's will be created in the resize() function.

	//
	reload();
}

void DepthOfFieldApp::update()
{
	// Create or resize Fbo's.
	if( mResized ) {
		mResized = false;

		int width = getWindowWidth();
		int height = getWindowHeight();

		auto fmt = gl::Fbo::Format()
		               .samples( 16 )
		               .attachment( GL_COLOR_ATTACHMENT0, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_RGBA16F ) ) )
		               .attachment( GL_DEPTH_STENCIL_ATTACHMENT, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_DEPTH24_STENCIL8 ).dataType( GL_UNSIGNED_INT_24_8 ) ) );
		mFboSource = gl::Fbo::create( width, height, fmt );

		width >>= 2;

		fmt = gl::Fbo::Format()
		          .attachment( GL_COLOR_ATTACHMENT0, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_RGBA16F ) ) )
		          .attachment( GL_COLOR_ATTACHMENT1, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_RGBA16F ) ) );
		mFboBlur[0] = gl::Fbo::create( width, height, fmt );

		height >>= 2;

		fmt = gl::Fbo::Format()
		          .attachment( GL_COLOR_ATTACHMENT0, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_RGBA16F ) ) )
		          .attachment( GL_COLOR_ATTACHMENT1, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_RGBA16F ) ) );
		mFboBlur[1] = gl::Fbo::create( width, height, fmt );
	}

	// Use a fixed time step for a steady 60 updates per second.
	static const double timestep = 1.0 / 60.0;

	// Keep track of time.
	static double time = getElapsedSeconds();
	static double accumulator = 0.0;

	// Calculate elapsed time since last frame.
	double elapsed = getElapsedSeconds() - time;
	time += elapsed;

	// Update all nodes in the scene graph.
	accumulator += math<double>::min( elapsed, 0.1 ); // prevents 'spiral of death'

	while( accumulator >= timestep ) {
		update( mPaused ? 0.0 : timestep );

		accumulator -= timestep;
	}
}

void DepthOfFieldApp::update( double timestep )
{
	mTime += timestep;

	// Adjust camera.
	auto distance = glm::clamp( mCamera.getPivotDistance(), 5.0f, 45.0f );
	auto target = mCamera.getPivotPoint();
	auto eye = target - distance * mCamera.getViewDirection();
	mCamera.lookAt( eye, target );

	mCamera.setFov( mFoV );
	mFocalLength = mCamera.getFocalLength();
	mFocalPlane = glm::max( mFocalPlane, mFocalLength );

	static const float fstops[] = { 0.7f, 0.8f, 1.0f, 1.2f, 1.4f, 1.7f, 2.0f, 2.4f, 2.8f, 3.3f, 4.0f, 4.8f, 5.6f, 6.7f, 8.0f, 9.5f, 11.0f };
	mAperture = 1.0f / fstops[mFocalStop];

	// float zNear = mCamera.getNearClip();
	// float zFar = mCamera.getFarClip();
	// mCoCScale = ( mAperture * mFocalLength * mFocalPlane * ( zFar - zNear ) ) / ( ( mFocalPlane - mFocalLength ) * zNear * zFar );
	// mCoCBias = ( mAperture * mFocalLength * ( zNear - mFocalPlane ) ) / ( ( mFocalPlane * mFocalLength ) * zNear );

	// Initialize ray-casting.
	auto  ray = mCamera.generateRay( mMousePos, getWindowSize() );
	float min, max, dist = FLT_MAX;

	//
	Rand::randSeed( 1612112 );

	// Animate teapots and perform ray casting at the same time.
	auto ptr = (mat4 *)mInstances->mapReplace();
	for( int z = -4; z <= 4; z++ ) {
		for( int y = -4; y <= 4; y++ ) {
			for( int x = -4; x <= 4; x++ ) {
				vec3  position = vec3( x, y, z ) * 5.0f;
				vec3  axis = Rand::randVec3();
				float angle = Rand::randFloat( -180.0f, 180.0f ) + Rand::randFloat( 1.0f, 90.0f ) * float( mTime );

				mat4 transform = glm::translate( position );
				transform *= glm::rotate( glm::radians( angle ), axis );

				( *ptr++ ) = transform;

				// Ray-casting.
				auto bounds = mBounds.transformed( transform );
				if( bounds.intersect( ray, &min, &max ) > 0 ) {
					if( min < dist )
						dist = min;
				}
			}
		}
	}
	mInstances->unmap();

	// Auto-focus.
	if( mShiftDown && dist < FLT_MAX ) {
		mFocalPlane = dist;
	}
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

		gl::ScopedDepth scpDepth( true );
		gl::ScopedBlend scpBlend( false );

		{
			gl::ScopedFaceCulling scpCull( true );
			gl::ScopedColor       scpColor( 1, 1, 1 );

			gl::ScopedTextureBind scpTex0( mTexGold );
			gl::ScopedGlslProg    scpGlsl( mTeapots->getGlslProg() );
			mTeapots->getGlslProg()->uniform( "uAperture", mAperture );
			mTeapots->getGlslProg()->uniform( "uFocalDistance", mFocalPlane );
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
			mBackground->getGlslProg()->uniform( "uFocalDistance", mFocalPlane );
			mBackground->getGlslProg()->uniform( "uFocalLength", mFocalLength );
			mBackground->getGlslProg()->uniform( "uMaxCoCRadiusPixels", mMaxCoCRadiusPixels );

			mBackground->draw();
		}

		if( false ) {
			gl::ScopedColor scpColor( 0, 1, 1 );
			mSpheres->drawInstanced( 9 * 9 * 9 );
		}
	}

	// Perform horizontal blur and downsampling. Output 2 targets.
	if( true ) {
		gl::ScopedFramebuffer scpFbo( mFboBlur[0] );
		gl::ScopedViewport    scpViewport( mFboBlur[0]->getSize() );

		gl::clear( ColorA( 0, 0, 0, 0 ) );

		gl::ScopedMatrices scpMatrices;
		gl::setMatricesWindow( mFboBlur[0]->getSize() );

		gl::ScopedColor        scpColor( 1, 1, 1 );
		gl::ScopedBlendPremult scpBlend;

		gl::ScopedTextureBind scpTex0( mFboSource->getColorTexture() );
		gl::ScopedGlslProg    scpGlsl( mGlslBlur[0] );
		mGlslBlur[0]->uniform( "maxCoCRadiusPixels", mMaxCoCRadiusPixels );
		mGlslBlur[0]->uniform( "nearBlurRadiusPixels", mMaxCoCRadiusPixels );
		mGlslBlur[0]->uniform( "invNearBlurRadiusPixels", 1.0f / mMaxCoCRadiusPixels );

		gl::drawSolidRect( mFboBlur[0]->getBounds() );
	}

	// Perform vertical blur.
	if( true ) {
		gl::ScopedFramebuffer scpFbo( mFboBlur[1] );
		gl::ScopedViewport    scpViewport( mFboBlur[1]->getSize() );

		gl::clear( ColorA( 0, 0, 0, 0 ) );

		gl::ScopedMatrices scpMatrices;
		gl::setMatricesWindow( mFboBlur[1]->getSize() );

		gl::ScopedColor        scpColor( 1, 1, 1 );
		gl::ScopedBlendPremult scpBlend;

		gl::ScopedTextureBind scpTex0( mFboBlur[0]->getTexture2d( GL_COLOR_ATTACHMENT0 ), 0 );
		gl::ScopedTextureBind scpTex1( mFboBlur[0]->getTexture2d( GL_COLOR_ATTACHMENT1 ), 1 );
		gl::ScopedGlslProg    scpGlsl( mGlslBlur[1] );
		mGlslBlur[1]->uniform( "maxCoCRadiusPixels", mMaxCoCRadiusPixels );
		mGlslBlur[1]->uniform( "nearBlurRadiusPixels", mMaxCoCRadiusPixels );
		mGlslBlur[1]->uniform( "invNearBlurRadiusPixels", 1.0f / mMaxCoCRadiusPixels );

		gl::drawSolidRect( mFboBlur[1]->getBounds() );
	}

	// Perform compositing.
	if( true ) {
		gl::ScopedColor scpColor( 1, 1, 1 );
		gl::ScopedBlend scpBlend( false );

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

void DepthOfFieldApp::mouseMove( MouseEvent event )
{
	mShiftDown = event.isShiftDown();
	mMousePos = event.getPos();
}

void DepthOfFieldApp::mouseDown( MouseEvent event )
{
	mCameraUi.mouseDown( event );
}

void DepthOfFieldApp::mouseDrag( MouseEvent event )
{
	mCameraUi.mouseDrag( event );

	mShiftDown = event.isShiftDown();
	mMousePos = event.getPos();
}

void DepthOfFieldApp::keyDown( KeyEvent event )
{
	mShiftDown = event.isShiftDown();

	switch( event.getCode() ) {
	case KeyEvent::KEY_ESCAPE:
		if( isFullScreen() )
			setFullScreen( false );
		else
			quit();
		break;
	case KeyEvent::KEY_SPACE:
		mPaused = !mPaused;
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_r:
		reload();
		break;
	default:
		break;
	}
}

void DepthOfFieldApp::resize()
{
	mCamera.setAspectRatio( getWindowAspectRatio() );
	mResized = true;
}

void DepthOfFieldApp::reload()
{
	try {
		auto glsl = gl::GlslProg::create( loadAsset( "instanced.vert" ), loadAsset( "scene.frag" ) );
		glsl->uniform( "uTex", 0 );
		glsl->uniform( "uMaxCoCRadiusPixels", mMaxCoCRadiusPixels );

		if( mTeapots )
			mTeapots->replaceGlslProg( glsl );
	}
	catch( const std::exception &exc ) {
		console() << "Failed to load teapots shader: " << exc.what() << std::endl;
	}

	try {
		auto glsl = gl::GlslProg::create( loadAsset( "instanced.vert" ), loadAsset( "debug.frag" ) );

		if( mSpheres )
			mSpheres->replaceGlslProg( glsl );
	}
	catch( const std::exception &exc ) {
		console() << "Failed to load teapots shader: " << exc.what() << std::endl;
	}

	try {
		auto glsl = gl::GlslProg::create( loadAsset( "single.vert" ), loadAsset( "scene.frag" ) );
		glsl->uniform( "uTex", 0 );
		glsl->uniform( "uMaxCoCRadiusPixels", mMaxCoCRadiusPixels );

		if( mBackground )
			mBackground->replaceGlslProg( glsl );
	}
	catch( const std::exception &exc ) {
		console() << "Failed to load background shader: " << exc.what() << std::endl;
	}

	// Load DoF shaders.
	try {
		auto fmt = gl::GlslProg::Format().vertex( loadAsset( "blur.vert" ) ).fragment( loadAsset( "blur.frag" ) ).define( "HORIZONTAL", "1" );

		mGlslBlur[0] = gl::GlslProg::create( fmt );
		mGlslBlur[0]->uniform( "blurSourceBuffer", 0 );
	}
	catch( const std::exception &exc ) {
		console() << "Failed to load horizontal blur shader: " << exc.what() << std::endl;
	}
	try {
		auto fmt = gl::GlslProg::Format().vertex( loadAsset( "blur.vert" ) ).fragment( loadAsset( "blur.frag" ) ).define( "HORIZONTAL", "0" );

		mGlslBlur[1] = gl::GlslProg::create( fmt );
		mGlslBlur[1]->uniform( "nearSourceBuffer", 0 );
		mGlslBlur[1]->uniform( "blurSourceBuffer", 1 );
	}
	catch( const std::exception &exc ) {
		console() << "Failed to load vertical blur shader: " << exc.what() << std::endl;
	}
	try {
		auto fmt = gl::GlslProg::Format().vertex( loadAsset( "composite.vert" ) ).fragment( loadAsset( "composite.frag" ) );

		mGlslComposite = gl::GlslProg::create( fmt );
		mGlslComposite->uniform( "packedBuffer", 0 );
		mGlslComposite->uniform( "blurBuffer", 2 );
		mGlslComposite->uniform( "nearBuffer", 1 );
		mGlslComposite->uniform( "shift", vec2( 0 ) );
		mGlslComposite->uniform( "farRadiusRescale", 1.0f );
		mGlslComposite->uniform( "debugOption", 0 );
	}
	catch( const std::exception &exc ) {
		console() << "Failed to load composite shader: " << exc.what() << std::endl;
	}
}

CINDER_APP( DepthOfFieldApp, RendererGl, DepthOfFieldApp::prepare )
