/*
 Copyright (c) 2010-2012, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this list of conditions and
 the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "Stars.h"
#include "Conversions.h"

#include "cinder/ImageIo.h"
#include "cinder/app/App.h"
#include "cinder/gl/scoped.h"

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

using namespace ci;
using namespace ci::app;
using namespace std;

Stars::Stars( void )
    : mAspectRatio( 1.0f )
    , mEnableStars( true )
    , mEnableHalos( true )
{
}

Stars::~Stars( void ) {}

void Stars::setup()
{
	// load shader and point sprite texture
	try {
		mShaderStars = gl::GlslProg::create( loadAsset( "shaders/stars.vert" ), loadAsset( "shaders/stars.frag" ) );
		mShaderHalos = gl::GlslProg::create( loadAsset( "shaders/halos.vert" ), loadAsset( "shaders/halos.frag" ) );
	}
	catch( const std::exception &e ) {
		console() << "Could not load & compile shader: " << e.what() << std::endl;
	}

	try {
		mTextureStar = gl::Texture2d::create( loadImage( loadAsset( "textures/star_glow.png" ) ) );
		mTextureCorona = gl::Texture2d::create( loadImage( loadAsset( "textures/nova.png" ) ) );
		mTextureHalo = gl::Texture2d::create( loadImage( loadAsset( "textures/corona.png" ) ) );
	}
	catch( const std::exception &e ) {
		console() << "Could not load texture: " << e.what() << std::endl;
	}
}

void Stars::draw()
{
	enablePointSprites();

	gl::ScopedBlendAdditive blend;
	gl::ScopedColor         color( Color::white() );

	if( mEnableStars && mTextureStar && mTextureCorona && mBatchStars ) {
		gl::ScopedTextureBind tex0( mTextureStar, (uint8_t)0 );
		gl::ScopedTextureBind tex1( mTextureCorona, (uint8_t)1 );
		mBatchStars->draw();
	}
	if( mEnableHalos && mTextureHalo && mBatchHalos ) {
		gl::ScopedTextureBind tex0( mTextureHalo, (uint8_t)0 );
		mBatchHalos->draw();
	}

	disablePointSprites();
}

void Stars::resize( const ivec2 &size )
{
	// adjust size based on the resolution
	mScale = size.x / 1280.0f;

	// point sprite sizes differ between ATI/AMD and NVIDIA GPU's,
	// ATI's are twice as large and have to be scaled down
	std::string vendor = std::string( (char *)glGetString( GL_VENDOR ) );

	if( vendor == "ATI Technologies Inc." )
		mScale *= 0.5f;
}

void Stars::clear()
{
	mVertices.clear();
	mTexcoords.clear();
	mColors.clear();
}

void Stars::enablePointSprites()
{
	// enable point sprites and initialize it
	gl::enable( GL_POINT_SPRITE_ARB );
	glPointParameterfARB( GL_POINT_FADE_THRESHOLD_SIZE_ARB, 1.0f );
	glPointParameterfARB( GL_POINT_SIZE_MIN_ARB, 0.1f );
	glPointParameterfARB( GL_POINT_SIZE_MAX_ARB, 2000.0f );

	// allow vertex shader to change point size
	gl::enable( GL_VERTEX_PROGRAM_POINT_SIZE );

	// set shader uniforms
	mShaderStars->bind();
	mShaderStars->uniform( "tex0", 0 );
	mShaderStars->uniform( "tex1", 1 );
	mShaderStars->uniform( "time", (float)getElapsedSeconds() );
	mShaderStars->uniform( "aspect", mAspectRatio );
	mShaderStars->uniform( "scale", mScale );

	mShaderHalos->bind();
	mShaderHalos->uniform( "tex0", 0 );
	mShaderHalos->uniform( "time", (float)getElapsedSeconds() );
	mShaderHalos->uniform( "aspect", mAspectRatio );
	mShaderHalos->uniform( "scale", mScale );
}

void Stars::disablePointSprites()
{
	gl::disable( GL_VERTEX_PROGRAM_POINT_SIZE );
	gl::disable( GL_POINT_SPRITE_ARB );
}

void Stars::load( DataSourceRef source )
{
	console() << "Loading star database from CSV, please wait..." << std::endl;

	// create color look up table
	//  see: http://www.vendian.org/mncharity/dir3/starcolor/details.html
	std::vector<ColorA> lookup( 49 );
	lookup[0] = Conversions::toColorA( 0xff9bb2ff );
	lookup[1] = Conversions::toColorA( 0xff9eb5ff );
	lookup[2] = Conversions::toColorA( 0xffa3b9ff );
	lookup[3] = Conversions::toColorA( 0xffaabfff );
	lookup[4] = Conversions::toColorA( 0xffb2c5ff );
	lookup[5] = Conversions::toColorA( 0xffbbccff );
	lookup[6] = Conversions::toColorA( 0xffc4d2ff );
	lookup[7] = Conversions::toColorA( 0xffccd8ff );
	lookup[8] = Conversions::toColorA( 0xffd3ddff );
	lookup[9] = Conversions::toColorA( 0xffdae2ff );
	lookup[10] = Conversions::toColorA( 0xffdfe5ff );
	lookup[11] = Conversions::toColorA( 0xffe4e9ff );
	lookup[12] = Conversions::toColorA( 0xffe9ecff );

	lookup[13] = Conversions::toColorA( 0xffeeefff );
	lookup[14] = Conversions::toColorA( 0xfff3f2ff );
	lookup[15] = Conversions::toColorA( 0xfff8f6ff );
	lookup[16] = Conversions::toColorA( 0xfffef9ff );
	lookup[17] = Conversions::toColorA( 0xfffff9fb );
	lookup[18] = Conversions::toColorA( 0xfffff7f5 );
	lookup[19] = Conversions::toColorA( 0xfffff5ef );
	lookup[20] = Conversions::toColorA( 0xfffff3ea );
	lookup[21] = Conversions::toColorA( 0xfffff1e5 );
	lookup[22] = Conversions::toColorA( 0xffffefe0 );
	lookup[23] = Conversions::toColorA( 0xffffeddb );
	lookup[24] = Conversions::toColorA( 0xffffebd6 );
	lookup[25] = Conversions::toColorA( 0xffffe9d2 );

	lookup[26] = Conversions::toColorA( 0xffffe8ce );
	lookup[27] = Conversions::toColorA( 0xffffe6ca );
	lookup[28] = Conversions::toColorA( 0xffffe5c6 );
	lookup[29] = Conversions::toColorA( 0xffffe3c3 );
	lookup[30] = Conversions::toColorA( 0xffffe2bf );
	lookup[31] = Conversions::toColorA( 0xffffe0bb );
	lookup[32] = Conversions::toColorA( 0xffffdfb8 );
	lookup[33] = Conversions::toColorA( 0xffffddb4 );
	lookup[34] = Conversions::toColorA( 0xffffdbb0 );
	lookup[35] = Conversions::toColorA( 0xffffdaad );
	lookup[36] = Conversions::toColorA( 0xffffd8a9 );
	lookup[37] = Conversions::toColorA( 0xffffd6a5 );
	lookup[38] = Conversions::toColorA( 0xffffd5a1 );

	lookup[39] = Conversions::toColorA( 0xffffd29c );
	lookup[40] = Conversions::toColorA( 0xffffd096 );
	lookup[41] = Conversions::toColorA( 0xffffcc8f );
	lookup[42] = Conversions::toColorA( 0xffffc885 );
	lookup[43] = Conversions::toColorA( 0xffffc178 );
	lookup[44] = Conversions::toColorA( 0xffffb765 );
	lookup[45] = Conversions::toColorA( 0xffffa94b );
	lookup[46] = Conversions::toColorA( 0xffff9523 );
	lookup[47] = Conversions::toColorA( 0xffff7b00 );
	lookup[48] = Conversions::toColorA( 0xffff5200 );

	// create empty buffers for the data
	clear();

	// load the star database
	std::string stars = loadString( source );

	// parse the file
	auto entries = ci::split( stars, "\n\r", true );
	for( auto &entry : entries ) {
		// retrieve a single, trimmed line
		std::string line = boost::algorithm::trim_copy( boost::copy_range<std::string>( entry ) );
		if( line.empty() )
			continue;

		// split into tokens
		auto tokens = ci::split( line, ";", false );

		// skip if data was incomplete
		if( tokens.size() < 23 )
			continue;

		//
		try {
			// absolute magnitude of the star
			double abs_mag = Conversions::toDouble( tokens[14] );

			// color (spectrum) of the star
			double colorindex = ( tokens.size() > 16 ) ? Conversions::toDouble( tokens[16] ) : 0.0;
			double colorlut = ( colorindex + 0.40 ) / 0.05;

			uint32_t index = math<uint32_t>::clamp( (uint32_t)colorlut, 0, 48 );
			uint32_t next_index = math<uint32_t>::clamp( (uint32_t)colorlut + 1, 0, 48 );
			float    t = math<float>::clamp( (float)colorlut - index, 0.0f, 1.0f );

			ColorA color = ( 1.0f - t ) * lookup[index] + t * lookup[next_index];

			// position
			double ra = Conversions::toDouble( tokens[7] );
			double dec = Conversions::toDouble( tokens[8] );
			double distance = Conversions::toDouble( tokens[9] );

			double alpha = toRadians( ra * 15.0 );
			double delta = toRadians( dec );

			// convert to world (universe) coordinates
			mVertices.push_back( vec3( distance * dvec3( (float)( sin( alpha ) * cos( delta ) ), (float)sin( delta ), (float)( cos( alpha ) * cos( delta ) ) ) ) );
			// put extra data (absolute magnitude and distance to Earth) in texture coordinates
			mTexcoords.push_back( vec2( (float)abs_mag, (float)distance ) );
			// put color in color attribute
			mColors.push_back( color );
		}
		catch( ... ) {
			// some of the data was invalid, ignore
			continue;
		}
	}

	// create VboMesh
	createMesh();
}

void Stars::read( DataSourceRef source )
{
	IStreamRef in = source->createStream();

	clear();

	uint8_t versionNumber;
	in->read( &versionNumber );

	uint32_t numVertices, numTexcoords, numColors;
	in->readLittle( &numVertices );
	in->readLittle( &numTexcoords );
	in->readLittle( &numColors );

	for( size_t idx = 0; idx < numVertices; ++idx ) {
		vec3 v;
		in->readLittle( &v.x );
		in->readLittle( &v.y );
		in->readLittle( &v.z );
		mVertices.push_back( v );
	}

	for( size_t idx = 0; idx < numTexcoords; ++idx ) {
		vec2 v;
		in->readLittle( &v.x );
		in->readLittle( &v.y );
		mTexcoords.push_back( v );
	}

	for( size_t idx = 0; idx < numColors; ++idx ) {
		Color v;
		in->readLittle( &v.r );
		in->readLittle( &v.g );
		in->readLittle( &v.b );
		mColors.push_back( v );
	}

	// create VboMesh
	createMesh();
}

void Stars::write( DataTargetRef target )
{
	OStreamRef out = target->getStream();

	const uint8_t versionNumber = 1;
	out->write( versionNumber );

	out->writeLittle( static_cast<uint32_t>( mVertices.size() ) );
	out->writeLittle( static_cast<uint32_t>( mTexcoords.size() ) );
	out->writeLittle( static_cast<uint32_t>( mColors.size() ) );

	for( vector<vec3>::const_iterator it = mVertices.begin(); it != mVertices.end(); ++it ) {
		out->writeLittle( it->x );
		out->writeLittle( it->y );
		out->writeLittle( it->z );
	}

	for( vector<vec2>::const_iterator it = mTexcoords.begin(); it != mTexcoords.end(); ++it ) {
		out->writeLittle( it->x );
		out->writeLittle( it->y );
	}

	for( vector<Color>::const_iterator it = mColors.begin(); it != mColors.end(); ++it ) {
		out->writeLittle( it->r );
		out->writeLittle( it->g );
		out->writeLittle( it->b );
	}
}

void Stars::createMesh()
{
	// create the batch
	auto vboMesh = gl::VboMesh::create( mVertices.size(), GL_POINTS, { gl::VboMesh::Layout().usage( GL_STATIC_DRAW ).attrib( geom::POSITION, 3 ).attrib( geom::TEX_COORD_0, 2 ).attrib( geom::COLOR, 3 ) } );
	vboMesh->bufferAttrib( geom::POSITION, mVertices );
	vboMesh->bufferAttrib( geom::TEX_COORD_0, mTexcoords );
	vboMesh->bufferAttrib( geom::COLOR, mColors );

	mBatchStars = gl::Batch::create( vboMesh, mShaderStars );
	mBatchHalos = gl::Batch::create( vboMesh, mShaderHalos );
}
