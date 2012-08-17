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

#pragma once

#include "text/Font.h"

#include <map>

namespace ph { namespace text {

typedef std::map<std::string, FontRef>	FontList;

class FontStore
{
private:
	FontStore() {};
	~FontStore() {};
public:
	// singleton implementation
	static FontStore& getInstance() { 
		static FontStore fm; 
		return fm; 
	};

	bool		hasFont( const std::string &family );
	FontRef		getFont( const std::string &family );

	//!
	bool		addFont( FontRef font );

	//! returns a vector with all available font families
	std::vector<std::string>	listFonts();

	//! loads an SDFF file
	FontRef		loadFont( ci::DataSourceRef source );
protected:
	FontList	mFonts;
};

// helper function(s) for easier access 
inline FontStore&	fonts() { return FontStore::getInstance(); };

} } // namespace ph::text