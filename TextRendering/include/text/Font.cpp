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

namespace ph { namespace text {

using namespace ci;
using namespace ci::app;
using namespace std;

Font::Font(void)
	: mInvalid(true), mFamily("Unknown"), mFontSize(12.0f), mLeading(0.0f), 
		mAscent(0.0f), mDescent(0.0f), mSpaceWidth(0.0f)
{
}

Font::~Font(void)
{
}

void Font::create(const ci::DataSourceRef png, const ci::DataSourceRef txt)
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
		mTexture = gl::Texture( mSurface ); 
	}
	catch( ... ) { throw FontInvalidSourceExc();	}

	// now that we have the font, let's read the metrics file
	std::string	data;
	try { data = loadString( txt ); }
	catch( ... ) { throw FontInvalidSourceExc();	}

	// parse the file
	try {
		// split the file into lines
		std::vector<std::string> lines = ci::split(data, "\n\r");
		if(lines.size() < 2) throw;

		// read the first line, containing the font name
		std::vector<std::string> tokens = ci::split(lines[0], "=");
		if(tokens.size() < 2 && tokens[0] != "info face") throw;

		mFamily = boost::algorithm::erase_all_copy( tokens[1], "\"" );

		// read the second containing the number of characters in this font
		tokens = ci::split(lines[1], "=");
		if(tokens.size() < 2 && tokens[0] != "chars count") throw;

		int count = boost::lexical_cast<int>( tokens[1] );
		if(count < 1) throw;

		// create the metrics
		uint32_t id = 0;
		for(int i=2;i<count+2;++i) {
			tokens = ci::split(lines[i], " ");
			Metrics m;
			for(size_t j=0;j<tokens.size();++j) {
				std::vector<std::string> kvp = ci::split(tokens[j], "=");
				if(kvp.size() < 2) continue;

				if(kvp[0] == "id")
					id = boost::lexical_cast<uint32_t>( kvp[1] );
				else if(kvp[0] == "x")
					m.x = boost::lexical_cast<float>( kvp[1] );
				else if(kvp[0] == "y")
					m.y = boost::lexical_cast<float>( kvp[1] );
				else if(kvp[0] == "width")
					m.w = boost::lexical_cast<float>( kvp[1] );
				else if(kvp[0] == "height")
					m.h = boost::lexical_cast<float>( kvp[1] );
				else if(kvp[0] == "xoffset")
					m.dx = boost::lexical_cast<float>( kvp[1] );
				else if(kvp[0] == "yoffset")
					m.dy = boost::lexical_cast<float>( kvp[1] );
				else if(kvp[0] == "xadvance")
					m.d = boost::lexical_cast<float>( kvp[1] );
			}

			mMetrics[id] = m;
		} 
	}
	catch( ... ) { throw FontInvalidSourceExc(); }

	// measure font (standard ASCII range only to prevent weird characters influencing the measurements)
	for(uint16_t i=33;i<127;++i) {
		MetricsMap::const_iterator itr = mMetrics.find(i);
		if(itr != mMetrics.end()) {
			mAscent = std::max( mAscent, itr->second.dy );
			mDescent = std::max( mDescent, itr->second.h - itr->second.dy );
		}
	}

	mLeading = mAscent + mDescent;
	mFontSize = mAscent + mDescent;
	
	if(mMetrics.find(32) != mMetrics.end())
		mSpaceWidth = mMetrics[32].d;
}

void Font::read(const ci::DataSourceRef source)
{
	mInvalid = true;

	IStreamRef	in = source->createStream();
	size_t		filesize = in->size();

	// read header
	uint8_t header;
	in->read( &header ); if(header != 'S') throw FontInvalidSourceExc();
	in->read( &header ); if(header != 'D') throw FontInvalidSourceExc();
	in->read( &header ); if(header != 'F') throw FontInvalidSourceExc();
	in->read( &header ); if(header != 'F') throw FontInvalidSourceExc();

	uint16_t version;
	in->readLittle( &version );

	// read font name
	if( version > 0x0001 ) in->read( &mFamily );

	// read font data
	in->readData( (void*) &mLeading, sizeof(mLeading) );
	in->readData( (void*) &mAscent, sizeof(mAscent) );
	in->readData( (void*) &mDescent, sizeof(mDescent) );
	in->readData( (void*) &mSpaceWidth, sizeof(mSpaceWidth) );
	mFontSize = mAscent + mDescent;

	// read metrics data
	mMetrics.clear();

	try {
		uint16_t count;
		in->readLittle( &count );

		for(int i=0;i<count;++i) {
			uint16_t id;
			in->readLittle( &id );

			Metrics metrics;
			in->readData( (void*) &(metrics.x), sizeof(metrics.x) );
			in->readData( (void*) &(metrics.y), sizeof(metrics.y) );
			in->readData( (void*) &(metrics.w), sizeof(metrics.w) );
			in->readData( (void*) &(metrics.h), sizeof(metrics.h) );

			in->readData( (void*) &(metrics.dx), sizeof(metrics.dx) );
			in->readData( (void*) &(metrics.dy), sizeof(metrics.dy) );
			in->readData( (void*) &(metrics.d), sizeof(metrics.d) );

			mMetrics[id] = metrics;
		}
	}
	catch( ... ) {
		throw FontInvalidSourceExc();
	}

	// read image data	
	try {
		// reserve memory
		Buffer buffer(filesize);							
		// read the remaining data into memory (needs fix from Github's Cinder, won't work with release version)
		size_t bytesRead = in->readDataAvailable(buffer.getData(), filesize);	

		// load image
		mSurface = Surface( loadImage( DataSourceBuffer::create(buffer), ImageSource::Options(), "png" ) );

		// apply mip-mapping
		gl::Texture::Format fmt;
		fmt.enableMipmapping();
		fmt.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
		fmt.setMagFilter( GL_LINEAR );

		mTexture = gl::Texture( mSurface, fmt );
	}
	catch( ... ) {
		throw FontInvalidSourceExc();
	} 
}

void Font::write(const ci::DataTargetRef target)
{
	if(!target) throw FontInvalidTargetExc();

	OStreamRef out = target->getStream();

	// write header
	out->write( (uint8_t) 'S' );
	out->write( (uint8_t) 'D' );
	out->write( (uint8_t) 'F' );
	out->write( (uint8_t) 'F' );

	uint16_t version = 0x0002;
	out->writeLittle( version );

	// write font name
	out->write(mFamily);

	// write font data
	out->writeData( (void*) &mLeading, sizeof(mLeading) );
	out->writeData( (void*) &mAscent, sizeof(mAscent) );
	out->writeData( (void*) &mDescent, sizeof(mDescent) );
	out->writeData( (void*) &mSpaceWidth, sizeof(mSpaceWidth) );

	// write metrics data
	{
		uint16_t count = (uint16_t) mMetrics.size();
		out->writeLittle( count );

		MetricsMap::const_iterator itr;
		for(itr=mMetrics.begin();itr!=mMetrics.end();++itr) {
			// write char code
			out->writeLittle( itr->first );
			// write metrics
			out->writeData( (void*) &(itr->second.x), sizeof(itr->second.x) );
			out->writeData( (void*) &(itr->second.y), sizeof(itr->second.y) );
			out->writeData( (void*) &(itr->second.w), sizeof(itr->second.w) );
			out->writeData( (void*) &(itr->second.h), sizeof(itr->second.h) );
			
			out->writeData( (void*) &(itr->second.dx), sizeof(itr->second.dx) );
			out->writeData( (void*) &(itr->second.dy), sizeof(itr->second.dy) );
			out->writeData( (void*) &(itr->second.d), sizeof(itr->second.d) );
		}
	}

	// write image data
	writeImage( DataTargetStream::createRef(out), mSurface.getChannelRed(), ImageTarget::Options(), "png" );
}

Rectf Font::getBounds(uint16_t charcode, float fontSize) const
{
	MetricsMap::const_iterator itr = mMetrics.find(charcode);
	if( itr != mMetrics.end() ) {
			float	scale = (fontSize / mFontSize);

			return Rectf( 
				Vec2f(itr->second.dx, -itr->second.dy) * scale, 
				Vec2f(itr->second.dx + itr->second.w, itr->second.h - itr->second.dy) * scale 
			);
	}
	else
		return Rectf();
}

Rectf Font::getTexCoords(uint16_t charcode) const
{
	MetricsMap::const_iterator itr = mMetrics.find(charcode);
	if( itr != mMetrics.end() ) {
			Vec2f	size( mTexture.getSize() );

			return Rectf( 
				Vec2f(itr->second.x, itr->second.y) / size, 
				Vec2f(itr->second.x + itr->second.w, itr->second.y + itr->second.h) / size 
			);
	}
	else
		return Rectf();
}

float Font::getAdvance(uint16_t charcode, float fontSize) const
{
	float	scale = (fontSize / mFontSize);
	
	MetricsMap::const_iterator itr = mMetrics.find(charcode);
	if( itr != mMetrics.end() ) 
		return itr->second.d * scale;

	return 0.0f;
}

Rectf Font::measure(const std::wstring &text, float fontSize) const 
{
	float offset = 0.0f;
	Rectf result(0.0f, 0.0f, 0.0f, 0.0f);

	std::wstring::const_iterator citr;
	for(citr=text.begin();citr!=text.end();++citr) {
		uint16_t id = (uint16_t) *citr;

		// TODO: handle special chars like /t

		MetricsMap::const_iterator itr = mMetrics.find(id);
		if(itr != mMetrics.end()) {
			result.include( 
				Rectf(offset + itr->second.dx, -itr->second.dy, 
				offset + itr->second.dx + itr->second.w, itr->second.h - itr->second.dy) 
			);
			offset += itr->second.d;
		}
	}

	// return
	return result.scaled( fontSize / mFontSize );
}

float Font::measureWidth(const std::wstring &text, float fontSize, bool precise) const 
{
	float offset = 0.0f;
	float adjust = 0.0f;

	std::wstring::const_iterator citr;
	for(citr=text.begin();citr!=text.end();++citr) {
		uint16_t id = (uint16_t) *citr;

		// TODO: handle special chars like /t

		MetricsMap::const_iterator itr = mMetrics.find(id);
		if(itr != mMetrics.end()) {
			offset += itr->second.d;
			adjust = itr->second.dx + itr->second.w - itr->second.d;
		}
	}

	// precise measurement takes into account that the last character 
	// contributes to the total width only by its own width, not its advance
	if( precise )
		return (offset + adjust) * ( fontSize / mFontSize );
	else
		return offset * (fontSize / mFontSize);
}

} } // namespace ph::text