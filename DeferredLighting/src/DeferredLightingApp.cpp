#include "cinder/Camera.h"
#include "cinder/CameraUi.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#define OBJECTS_X 10
#define OBJECTS_Y 10
#define OBJECTS_Z 10
#define OBJECTS_COUNT ( OBJECTS_X * OBJECTS_Y * OBJECTS_Z )
#define POINTLIGHTS_COUNT 100

using namespace ci;
using namespace ci::app;
using namespace std;

// Define a structure for our objects.
typedef struct {
	vec3  position;
	float scale;
	vec3  axis;
	float angle;
	mat4  transform;
} Object;

// Define a structure for our point lights.
typedef struct {
	Sphere sphere;
	vec3   color;
	float  intensity;
} PointLight;

class DeferredLightingApp : public App {
  public:
	DeferredLightingApp();

	void setup() override;
	void update() override;
	void draw() override;

	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;

	void keyDown( KeyEvent event ) override;

	void resize() override;

  private:
	void update( double timestep );

	void reloadShaders();
	void createFramebuffers();

	void bindFramebuffer( const gl::FboRef &fbo ); // Helper function.
	void unbindFramebuffer();                      // Helper function.

  private:
	CameraPersp mCamera;   // Our main camera.
	CameraUi    mCameraUi; // Used to control our main camera.

	gl::VboRef mObjectData; // Our objects instanced data buffer.
	gl::VboRef mLightData;  // Our lights instanced data buffer.

	gl::FboRef mFboNormalsAndDepth; // Full screen buffer for pre-pass. Contains per-pixel normal and depth.
	gl::FboRef mFboLightPrePass;    // Full screen buffer for lighting pass.

	gl::BatchRef mBatchPrePass, mBatchFinalPass;  // Our scene with shaders for pre-pass and final pass.
	gl::BatchRef mPointLights, mPointLightsDebug; // Our point light mesh with shaders for lighting pass and debugging.

	gl::Texture2dRef mLabels; // Label texture.

	std::vector<PointLight> mLights;
	int                     mLightCount;

	AxisAlignedBox      mBounds; // Teapot bounding box, used for frustum culling.
	std::vector<Object> mObjects;
	int                 mObjectCount;

	double mTime;

	bool mIsResized = true; // True if we need to resize our frame buffers.
	bool mIsPaused = false;
	bool mEnableLightCulling = true;
	bool mEnableObjectCulling = true;
	bool mEnableDebug = false;
};

DeferredLightingApp::DeferredLightingApp()
    : mTime( 0 )
    , mLightCount( 0 )
{
	// Setup our main camera.
	mCamera.setPerspective( 35.0f, 1.0f, 0.1f, 1000.0f );
	mCamera.lookAt( vec3( 0, 2, 5 ), vec3( 0 ) );
	mCameraUi.setCamera( &mCamera );
}

void DeferredLightingApp::setup()
{
	// Maximum frame rate.
	disableFrameRate();

	// Label texture.
	mLabels = gl::Texture2d::create( loadImage( loadAsset( "labels.png" ) ) );

	//
	auto glsl = gl::getStockShader( gl::ShaderDef() );

	// Create our scene batch.
	mObjects.reserve( OBJECTS_COUNT );
	mObjects.clear();

	try {
		Rand::randSeed( 12345 );

		std::vector<mat4> matrices;
		for( int x = 0; x < OBJECTS_X; ++x ) {
			for( int y = 0; y < OBJECTS_Y; ++y ) {
				for( int z = 0; z < OBJECTS_Z; ++z ) {
					Object object;
					object.position = vec3( x - OBJECTS_X / 2, y - OBJECTS_Y / 2, z - OBJECTS_Z / 2 ) + 0.5f * Rand::randVec3();
					object.scale = 0.25f;
					object.axis = Rand::randVec3();
					object.angle = Rand::randFloat( -3.14159f, +3.14159f );

					object.transform = glm::translate( object.position );
					object.transform *= glm::scale( vec3( object.scale ) );
					object.transform *= glm::rotate( object.angle, object.axis );

					mObjects.emplace_back( object );
					matrices.emplace_back( object.transform );
				}
			}
		}

		geom::BufferLayout layout;
		layout.append( geom::Attrib::CUSTOM_0, sizeof( mat4 ) / sizeof( float ) /* dims */, sizeof( mat4 ) /* stride */, 0 /* offset */, 1 /* per instance */ );

		mObjectData = gl::Vbo::create( GL_ARRAY_BUFFER, matrices.size() * sizeof( mat4 ), matrices.data(), GL_STATIC_DRAW );

		auto mesh = gl::VboMesh::create( geom::Teapot().subdivisions( 9 ) >> geom::Translate( 0.0f, -0.5f, 0.0f ) >> geom::Bounds( &mBounds ) );
		mesh->appendVbo( layout, mObjectData );

		mBatchPrePass = gl::Batch::create( mesh, glsl, { { geom::CUSTOM_0, "iModelMatrix" } } );
	}
	catch( const std::exception &exc ) {
		console() << exc.what() << std::endl;
	}

	// Create our point lights.
	mLights.clear();
	mLights.reserve( POINTLIGHTS_COUNT );

	try {
		Rand::randSeed( 1 );

		for( int i = 0; i < POINTLIGHTS_COUNT; ++i ) {
			PointLight light;
			light.sphere.setCenter( Rand::randVec3() * Rand::randFloat( 1.5f, 5.0f ) );
			light.color = Color( CM_HSV, Rand::randFloat( 0.0f, 1.0f ), 1.0f, 1.0f );

			// See: https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
			const float kCutoff = 4.0f / 255.0f;
			const float kRadius = 1.0f;
			light.intensity = randFloat( 0.1f, 0.5f );
			light.sphere.setRadius( kRadius * ( glm::sqrt( light.intensity / kCutoff ) - 1.0f ) );

			mLights.emplace_back( light );
		}

		geom::BufferLayout layout;
		layout.append( geom::Attrib::CUSTOM_0, sizeof( vec4 ) / sizeof( float ) /* dims */, sizeof( PointLight ) /* stride */, offsetof( PointLight, sphere ) /* offset */, 1 /* per instance */ );
		layout.append( geom::Attrib::CUSTOM_1, sizeof( vec4 ) / sizeof( float ) /* dims */, sizeof( PointLight ) /* stride */, offsetof( PointLight, color ) /* offset */, 1 /* per instance */ );

		mLightData = gl::Vbo::create( GL_ARRAY_BUFFER, mLights.size() * sizeof( PointLight ), mLights.data(), GL_STATIC_DRAW );

		auto mesh = gl::VboMesh::create( geom::Sphere().radius( 1.0f ) );
		mesh->appendVbo( layout, mLightData );

		mPointLights = gl::Batch::create( mesh, glsl, { { geom::Attrib::CUSTOM_0, "iPositionAndRadius" }, { geom::Attrib::CUSTOM_1, "iColorAndIntensity" } } );

		mesh = gl::VboMesh::create( geom::WireSphere().radius( 1.0f ) );
		mesh->appendVbo( layout, mLightData );

		mPointLightsDebug = gl::Batch::create( mesh, glsl, { { geom::Attrib::CUSTOM_0, "iPositionAndRadius" }, { geom::Attrib::CUSTOM_1, "iColorAndIntensity" } } );
	}
	catch( const std::exception &exc ) {
		console() << exc.what() << std::endl;
	}

	//
	reloadShaders();
}

void DeferredLightingApp::update()
{
	// (Re-)create frame buffers on resize.
	if( mIsResized ) {
		mIsResized = false;

		createFramebuffers();
	}

	// Cull light sources against camera frustum.
	if( mEnableLightCulling ) {
		auto frustum = Frustum( mCamera );

		auto itr = std::begin( mLights );
		auto end = std::end( mLights );
		while( itr != end ) {
			auto &light = *itr;
			if( !frustum.intersects( light.sphere ) ) {
				std::swap( light, *( --end ) );
			}
			else {
				itr++;
			}
		}

		mLightCount = itr - mLights.begin();

		// Update data buffer.
		auto ptr = (PointLight *)mLightData->mapReplace();
		for( int i = 0; i < mLightCount; ++i ) {
			*ptr++ = mLights[i];
		}
		mLightData->unmap();
	}
	else {
		mLightCount = POINTLIGHTS_COUNT;
	}

	// Cull teapots against camera frustum.
	if( mEnableObjectCulling ) {
		auto frustum = Frustum( mCamera );

		auto itr = std::begin( mObjects );
		auto end = std::end( mObjects );
		while( itr != end ) {
			auto &object = *itr;
			if( !frustum.intersects( mBounds.transformed( object.transform ) ) ) {
				std::swap( object, *( --end ) );
			}
			else {
				itr++;
			}
		}

		mObjectCount = itr - mObjects.begin();

		// Update data buffer.
		auto ptr = (mat4 *)mObjectData->mapReplace();
		for( int i = 0; i < mObjectCount; ++i ) {
			*ptr++ = mObjects[i].transform;
		}
		mObjectData->unmap();
	}
	else {
		mObjectCount = OBJECTS_COUNT;

		// Update data buffer.
		auto ptr = (mat4 *)mObjectData->mapReplace();
		for( int i = 0; i < mObjectCount; ++i ) {
			*ptr++ = mObjects[i].transform;
		}
		mObjectData->unmap();
	}

	// Update window title.
	getWindow()->setTitle( std::string( "Objects: " ) + toString( mObjectCount ) + ", Lights: " + toString( mLightCount ) );

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
		update( mIsPaused ? 0.0 : timestep );

		accumulator -= timestep;
	}
}

void DeferredLightingApp::update( double timestep )
{
	mTime += timestep;

	// Animate objects.
	for( auto &object : mObjects ) {
		object.angle += float( timestep );

		object.transform = glm::translate( object.position );
		object.transform *= glm::scale( vec3( object.scale ) );
		object.transform *= glm::rotate( object.angle, object.axis );
	}
}

void DeferredLightingApp::draw()
{
	// Render pre-pass.
	bindFramebuffer( mFboNormalsAndDepth );
	gl::clear();

	if( mBatchPrePass && mObjectCount > 0 ) {

		gl::ScopedDepth       scpDepth( true, GL_LESS );
		gl::ScopedFaceCulling scpFaceCull( true, GL_BACK );

		gl::ScopedMatrices scpMatrices;
		gl::setMatrices( mCamera );

		mBatchPrePass->drawInstanced( mObjectCount );
	}
	unbindFramebuffer();

	// Render point lights.
	bindFramebuffer( mFboLightPrePass );
	gl::clear( GL_COLOR_BUFFER_BIT );

	if( mPointLights && mLightCount > 0 ) {

		gl::ScopedDepthTest     scpDepthTest( true /* true */, GL_GEQUAL /* GL_GEQUAL */ );
		gl::ScopedDepthWrite    scpDepthWrite( false );
		gl::ScopedFaceCulling   scpFaceCull( true /* true */, GL_FRONT /* GL_FRONT */ );
		gl::ScopedBlendAdditive scpBlend;

		gl::ScopedMatrices scpMatrices;
		gl::setMatrices( mCamera );

		gl::ScopedTextureBind scpTexNormals( mFboNormalsAndDepth->getTexture2d( GL_COLOR_ATTACHMENT0 ), 0 );
		gl::ScopedTextureBind scpTexDepth( mFboNormalsAndDepth->getTexture2d( GL_DEPTH_STENCIL_ATTACHMENT ), 1 );

		gl::ScopedGlslProg scpGlsl( mPointLights->getGlslProg() );
		mPointLights->getGlslProg()->uniform( "uInvViewportSize", 1.0f / vec2( mFboLightPrePass->getSize() ) );
		mPointLights->getGlslProg()->uniform( "uNearFarClip", vec2( mCamera.getNearClip(), mCamera.getFarClip() ) );

		mPointLights->drawInstanced( mLightCount );
	}

	// Render point light wire spheres.
	if( mPointLightsDebug && mEnableDebug && mLightCount > 0 ) {
		gl::ScopedDepth         scpDepth( true );
		gl::ScopedBlendAdditive scpBlend;

		gl::ScopedMatrices scpMatrices;
		gl::setMatrices( mCamera );

		mPointLightsDebug->drawInstanced( mLightCount );
	}
	unbindFramebuffer();

	// Clear main buffer and render results.
	gl::clear();

	int w = getWindowWidth();
	int h = getWindowHeight();

	gl::draw( mFboNormalsAndDepth->getColorTexture(), Rectf( 0, 0, w / 2, h / 2 ) ); // Pre-pass.
	gl::draw( mFboLightPrePass->getColorTexture(), Rectf( w / 2, 0, w, h / 2 ) );    // Lighting pass.

	gl::ScopedBlendAlpha scpBlend;
	gl::draw( mLabels, { 0, 0, 256, 64 }, Rectf( 0, 0, 128, 32 ) );
	gl::draw( mLabels, { 0, 64, 256, 128 }, Rectf( w / 2, 0, w / 2 + 128, 32 ) );
}

void DeferredLightingApp::mouseDown( MouseEvent event )
{
	mCameraUi.mouseDown( event );
}

void DeferredLightingApp::mouseDrag( MouseEvent event )
{
	mCameraUi.mouseDrag( event );
}

void DeferredLightingApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	case KeyEvent::KEY_ESCAPE:
		if( isFullScreen() )
			setFullScreen( false );
		else
			quit();
		break;
	case KeyEvent::KEY_SPACE:
		mIsPaused = !mIsPaused;
		break;
	case KeyEvent::KEY_d:
		mEnableDebug = !mEnableDebug;
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_l:
		mEnableLightCulling = !mEnableLightCulling;
		break;
	case KeyEvent::KEY_o:
		mEnableObjectCulling = !mEnableObjectCulling;
		break;
	case KeyEvent::KEY_r:
		reloadShaders();
		break;
	case KeyEvent::KEY_v:
		gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
		break;
	default:
		break;
	}
}

void DeferredLightingApp::resize()
{
	mIsResized = true;

	mCamera.setAspectRatio( getWindowAspectRatio() );
}

void DeferredLightingApp::reloadShaders()
{
	if( mBatchPrePass ) {
		try {
			auto glsl = gl::GlslProg::create( loadAsset( "prepass.vert" ), loadAsset( "prepass.frag" ) );
			mBatchPrePass->replaceGlslProg( glsl );
		}
		catch( const std::exception &exc ) {
			console() << "Failed to load prepass shader: " << exc.what() << std::endl;
		}
	}

	if( mPointLights ) {
		try {
			auto glsl = gl::GlslProg::create( loadAsset( "lighting.vert" ), loadAsset( "pointlight.frag" ) );
			glsl->uniform( "uTexNormals", 0 );
			glsl->uniform( "uTexDepth", 1 );
			mPointLights->replaceGlslProg( glsl );
		}
		catch( const std::exception &exc ) {
			console() << "Failed to load pointlight shader: " << exc.what() << std::endl;
		}
	}

	if( mPointLightsDebug ) {
		try {
			auto fmt = gl::GlslProg::Format();
			fmt.vertex( loadAsset( "lighting.vert" ) );
			fmt.fragment(
			    "#version 150\n"
			    "in vec4 lightPosition;\n"
			    "in vec4 lightColor;\n"
			    "out vec4 fragColor;\n"
			    "void main( void ){\n"
			    "    fragColor.rgb = lightColor.rgb;\n"
			    "    fragColor.a = 1.0;\n"
			    "}" );

			auto glsl = gl::GlslProg::create( fmt );
			mPointLightsDebug->replaceGlslProg( glsl );
		}
		catch( const std::exception &exc ) {
			console() << "Failed to load pointlight debug shader: " << exc.what() << std::endl;
		}
	}
}

void DeferredLightingApp::createFramebuffers()
{
	// We only need a quarter size frame buffer, because we're rendering multiple views to the window.
	int width = getWindowWidth() / 2;
	int height = getWindowHeight() / 2;

	// We'll share the depth buffer among the pre-pass and lighting pass.
	auto depthTexture = gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_DEPTH24_STENCIL8 ).dataType( GL_UNSIGNED_INT_24_8 ) );

	// Pre-pass buffer.
	auto fmt = gl::Fbo::Format().attachment( GL_COLOR_ATTACHMENT0, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_RGB10_A2 ) ) ).attachment( GL_DEPTH_STENCIL_ATTACHMENT, depthTexture );
	mFboNormalsAndDepth = gl::Fbo::create( width, height, fmt );

	// Lighting pass buffer.
	fmt = gl::Fbo::Format().attachment( GL_COLOR_ATTACHMENT0, gl::Texture2d::create( width, height, gl::Texture2d::Format().internalFormat( GL_RGB16F ) ) ).attachment( GL_DEPTH_STENCIL_ATTACHMENT, depthTexture );
	mFboLightPrePass = gl::Fbo::create( width, height, fmt );
}

void DeferredLightingApp::bindFramebuffer( const gl::FboRef &fbo )
{
	auto ctx = gl::context();

	ctx->pushFramebuffer( fbo );
	ctx->pushViewport( std::make_pair( ivec2( 0 ), fbo->getSize() ) );
}

void DeferredLightingApp::unbindFramebuffer()
{
	auto ctx = gl::context();

	ctx->popViewport();
	ctx->popFramebuffer();
}

CINDER_APP( DeferredLightingApp, RendererGl )
