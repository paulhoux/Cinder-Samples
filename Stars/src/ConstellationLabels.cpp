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

#include "ConstellationLabels.h"
#include "Conversions.h"

#include "text/FontStore.h"

#include "cinder/app/App.h"

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

using namespace ci;
using namespace ci::app;
using namespace ph;

ConstellationLabels::ConstellationLabels( void ) {}

ConstellationLabels::~ConstellationLabels( void ) {}

void ConstellationLabels::setup()
{
	// intialize labels
	text::fonts().loadFont( loadAsset( "fonts/Ubuntu-BoldItalic.sdff" ) );
	mLabels.setFont( text::fonts().getFont( "Ubuntu-BoldItalic" ) );
	mLabels.setFontSize( 16.0f );
	mLabels.setBoundary( text::Text::LINE );
	// mLabels.setOffset( 2.5f, 2.5f );

	double alpha = toRadians( 17.76112222 * 15.0 );
	double delta = toRadians( -29.00780555 );
	vec3   position = 8330.0f * vec3( (float)( sin( alpha ) * cos( delta ) ), (float)sin( delta ), (float)( cos( alpha ) * cos( delta ) ) );

	mLabels.addLabel( position, "Center of the Galaxy" );
}

void ConstellationLabels::draw()
{
	// glPushAttrib( GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT );
	gl::enableAdditiveBlending();
	gl::color( Color( 0.5f, 0.6f, 0.8f ) * mAttenuation );

	mLabels.draw();

	// glPopAttrib();
}

void ConstellationLabels::setCameraDistance( float distance )
{
	static const float minimum = 0.25f;
	static const float maximum = 1.0f;
	static const float range = 10.0f;

	if( distance > range ) {
		mAttenuation = ci::lerp<float>( minimum, 0.0f, ( distance - range ) / range );
		mAttenuation = math<float>::clamp( mAttenuation, 0.0f, maximum );
	}
	else {
		mAttenuation = math<float>::clamp( 1.0f - math<float>::log10( distance ) / math<float>::log10( range ), minimum, maximum );
	}
}

void ConstellationLabels::load( DataSourceRef source )
{
	console() << "Loading constellation label database from CSV, please wait..." << std::endl;

	mLabels.clear();

	// load the star database
	std::string names = loadString( source );

	// use boost tokenizer to parse the file
	std::vector<std::string>                     tokens;
	boost::split_iterator<std::string::iterator> lineItr, endItr;
	for( lineItr = boost::make_split_iterator( names, boost::token_finder( boost::is_any_of( "\n\r" ) ) ); lineItr != endItr; ++lineItr ) {
		// retrieve a single, trimmed line
		std::string line = boost::algorithm::trim_copy( boost::copy_range<std::string>( *lineItr ) );
		if( line.substr( 0, 1 ) == ";" )
			continue;
		if( line.empty() )
			continue;

		// split into tokens
		boost::algorithm::split( tokens, line, boost::is_any_of( ";" ), boost::token_compress_off );

		// skip if data was incomplete
		if( tokens.size() < 4 )
			continue;

		//
		try {
			// name
			std::string name = boost::trim_copy( tokens[3] );
			if( name.empty() )
				continue;

			// position
			double ra = Conversions::toDouble( tokens[0] );
			double dec = Conversions::toDouble( tokens[1] );

			double alpha = toRadians( ra * 15.0 );
			double delta = toRadians( dec );

			vec3 position = 2000.0f * vec3( (float)( sin( alpha ) * cos( delta ) ), (float)sin( delta ), (float)( cos( alpha ) * cos( delta ) ) );

			mLabels.addLabel( position, name );
		}
		catch( ... ) {
			// some of the data was invalid, ignore
			continue;
		}
	}
}
