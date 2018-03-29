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

#include "Labels.h"
#include "Conversions.h"

#include "text/FontStore.h"

#include "cinder/app/App.h"

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

using namespace ci;
using namespace ci::app;
using namespace ph;

Labels::Labels( void )
    : mAttenuation( 1.0f )
{
}

Labels::~Labels( void ) {}

void Labels::setup()
{
	// intialize labels
	text::fonts().loadFont( loadAsset( "fonts/Ubuntu-BoldItalic.sdff" ) );
	mLabels.setFont( text::fonts().getFont( "Ubuntu-BoldItalic" ) );
	mLabels.setFontSize( 16.0f );
	mLabels.setBoundary( text::Text::LINE );
	// mLabels.setOffset( 2.5f, 2.5f );

	try {
		auto fmt = gl::GlslProg::Format().vertex( loadAsset( "shaders/labels.vert" ) ).fragment( mLabels.getFragmentShader() );
		auto shader = gl::GlslProg::create( fmt );
		mLabels.setShader( shader );
	}
	catch( const std::exception &exc ) {
		console() << exc.what() << std::endl;
	}
}

void Labels::draw()
{
	// glPushAttrib( GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT );

	gl::enableAdditiveBlending();
	gl::color( Color::white() * mAttenuation );

	mLabels.draw();

	// glPopAttrib();
}

void Labels::setCameraDistance( float distance )
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

void Labels::load( DataSourceRef source )
{
	console() << "Loading label database from CSV, please wait..." << std::endl;

	mLabels.clear();

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
			// name
			std::string name = boost::trim_copy( tokens[6] );
			// if( name.empty() ) name = boost::trim_copy( tokens[5] );
			// if( name.empty() ) name = boost::trim_copy( tokens[4] );
			if( name.empty() )
				continue;

			// position
			double ra = Conversions::toDouble( tokens[7] );
			double dec = Conversions::toDouble( tokens[8] );
			float  distance = Conversions::toFloat( tokens[9] );

			// absolute magnitude of the star
			double abs_mag = Conversions::toDouble( tokens[14] );

			double alpha = toRadians( ra * 15.0 );
			double delta = toRadians( dec );

			vec3 position = distance * vec3( (float)( sin( alpha ) * cos( delta ) ), (float)sin( delta ), (float)( cos( alpha ) * cos( delta ) ) );

			mLabels.addLabel( position, name, abs_mag );
		}
		catch( ... ) {
			// some of the data was invalid, ignore
			continue;
		}
	}
}

void Labels::read( DataSourceRef source )
{
	IStreamRef in = source->createStream();

	mLabels.clear();

	uint8_t versionNumber;
	in->read( &versionNumber );

	uint32_t numLabels;
	in->readLittle( &numLabels );

	float data = 0.0f;
	for( size_t idx = 0; idx < numLabels; ++idx ) {
		vec3 position;
		in->readLittle( &position.x );
		in->readLittle( &position.y );
		in->readLittle( &position.z );
		if( versionNumber > 1 )
			in->readLittle( &data );
		std::string name;
		in->read( &name );

		mLabels.addLabel( position, name, data );
	}
}

void Labels::write( DataTargetRef target )
{
	OStreamRef out = target->getStream();

	const uint8_t versionNumber = 2;
	out->write( versionNumber );

	out->writeLittle( static_cast<uint32_t>( mLabels.size() ) );

	for( text::TextLabelListConstIter it = mLabels.begin(); it != mLabels.end(); ++it ) {
		vec4 position = it->first;
		out->writeLittle( position.x );
		out->writeLittle( position.y );
		out->writeLittle( position.z );
		out->writeLittle( position.w );
		std::string name = toUtf8( it->second );
		out->write( name );
	}
}