#include "Background.h"
#include "Conversions.h"

#include "cinder/ImageIo.h"
#include "cinder/app/AppBasic.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// define constants used in the coordinate conversion methods
const Vec3d	Background::GALACTIC_CENTER_EQUATORIAL = Vec3d(toRadians(266.40510), toRadians(-28.936175), 8.33);
const Vec2d	Background::GALACTIC_NORTHPOLE_EQUATORIAL = Vec2d(toRadians(192.859508), toRadians(27.128336));

Background::Background(void)
	: mAttenuation(1.0f)
{
	// roughly convert galactic coordinates of map to equatorial coordinates of stars
	// by rotating the sphere on which the map is projected. The rotation angles were
	// found using a celestial reference map.
	//mRotation = Vec3f(-60.3f, 81.9f, 22.0f);
	mRotation = Vec3f(-60.25f, 83.45f, 23.25f);

	mTransform.rotate( mRotation * Vec3f::zAxis() ); // rotate z-axis
	mTransform.rotate( mRotation * Vec3f::xAxis() ); // rotate x-axis
	mTransform.rotate( mRotation * Vec3f::yAxis() ); // rotate y-axis

	//mTransform.setColumn(0, Vec4f( -0.054875539, -0.873437105, -0.483834992, 0.0 ));
	//mTransform.setColumn(1, Vec4f( 0.494109454, -0.444829594, 0.746982249, 0.0 ));
	//mTransform.setColumn(2, Vec4f( -0.867666136, -0.198076390, 0.455983795, 0.0 ));
}

Background::~Background(void)
{
}

void Background::setup()
{
	try { mTexture = gl::Texture( loadImage( loadAsset("textures/background.jpg") ) ); }
	catch( const std::exception &e ) { console() << "Could not load texture: " << e.what() << std::endl; }

	create();
}

void Background::draw()
{
	if(!( mTexture && mVboMesh )) return;

	glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT );

	mTexture.enableAndBind();

	gl::pushModelView();
	{
		Matrix44f m = gl::getModelView();
		gl::rotate( mRotation * Vec3f::zAxis() ); // rotate z-axis
		gl::rotate( mRotation * Vec3f::xAxis() ); // rotate x-axis
		gl::rotate( mRotation * Vec3f::yAxis() ); // rotate y-axis
		//gl::multModelView( mTransform );

		gl::color( mAttenuation * Color::white() );
		gl::draw( mVboMesh );
	}
	gl::popModelView();

	glPopAttrib();
}


void Background::create()
{
	const double	TWO_PI = 2.0 * M_PI;
	const double	HALF_PI = 0.5 * M_PI;

	const int		SLICES = 30;
	const int		SEGMENTS = 60;
	const int		RADIUS = 2000;

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
			double phi = (0.5 - static_cast<double>(y) / SLICES) * M_PI;

			normals.push_back( Vec3f(
				static_cast<float>( cos(phi) * sin(theta) ),
				static_cast<float>( sin(phi) ),
				static_cast<float>( cos(phi) * cos(theta) ) ) );

			positions.push_back( normals.back() * RADIUS );	

			float tx = 1.0f - static_cast<float>(x) / SEGMENTS;
			float ty = static_cast<float>(y) / SLICES;

			texCoords.push_back( Vec2f(tx, ty) );
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
	static const float minimum = 0.01f;
	static const float maximum = 1.0f;

	if( distance > 300.0f ) {
		mAttenuation = ci::lerp<float>( minimum, 0.0f, (distance - 300.0f) / 200.0f );
		mAttenuation = math<float>::clamp( mAttenuation, 0.0f, maximum );
	}
	else {
		mAttenuation = math<float>::clamp( 1.0f - math<float>::log10( distance ) / math<float>::log10(100.0f), minimum, maximum );
	}
}

Vec2d Background::toEquatorial( const Vec2d &radians )
{	
	double longitude = radians.x;	// galactic longitude
	double latitude = radians.y;	// galactic latitude
	
	double alpha = GALACTIC_NORTHPOLE_EQUATORIAL.x;
	double delta = GALACTIC_NORTHPOLE_EQUATORIAL.y;
	double la = toRadians(33.0);

	double dec = asin(sin(latitude) * sin(delta) + cos(latitude) * cos(delta) * sin(longitude - la));
	double ra = atan2(cos(latitude) * cos(longitude - la), sin(latitude) * cos(delta) - cos(latitude) * sin(delta) * sin(longitude - la)) + alpha;
	
	ra = Conversions::wrap(ra, 0.0, 2.0 * M_PI);

	return Vec2d(ra, dec);
}

Vec2d Background::toGalactic( const Vec2d &radians )
{
	double ra = radians.x;
	double dec = radians.y;

	double alpha = GALACTIC_NORTHPOLE_EQUATORIAL.x;
	double delta = GALACTIC_NORTHPOLE_EQUATORIAL.y;
	double la = toRadians(33.0);
    
    double latitude = asin(sin(dec) * sin(delta) + cos(dec) * cos(delta) * cos(ra - alpha));
    double longitude = atan2(sin(dec) * cos(delta) - cos(dec) * sin(delta) * cos(ra - alpha), cos(dec) * sin(ra - alpha)) + la;

	longitude = Conversions::wrap(longitude, 0.0, 2.0 * M_PI);

	return Vec2d(longitude, latitude);
}
