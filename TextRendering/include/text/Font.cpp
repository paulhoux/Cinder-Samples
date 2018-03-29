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

#include "text/Font.h"
#include <boost/algorithm/string.hpp>

namespace ph {
namespace text {

using namespace ci;
using namespace ci::app;
using namespace std;

Font::Font( void )
    : mInvalid( true )
    , mFamily( "Unknown" )
    , mFontSize( 12.0f )
    , mLeading( 0.0f )
    , mAscent( 0.0f )
    , mDescent( 0.0f )
    , mSpaceWidth( 0.0f )
{
}

Font::~Font( void ) {}

void Font::create( const ci::DataSourceRef png, const ci::DataSourceRef txt )
{
	// initialize
	mInvalid = true;
	mFamily = "Unknown";
	mFontSize = 12.0f;
	mLeading = 0.0f;
	mAscent = 0.0f;
	mDescent = 0.0f;
	mSpaceWidth = 0.0f;

	mMetrics.clear();

	// try to load the font texture
	try {
		mSurface = ci::Surface( loadImage( png ) );
		mTexture = gl::Texture2d::create( mSurface );
		mTextureSize = mTexture->getSize();
	}
	catch( ... ) {
		throw FontInvalidSourceExc();
	}

	// now that we have the font, let's read the metrics file
	std::string data;
	try {
		data = loadString( txt );
	}
	catch( ... ) {
		throw FontInvalidSourceExc();
	}

	// parse the file
	try {
		// split the file into lines
		std::vector<std::string> lines = ci::split( data, "\n\r" );
		if( lines.size() < 2 )
			throw;

		// read the first line, containing the font name
		std::vector<std::string> tokens = ci::split( lines[0], "=" );
		if( tokens.size() < 2 && tokens[0] != "info face" )
			throw;

		mFamily = boost::algorithm::erase_all_copy( tokens[1], "\"" );

		// read the second containing the number of characters in this font
		tokens = ci::split( lines[1], "=" );
		if( tokens.size() < 2 && tokens[0] != "chars count" )
			throw;

		const int count = std::atoi( tokens[1].c_str() );
		if( count < 1 )
			throw;

		// create the metrics
		uint32_t charcode = 0;
		for( int i = 2; i < count + 2; ++i ) {
			tokens = ci::split( lines[i], " " );
			Metrics m = {};
			for( size_t j = 0; j < tokens.size(); ++j ) {
				std::vector<std::string> kvp = ci::split( tokens[j], "=" );
				if( kvp.size() < 2 )
					continue;

				char *endPtr = nullptr;

				if( kvp[0] == "id" )
					charcode = uint32_t( std::strtoul( kvp[1].c_str(), &endPtr, 0 ) );
				else if( kvp[0] == "x" )
					m.x1 = std::strtof( kvp[1].c_str(), &endPtr );
				else if( kvp[0] == "y" )
					m.y1 = std::strtof( kvp[1].c_str(), &endPtr );
				else if( kvp[0] == "width" )
					m.w = std::strtof( kvp[1].c_str(), &endPtr );
				else if( kvp[0] == "height" )
					m.h = std::strtof( kvp[1].c_str(), &endPtr );
				else if( kvp[0] == "xoffset" )
					m.dx = std::strtof( kvp[1].c_str(), &endPtr );
				else if( kvp[0] == "yoffset" )
					m.dy = std::strtof( kvp[1].c_str(), &endPtr );
				else if( kvp[0] == "xadvance" )
					m.d = std::strtof( kvp[1].c_str(), &endPtr );
			}

			m.x2 = m.x1 + m.w;
			m.y2 = m.y1 + m.h;
			mMetrics[charcode] = m;
		}
	}
	catch( ... ) {
		throw FontInvalidSourceExc();
	}

	// measure font (standard ASCII range only to prevent weird characters influencing the measurements)
	for( uint16_t i = 33; i < 127; ++i ) {
		const MetricsData::const_iterator itr = mMetrics.find( i );
		if( itr != mMetrics.end() ) {
			mAscent = std::max( mAscent, itr->second.dy );
			mDescent = std::max( mDescent, itr->second.h - itr->second.dy );
		}
	}

	mLeading = mAscent + mDescent;
	mFontSize = mAscent + mDescent;

	if( mMetrics.find( 32 ) != mMetrics.end() )
		mSpaceWidth = mMetrics[32].d;
}

void Font::read( const ci::DataSourceRef source )
{
	mInvalid = true;

	IStreamRef   in = source->createStream();
	const size_t filesize = in->size();

	// read header
	uint8_t header;
	in->read( &header );
	if( header != 'S' )
		throw FontInvalidSourceExc();
	in->read( &header );
	if( header != 'D' )
		throw FontInvalidSourceExc();
	in->read( &header );
	if( header != 'F' )
		throw FontInvalidSourceExc();
	in->read( &header );
	if( header != 'F' )
		throw FontInvalidSourceExc();

	uint16_t version;
	in->readLittle( &version );

	// read font name
	if( version > 0x0001 )
		in->read( &mFamily );

	// read font data
	in->readData( static_cast<void *>( &mLeading ), sizeof( mLeading ) );
	in->readData( static_cast<void *>( &mAscent ), sizeof( mAscent ) );
	in->readData( static_cast<void *>( &mDescent ), sizeof( mDescent ) );
	in->readData( static_cast<void *>( &mSpaceWidth ), sizeof( mSpaceWidth ) );
	mFontSize = mAscent + mDescent;

	// read metrics data
	mMetrics.clear();

	try {
		uint16_t count;
		in->readLittle( &count );

		for( int i = 0; i < count; ++i ) {
			uint16_t charcode;
			in->readLittle( &charcode );

			Metrics m;
			in->readData( static_cast<void *>( &( m.x1 ) ), sizeof( m.x1 ) );
			in->readData( static_cast<void *>( &( m.y1 ) ), sizeof( m.y1 ) );
			in->readData( static_cast<void *>( &( m.w ) ), sizeof( m.w ) );
			in->readData( static_cast<void *>( &( m.h ) ), sizeof( m.h ) );

			in->readData( static_cast<void *>( &( m.dx ) ), sizeof( m.dx ) );
			in->readData( static_cast<void *>( &( m.dy ) ), sizeof( m.dy ) );
			in->readData( static_cast<void *>( &( m.d ) ), sizeof( m.d ) );

			m.x2 = m.x1 + m.w;
			m.y2 = m.y1 + m.h;
			mMetrics[charcode] = m;
		}
	}
	catch( ... ) {
		throw FontInvalidSourceExc();
	}

	// read image data
	try {
		// reserve memory
		BufferRef buffer = Buffer::create( filesize );
		// read the remaining data into memory (needs fix from Github's Cinder, won't work with release version)
		size_t bytesRead = in->readDataAvailable( buffer->getData(), filesize );

		// load image
		mSurface = Surface( loadImage( DataSourceBuffer::create( buffer ), ImageSource::Options(), "png" ) );

		// apply mip-mapping and load top-down
		gl::Texture2d::Format fmt;
		fmt.enableMipmapping();
		fmt.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
		fmt.setMagFilter( GL_LINEAR );
		fmt.loadTopDown( true );

		mTexture = gl::Texture2d::create( mSurface, fmt );
		mTextureSize = mTexture->getSize();
	}
	catch( ... ) {
		throw FontInvalidSourceExc();
	}
}

void Font::write( const ci::DataTargetRef target )
{
	if( !target )
		throw FontInvalidTargetExc();

	OStreamRef out = target->getStream();

	// write header
	out->write( uint8_t( 'S' ) );
	out->write( uint8_t( 'D' ) );
	out->write( uint8_t( 'F' ) );
	out->write( uint8_t( 'F' ) );

	const uint16_t version = 0x0002;
	out->writeLittle( version );

	// write font name
	out->write( mFamily );

	// write font data
	out->writeData( static_cast<void *>( &mLeading ), sizeof( mLeading ) );
	out->writeData( static_cast<void *>( &mAscent ), sizeof( mAscent ) );
	out->writeData( static_cast<void *>( &mDescent ), sizeof( mDescent ) );
	out->writeData( static_cast<void *>( &mSpaceWidth ), sizeof( mSpaceWidth ) );

	// write metrics data
	{
		const uint16_t count = uint16_t( mMetrics.size() );
		out->writeLittle( count );

		for( MetricsData::const_iterator itr = mMetrics.begin(); itr != mMetrics.end(); ++itr ) {
			// write char code
			out->writeLittle( itr->first );
			// write metrics
			out->writeData( &( itr->second.x1 ), sizeof( itr->second.x1 ) );
			out->writeData( &( itr->second.y1 ), sizeof( itr->second.y1 ) );
			out->writeData( &( itr->second.w ), sizeof( itr->second.w ) );
			out->writeData( &( itr->second.h ), sizeof( itr->second.h ) );

			out->writeData( &( itr->second.dx ), sizeof( itr->second.dx ) );
			out->writeData( &( itr->second.dy ), sizeof( itr->second.dy ) );
			out->writeData( &( itr->second.d ), sizeof( itr->second.d ) );
		}
	}

	// write image data
	writeImage( DataTargetStream::createRef( out ), mSurface.getChannelRed(), ImageTarget::Options(), "png" );
}

Font::Metrics Font::getMetrics( uint16_t charcode ) const
{
	const MetricsData::const_iterator itr = mMetrics.find( charcode );
	if( itr == mMetrics.end() )
		return Metrics();

	return itr->second;
}

Rectf Font::getBounds( uint16_t charcode, float fontSize ) const
{
	const MetricsData::const_iterator itr = mMetrics.find( charcode );
	if( itr != mMetrics.end() )
		return getBounds( itr->second, fontSize );
	else
		return Rectf();
}

Rectf Font::getBounds( const Metrics &metrics, float fontSize ) const
{
	const float scale = ( fontSize / mFontSize );

	return Rectf( vec2( metrics.dx, -metrics.dy ) * scale, vec2( metrics.dx + metrics.w, metrics.h - metrics.dy ) * scale );
}

Rectf Font::getTexCoords( uint16_t charcode ) const
{
	const MetricsData::const_iterator itr = mMetrics.find( charcode );
	if( itr != mMetrics.end() )
		return getTexCoords( itr->second );
	else
		return Rectf();
}

Rectf Font::getTexCoords( const Metrics &metrics ) const
{
	return Rectf( vec2( metrics.x1, metrics.y1 ) / mTextureSize, vec2( metrics.x2, metrics.y2 ) / mTextureSize );
}

float Font::getAdvance( uint16_t charcode, float fontSize ) const
{
	const MetricsData::const_iterator itr = mMetrics.find( charcode );
	if( itr != mMetrics.end() )
		return getAdvance( itr->second, fontSize );

	return 0.0f;
}

float Font::getAdvance( const Metrics &metrics, float fontSize ) const
{
	return metrics.d * fontSize / mFontSize;
}

Rectf Font::measure( const std::u16string &text, float fontSize ) const
{
	float offset = 0.0f;
	Rectf result( 0.0f, 0.0f, 0.0f, 0.0f );

	for( std::u16string::const_iterator citr = text.begin(); citr != text.end(); ++citr ) {
		uint16_t charcode = uint16_t( *citr );

		// TODO: handle special chars like /t

		const MetricsData::const_iterator itr = mMetrics.find( charcode );
		if( itr != mMetrics.end() ) {
			result.include( Rectf( offset + itr->second.dx, -itr->second.dy, offset + itr->second.dx + itr->second.w, itr->second.h - itr->second.dy ) );
			offset += itr->second.d;
		}
	}

	// return
	return result.scaled( fontSize / mFontSize );
}

float Font::measureWidth( const std::u16string &text, float fontSize, bool precise ) const
{
	float offset = 0.0f;
	float adjust = 0.0f;

	for( std::u16string::const_iterator citr = text.begin(); citr != text.end(); ++citr ) {
		uint16_t charcode = uint16_t( *citr );

		// TODO: handle special chars like /t

		const MetricsData::const_iterator itr = mMetrics.find( charcode );
		if( itr != mMetrics.end() ) {
			offset += itr->second.d;

			// precise measurement takes into account that the last character
			// contributes to the total width only by its own width, not its advance
			if( precise )
				adjust = itr->second.dx + itr->second.w - itr->second.d;
		}
	}

	return ( offset + adjust ) * ( fontSize / mFontSize );
}
} // namespace text
} // namespace ph
