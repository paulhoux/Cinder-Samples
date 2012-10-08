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
		if(mMetrics.find(i) != mMetrics.end()) {
			mAscent = std::max( mAscent, mMetrics.at(i).dy );
			mDescent = std::max( mDescent, mMetrics.at(i).h - mMetrics.at(i).dy );
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

		std::map<uint16_t, Metrics>::const_iterator itr;
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
	if( mMetrics.find(charcode) != mMetrics.end() ) {
			float	scale = (fontSize / mFontSize);
			Metrics	m = mMetrics.at(charcode);

			return Rectf( Vec2f(m.dx, -m.dy) * scale, Vec2f(m.dx + m.w, m.h - m.dy) * scale );
	}
	else
		return Rectf();
}

Rectf Font::getTexCoords(uint16_t charcode) const
{
	if( mMetrics.find(charcode) != mMetrics.end() ) {
			Vec2f	size( mTexture.getSize() );
			Metrics	m = mMetrics.at(charcode);

			return Rectf( Vec2f(m.x, m.y) / size, Vec2f(m.x + m.w, m.y + m.h) / size );
	}
	else
		return Rectf();
}

float Font::getAdvance(uint16_t charcode, float fontSize) const
{
	float	scale = (fontSize / mFontSize);

	if( mMetrics.find(charcode) != mMetrics.end() ) 
		return mMetrics.at(charcode).d * scale;

	return 0.0f;
}

Rectf Font::measure(const std::wstring &text, float fontSize) const 
{
	float offset = 0.0f;
	Rectf result(0.0f, 0.0f, 0.0f, 0.0f);

	float scale = (fontSize / mFontSize);

	std::wstring::const_iterator itr;
	for(itr=text.begin();itr!=text.end();++itr) {
		uint16_t id = (uint16_t) *itr;

		// TODO: handle special chars

		if(mMetrics.find(id) != mMetrics.end()) {
			Metrics m = mMetrics.at(id);
			result.include( Rectf(offset + (m.dx * scale), -(m.dy * scale), offset + (m.dx + m.w) * scale, (m.h - m.dy) * scale) );
			offset += m.d * scale;
		}
	}

	// return
	return result;
}

/*
Vec2f Font::render(TriMesh2d &mesh, const std::wstring &text, float fontSize, const Vec2f &origin)
{
	// set cursor
	Vec2f cursor(origin);
	Vec2f size( mTexture.getSize() );

	float scale = (fontSize / mFontSize);

	// 
	std::vector<uint32_t>	indices;

	std::wstring::const_iterator itr;
	for(itr=text.begin();itr!=text.end();++itr) {
		uint16_t id = (uint16_t) *itr;

		// TODO better handling of special character
		if(id == '\n') {
			cursor.x = 0.0f;
			cursor.y += mLeading * scale;
		}
		else if(id == '\r') {
		}
		else if(id == '\t') { 
			cursor.x += 4.0f * mSpaceWidth * scale;
		}
		else if(id == 32) {
			cursor.x += mSpaceWidth * scale;
		}
		else if( mMetrics.find(id) != mMetrics.end() ) {
			Metrics m = mMetrics[id];

			int n = (int) mesh.getVertices().size();
			mesh.appendVertex( cursor + Vec2f(m.dx, -m.dy) * scale );
			mesh.appendVertex( cursor + Vec2f(m.dx + m.w, -m.dy) * scale );
			mesh.appendVertex( cursor + Vec2f(m.dx + m.w, -m.dy + m.h) * scale );
			mesh.appendVertex( cursor + Vec2f(m.dx, -m.dy + m.h) * scale );
			
			mesh.appendTexCoord( Vec2f(m.x, m.y) / size );
			mesh.appendTexCoord( Vec2f(m.x + m.w, m.y) / size );
			mesh.appendTexCoord( Vec2f(m.x + m.w, m.y + m.h) / size );
			mesh.appendTexCoord( Vec2f(m.x, m.y + m.h) / size );

			indices.push_back(n); indices.push_back(n+3); indices.push_back(n+1);
			indices.push_back(n+1); indices.push_back(n+3); indices.push_back(n+2);

			cursor += Vec2f(m.d, 0.0f) * scale;
		}
	}

	//
	mesh.appendIndices( &(*(indices.begin())), indices.size() );

	//
	return cursor;
}
*/

} } // namespace ph::text