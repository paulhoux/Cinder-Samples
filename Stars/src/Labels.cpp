#include "Labels.h"
#include "Conversions.h"

#include "text/FontStore.h"

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

using namespace ci;
using namespace ci::app;
using namespace ph;

Labels::Labels(void)
{
}

Labels::~Labels(void)
{
}

void Labels::setup()
{
	// intialize labels
	text::fonts().loadFont( loadAsset("fonts/Ubuntu-LightItalic.sdff") ); 
	mLabels.setFont( text::fonts().getFont("Ubuntu-LightItalic") );
	mLabels.setFontSize( 12.0f );
	mLabels.setBoundary( text::Text::NONE );
	mLabels.setOffset( 2.5f, 2.5f );
}

void Labels::draw()
{
	glPushAttrib( GL_CURRENT_BIT );
	gl::color( Color::white() );

	mLabels.draw();

	glPopAttrib();
}

void Labels::load( DataSourceRef source )
{
	console() << "Loading label database from CSV, please wait..." << std::endl;

	mLabels.clear();

	// load the star database
	std::string	stars = loadString( source );

	// use boost tokenizer to parse the file
	std::vector<std::string> tokens;
	boost::split_iterator<std::string::iterator> lineItr, endItr;
	for (lineItr=boost::make_split_iterator(stars, boost::token_finder(boost::is_any_of("\n\r")));lineItr!=endItr;++lineItr) {
		// retrieve a single, trimmed line
		std::string line = boost::algorithm::trim_copy( boost::copy_range<std::string>(*lineItr) );
		if(line.empty()) continue;

		// split into tokens   
		boost::algorithm::split( tokens, line, boost::is_any_of(";"), boost::token_compress_off );

		// skip if data was incomplete
		if(tokens.size() < 23)  continue;
		
		// 
		try {
			// name
			std::string name = boost::trim_copy( tokens[6] );
			//if( name.empty() ) name = boost::trim_copy( tokens[5] );
			//if( name.empty() ) name = boost::trim_copy( tokens[4] );
			if( name.empty() ) continue;

			// position
			double ra = Conversions::toDouble(tokens[7]);
			double dec = Conversions::toDouble(tokens[8]);
			float distance = Conversions::toFloat(tokens[9]);

			double alpha = toRadians( ra * 15.0 );
			double delta = toRadians( dec );

			Vec3f position = distance * Vec3f((float) (sin(alpha) * cos(delta)), (float) sin(delta), (float) (cos(alpha) * cos(delta)));

			mLabels.addLabel( position, name );
		}
		catch(...) {
			// some of the data was invalid, ignore 
			continue;
		}
	}
}

void Labels::read(DataSourceRef source)
{
	IStreamRef in = source->createStream();
	
	mLabels.clear();

	uint8_t versionNumber;
	in->read( &versionNumber );
	
	uint32_t numLabels;
	in->readLittle( &numLabels );
	
	for( size_t idx = 0; idx < numLabels; ++idx ) {
		Vec3f position;
		in->readLittle( &position.x ); in->readLittle( &position.y ); in->readLittle( &position.z );
		std::string name;
		in->read( &name );

		mLabels.addLabel( position, name );
	}
}

void Labels::write(DataTargetRef target)
{
	OStreamRef out = target->getStream();
	
	const uint8_t versionNumber = 1;
	out->write( versionNumber );
	
	out->writeLittle( static_cast<uint32_t>( mLabels.size() ) );
	
	for( text::TextLabelListConstIter it = mLabels.begin(); it != mLabels.end(); ++it ) {
		Vec3f position = it->first;
		out->writeLittle( position.x ); out->writeLittle( position.y ); out->writeLittle( position.z );
		std::string name = toUtf8( it->second );
		out->write( name );
	}
}
