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

#include "Constellations.h"
#include "Conversions.h"

#include "cinder/app/App.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/scoped.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>

using namespace ci;
using namespace ci::app;

Constellations::Constellations( void )
    : mLineWidth( 1.0f )
{
}

Constellations::~Constellations( void ) {}

void Constellations::draw()
{
	if( !mBatch )
		return;

	glLineWidth( mLineWidth );

	gl::ScopedColor         color( Color( 0.5f, 0.6f, 0.8f ) * mAttenuation );
	gl::ScopedBlendAdditive blend;

	mBatch->draw();
}

void Constellations::clear()
{
	mBatch.reset();
	mVertices.clear();
}

void Constellations::setCameraDistance( float distance )
{
	static const float minimum = 0.25f;
	static const float maximum = 1.0f;
	static const float range = 50.0f;

	if( distance > range ) {
		mAttenuation = ci::lerp<float>( minimum, 0.0f, ( distance - range ) / range );
		mAttenuation = math<float>::clamp( mAttenuation, 0.0f, maximum );
	}
	else {
		mAttenuation = math<float>::clamp( 1.0f - math<float>::log10( distance ) / math<float>::log10( range ), minimum, maximum );
	}
}

void Constellations::load( DataSourceRef source )
{
	console() << "Loading constellation database from CSV, please wait..." << std::endl;

	// prepare star database in case this is needed
	std::vector<dvec3> stars;

	// load the database
	std::string constellations = loadString( source );
	std::string adjusted;

	// use boost tokenizer to parse the file
	std::vector<std::string>                     tokens;
	boost::split_iterator<std::string::iterator> lineItr, endItr;
	for( lineItr = boost::make_split_iterator( constellations, boost::token_finder( boost::is_any_of( "\n\r" ) ) ); lineItr != endItr; ++lineItr ) {
		// retrieve a single, trimmed line
		std::string line = boost::algorithm::trim_copy( boost::copy_range<std::string>( *lineItr ) );
		if( line.empty() )
			continue;

		// split into tokens
		boost::algorithm::split( tokens, line, boost::is_any_of( "; \t" ), boost::token_compress_on );

		// skip if data was incomplete
		if( tokens.size() < 4 )
			continue;

		// add coordinate pairs
		if( tokens.size() < 6 ) {
			if( stars.empty() ) {
				console() << "Star distance is missing from constellation database, creating lookup from star database..." << std::endl;
				stars = getStarCoordinates( loadAsset( "hygxyz.csv" ) );
			}

			// distance is missing, look it up in star database
			for( int j = 0; j < 2; ++j ) {
				double ra = Conversions::toDouble( tokens[0 + 2 * j] );
				double dec = Conversions::toDouble( tokens[1 + 2 * j] );
				double distance = 2000.0;
				dvec3  s = getStarCoordinate( ra, dec, 2000.0 );

				// find adjusted star position and distance
				double                             d = 2000.0;
				std::vector<dvec3>::const_iterator i;
				for( i = stars.begin(); i < stars.end(); ++i ) {
					dvec3  c = getStarCoordinate( i->x, i->y, 2000.0 );
					double dist = glm::distance( s, c );
					if( dist < d ) {
						ra = i->x;
						dec = i->y;
						distance = i->z;
						d = dist;
					}
				}

				mVertices.push_back( (vec3)getStarCoordinate( ra, dec, distance ) );

				adjusted.append( ( boost::format( "%.7d;%.7d;%.7d;" ) % ra % dec % distance ).str() );
			}
			adjusted.append( "\r\n" );
		}
		else {
			double ra1 = Conversions::toDouble( tokens[0] );
			double dec1 = Conversions::toDouble( tokens[1] );
			double distance1 = Conversions::toDouble( tokens[2] );

			double ra2 = Conversions::toDouble( tokens[3] );
			double dec2 = Conversions::toDouble( tokens[4] );
			double distance2 = Conversions::toDouble( tokens[5] );

			mVertices.push_back( (vec3)getStarCoordinate( ra1, dec1, distance1 ) );
			mVertices.push_back( (vec3)getStarCoordinate( ra2, dec2, distance2 ) );

			adjusted.append( ( boost::format( "%.7d;%.7d;%.7d;" ) % ra1 % dec1 % distance1 ).str() );
			adjusted.append( ( boost::format( "%.7d;%.7d;%.7d\r\n" ) % ra2 % dec2 % distance2 ).str() );
		}
	}

	//
	// if( ! stars.empty() ) {
	DataTargetPathRef target = writeFile( source->getFilePath().parent_path() / "constellations.cln" );
	OStreamRef        stream = target->getStream();
	stream->write( adjusted );
	//}

	createMesh();
}

void Constellations::read( DataSourceRef source )
{
	IStreamRef in = source->createStream();

	clear();

	uint8_t versionNumber;
	in->read( &versionNumber );

	uint32_t numVertices;
	in->readLittle( &numVertices );

	for( size_t idx = 0; idx < numVertices; ++idx ) {
		vec3 v;
		in->readLittle( &v.x );
		in->readLittle( &v.y );
		in->readLittle( &v.z );
		mVertices.push_back( v );
	}

	// create mesh
	createMesh();
}

void Constellations::write( DataTargetRef target )
{
	OStreamRef out = target->getStream();

	const uint8_t versionNumber = 1;
	out->write( versionNumber );

	out->writeLittle( static_cast<uint32_t>( mVertices.size() ) );

	for( std::vector<vec3>::const_iterator it = mVertices.begin(); it != mVertices.end(); ++it ) {
		out->writeLittle( it->x );
		out->writeLittle( it->y );
		out->writeLittle( it->z );
	}
}

void Constellations::createMesh()
{
	auto vboMesh = gl::VboMesh::create( mVertices.size(), GL_LINES, { gl::VboMesh::Layout().usage( GL_STATIC_DRAW ).attrib( geom::POSITION, 3 ) } );
	vboMesh->bufferAttrib( geom::POSITION, mVertices );

	auto shader = gl::context()->getStockShader( gl::ShaderDef().color() );

	mBatch = gl::Batch::create( vboMesh, shader );
}

dvec3 Constellations::getStarCoordinate( double ra, double dec, double distance )
{
	double alpha = toRadians( ra * 15.0 );
	double delta = toRadians( dec );
	return distance * dvec3( sin( alpha ) * cos( delta ), sin( delta ), cos( alpha ) * cos( delta ) );
}

std::vector<dvec3> Constellations::getStarCoordinates( DataSourceRef source )
{
	std::vector<dvec3> result;

	// load the star database
	std::string stars = loadString( source );

	// use boost tokenizer to parse the file
	std::vector<std::string>                     tokens;
	boost::split_iterator<std::string::iterator> lineItr, endItr;
	for( lineItr = boost::make_split_iterator( stars, boost::token_finder( boost::is_any_of( "\n\r" ) ) ); lineItr != endItr; ++lineItr ) {
		// retrieve a single, trimmed line
		std::string line = boost::algorithm::trim_copy( boost::copy_range<std::string>( *lineItr ) );
		if( line.empty() )
			continue;

		// split into tokens
		boost::algorithm::split( tokens, line, boost::is_any_of( ";" ), boost::token_compress_off );

		// skip if data was incomplete
		if( tokens.size() < 23 )
			continue;

		//
		try {
			// position
			double ra = Conversions::toDouble( tokens[7] );
			double dec = Conversions::toDouble( tokens[8] );
			double distance = Conversions::toDouble( tokens[9] );

			result.push_back( dvec3( ra, dec, distance ) );
		}
		catch( ... ) {
		}
	}

	return result;
}
