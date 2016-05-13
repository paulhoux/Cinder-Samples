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

#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "text/FontStore.h"

namespace ph {
namespace text {

using namespace ci;

bool FontStore::hasFont( const std::string &family )
{
	return ( mFonts.find( family ) != mFonts.end() );
}

FontRef FontStore::getFont( const std::string &family )
{
	if( hasFont( family ) )
		return mFonts.at( family );

	// return empty font on error
	return FontRef();
}

bool FontStore::addFont( FontRef font )
{
	if( !font )
		return false;

	// check if font family is already known
	std::string family = font->getFamily();
	if( !hasFont( family ) ) {
		mFonts[family] = font;
		return true;
	}

	return false;
}

std::vector<std::string> FontStore::listFonts()
{
	std::vector<std::string> keys;

	FontList::const_iterator itr;
	for( itr = mFonts.begin(); itr != mFonts.end(); ++itr )
		keys.push_back( itr->first );

	return keys;
}

FontRef FontStore::loadFont( DataSourceRef source )
{
	try {
		// try to load the file from source
		FontRef font = FontRef( new Font() );
		font->read( source );

		addFont( font );

		return font;
	}
	catch( const std::exception &e ) {
		app::console() << "Error loading font:" << e.what() << std::endl;
	}

	// return empty font on error
	return FontRef();
}
}
} // namespace ph::text