/*
Copyright (C) 2011-2012 Paul Houx

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "cinder/Utilities.h"
#include "cinder/app/AppBasic.h"
#include "text/FontStore.h"

namespace ph { namespace text {

using namespace ci;

bool	FontStore::hasFont( const std::string &family )
{
	return (mFonts.find(family) != mFonts.end());
}

FontRef	FontStore::getFont( const std::string &family )
{
	if( hasFont(family) )
		return mFonts.at(family);

	// return empty font on error
	return FontRef();
}

bool	FontStore::addFont( FontRef font )
{
	if(!font) return false;

	// check if font family is already known
	std::string family = font->getFamily();
	if( ! hasFont( family ) ) {
		mFonts[family] = font;
		return true;
	}

	return false;
}

std::vector<std::string> FontStore::listFonts()
{
	std::vector<std::string> keys;

	FontList::const_iterator itr;
	for(itr=mFonts.begin();itr!=mFonts.end();++itr) 
		keys.push_back( itr->first );

	return keys;
}

FontRef	FontStore::loadFont( DataSourceRef source )
{
	try { 
		// try to load the file from source
		FontRef font = FontRef( new Font() );
		font->read(source); 

		addFont(font);

		return font;
	}
	catch( const std::exception &e ) {
		app::console() << "Error loading font:" << e.what() << std::endl;
	}
	
	// return empty font on error
	return FontRef();
}

} } // namespace ph::text