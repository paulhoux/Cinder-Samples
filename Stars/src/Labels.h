#pragma once

#include "cinder/DataSource.h"
#include "cinder/DataTarget.h"
#include "cinder/Utilities.h"

#include "text/TextLabels.h"

class Labels
{
public:
	Labels(void);
	virtual ~Labels(void);

	virtual void setup();
	virtual void update() {};
	virtual void draw();

	//! load a comma separated file containing the database
	virtual void load( ci::DataSourceRef source );

	//! reads a binary label data file
	void	read( ci::DataSourceRef source );
	//! writes a binary label data file
	void	write( ci::DataTargetRef target );
protected:
	ph::text::TextLabels	mLabels;
};

