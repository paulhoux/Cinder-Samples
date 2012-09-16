#pragma once

#include "cinder/DataSource.h"
#include "cinder/DataTarget.h"
#include "cinder/Utilities.h"

#include "text/TextLabels.h"

class Labels
{
public:
	Labels(void);
	~Labels(void);

	void setup();
	void update() {};
	void draw();

	//! load a comma separated file containing the HYG star database
	void	load( ci::DataSourceRef source );

	//! reads a binary label data file
	void	read( ci::DataSourceRef source );
	//! writes a binary label data file
	void	write( ci::DataTargetRef target );
private:
	ph::text::TextLabels	mLabels;
};

