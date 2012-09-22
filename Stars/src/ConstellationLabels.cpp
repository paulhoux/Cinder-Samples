#include "ConstellationLabels.h"
#include "Conversions.h"

#include "text/FontStore.h"

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

using namespace ci;
using namespace ci::app;
using namespace ph;

ConstellationLabels::ConstellationLabels(void)
{
}

ConstellationLabels::~ConstellationLabels(void)
{
}

void ConstellationLabels::draw()
{
	glPushAttrib( GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT );
	gl::enableAdditiveBlending();
	gl::color( Color(0.5f, 0.6f, 0.8f) );

	mLabels.draw();

	glPopAttrib();
}

void ConstellationLabels::load( DataSourceRef source )
{
	console() << "Loading constellation label database from CSV, please wait..." << std::endl;

	mLabels.clear();

	// load the star database
	std::string	names = loadString( source );

	// use boost tokenizer to parse the file
	std::vector<std::string> tokens;
	boost::split_iterator<std::string::iterator> lineItr, endItr;
	for (lineItr=boost::make_split_iterator(names, boost::token_finder(boost::is_any_of("\n\r")));lineItr!=endItr;++lineItr) {
		// retrieve a single, trimmed line
		std::string line = boost::algorithm::trim_copy( boost::copy_range<std::string>(*lineItr) );
		if(line.substr(0,1) == ";") continue;
		if(line.empty()) continue;

		// split into tokens   
		boost::algorithm::split( tokens, line, boost::is_any_of(";"), boost::token_compress_off );

		// skip if data was incomplete
		if(tokens.size() < 4)  continue;
		
		// 
		try {
			// name
			std::string name = boost::trim_copy( tokens[3] );
			if( name.empty() ) continue;

			// position
			double ra = Conversions::toDouble(tokens[0]);
			double dec = Conversions::toDouble(tokens[1]);

			double alpha = toRadians( ra * 15.0 );
			double delta = toRadians( dec );

			Vec3f position = 2000.0f * Vec3f((float) (sin(alpha) * cos(delta)), (float) sin(delta), (float) (cos(alpha) * cos(delta)));

			mLabels.addLabel( position, name );
		}
		catch(...) {
			// some of the data was invalid, ignore 
			continue;
		}
	}
}
