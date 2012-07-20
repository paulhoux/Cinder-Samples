#include "Background.h"

#include "cinder/ImageIo.h"
#include "cinder/app/AppBasic.h"

using namespace ci;
using namespace ci::app;
using namespace std;

Background::Background(void)
	: mAttenuation(1.0f)
{
	// rough estimation of the earth's rotation with respect to the background map
	mRotation = Vec3f(-23.4393f, -61.5f, 83.0f);
}


Background::~Background(void)
{
}

void Background::setup()
{
	try { mTexture = gl::Texture( loadImage( loadAsset("background.jpg") ) ); }
	catch( const std::exception &e ) { console() << e.what() << std::endl; }

	create();
}

void Background::draw()
{
	glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT );

	mTexture.enableAndBind();

	gl::color( mAttenuation * Color::white() );

	gl::pushModelView();

	gl::rotate( Vec3f(0.0f, -90.0f, 0.0f) );
	gl::rotate( Vec3f(0.0f, 0.0f, mRotation.z) );
	gl::rotate( Vec3f(0.0f, mRotation.y, 0.0f) );
	gl::rotate( Vec3f(mRotation.x, 0.0f, 0.0f) );

	gl::draw( mVboMesh );

	gl::popModelView();

	glPopAttrib();
}


void Background::create()
{
	const double	TWO_PI = 2.0 * M_PI;
	const double	HALF_PI = 0.5 * M_PI;

	const int		SLICES = 30;
	const int		SEGMENTS = 60;
	const int		RADIUS = 3000;

	// create data buffers
	vector<Vec3f>		normals;
	vector<Vec3f>		positions;
	vector<Vec2f>		texCoords;
	vector<uint32_t>	indices;

	//	
	int x, y;
	for(x=0;x<=SEGMENTS;++x) {
		double theta = static_cast<double>(x) / SEGMENTS * TWO_PI;

		for(y=0;y<=SLICES;++y) {
			double phi = static_cast<double>(y) / SLICES * M_PI;

			normals.push_back( Vec3f(
				static_cast<float>( sin(phi) * cos(theta) ),
				static_cast<float>( cos(phi) ),
				static_cast<float>( sin(phi) * sin(theta) ) ) );

			positions.push_back( normals.back() * RADIUS );

			texCoords.push_back( Vec2f(
				1.0f - static_cast<float>(x) / SEGMENTS,
				static_cast<float>(y) / SLICES ) );
		}
	}

	//
	int rings = SLICES+1;
	bool forward = false;
	for(x=0;x<SEGMENTS;++x) {
        if(forward) {
			// create jumps in the triangle strip by introducing degenerate polygons
			indices.push_back(  x      * rings + 0 );
			indices.push_back(  x      * rings + 0 );
			// 
            for(y=0;y<rings;++y) {
                indices.push_back(  x      * rings + y );
                indices.push_back( (x + 1) * rings + y );
            }
        }
        else {
			// create jumps in the triangle strip by introducing degenerate polygons
			indices.push_back( (x + 1) * rings + SLICES );
			indices.push_back( (x + 1) * rings + SLICES );
			// 
            for(y=SLICES;y>=0;--y) {
                indices.push_back( (x + 1) * rings + y );
                indices.push_back(  x      * rings + y );
            }
        }

        forward = !forward;
    }

	// create the mesh
	gl::VboMesh::Layout layout;	
	layout.setStaticIndices();
	layout.setStaticPositions();
	layout.setStaticTexCoords2d();
	layout.setStaticNormals();

	mVboMesh = gl::VboMesh(positions.size(), indices.size(), layout, GL_TRIANGLE_STRIP);

	mVboMesh.bufferNormals( normals );
	mVboMesh.bufferTexCoords2d( 0, texCoords );
	mVboMesh.bufferPositions( positions );
	mVboMesh.bufferIndices( indices );
}

void Background::setCameraDistance( float distance )
{
	static const float minimum = 0.03f;
	static const float maximum = 0.2f;

	if( distance > 1500.0f ) {
		mAttenuation = ci::lerp<float>( minimum, 0.0f, (distance - 1500.0f) / 250.0f );
		mAttenuation = math<float>::clamp( mAttenuation, 0.0f, maximum );
	}
	else {
		mAttenuation = math<float>::clamp( 1.0f - math<float>::log10( distance ) / math<float>::log10(100.0f), minimum, maximum );
	}

}
