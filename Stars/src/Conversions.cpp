#include "Conversions.h"

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <map>
#include <sstream>

using namespace ci;
using namespace std;

Color Conversions::toColor(uint32_t hex)
{
	float r = ((hex & 0x00FF0000) >> 16) / 255.0f;
	float g = ((hex & 0x0000FF00) >> 8) / 255.0f;
	float b = ((hex & 0x000000FF)) / 255.0f;

	return Color(r, g, b);
}

ColorA Conversions::toColorA(uint32_t hex)
{
	float a = ((hex & 0xFF000000) >> 24) / 255.0f;
	float r = ((hex & 0x00FF0000) >> 16) / 255.0f;
	float g = ((hex & 0x0000FF00) >> 8) / 255.0f;
	float b = ((hex & 0x000000FF)) / 255.0f;

	return ColorA(r, g, b, a);
}

int Conversions::toInt(const std::string &str)
{
	int x;
	std::istringstream i(str);

	if (!(i >> x)) throw std::exception();

	return x;
}

float Conversions::toFloat(const std::string &str)
{
	float x;
	std::istringstream i(str);

	if (!(i >> x)) throw std::exception();

	return x;
}

double Conversions::toDouble(const std::string &str)
{
	double x;
	std::istringstream i(str);

	if (!(i >> x)) throw std::exception();

	return x;
}

//

void Conversions::mergeNames( ci::DataSourceRef hyg, ci::DataSourceRef ciel )
{
	// read star names
	std::string	stars = loadString( ciel );

	std::vector< std::string > tokens;
	std::map< uint32_t, std::string > names;

	std::vector< std::string > lines;
	boost::algorithm::split( lines, stars, boost::is_any_of("\r\n"), boost::token_compress_on );

	std::vector< std::string >::iterator itr;
	for(itr=lines.begin();itr!=lines.end();++itr) {
		std::string line = boost::trim_copy( *itr );
		if(line.empty()) continue;
		if(line.substr(0,1) == ";") continue;

		try {
			uint32_t hr = Conversions::toInt( itr->substr(0,9) );
			boost::algorithm::split( tokens, itr->substr(9), boost::is_any_of(";"), boost::token_compress_off );

			names.insert( std::pair< uint32_t, std::string >( hr, tokens[0] ) );
		}
		catch(...) {}
	}
	
	// merge star names with HYG
	stars = loadString( hyg );
	boost::algorithm::split( lines, stars, boost::is_any_of("\n\r"), boost::token_compress_on );
	for(itr=lines.begin();itr!=lines.end();++itr) {
		std::string line = boost::trim_copy( *itr );

		boost::algorithm::split( tokens, line, boost::is_any_of(";"), boost::token_compress_off );

		if( tokens.size() >= 4 && !tokens[4].empty() ) {
			try {
				uint32_t hr = Conversions::toInt( tokens[3] );
				if( !names[hr].empty() ) {
					tokens[6] = names[hr];
				}
			}
			catch(...) {}
		}

		*itr = boost::algorithm::join( tokens, ";" );
	}

	stars = boost::algorithm::join( lines, "\r\n" );

	DataTargetPathRef target = writeFile( hyg->getFilePath() );
	OStreamRef stream = target->getStream();
	stream->write( stars );
}


