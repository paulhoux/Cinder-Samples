#pragma once

#include "Labels.h"

class ConstellationLabels
	: public Labels
{
public:
	ConstellationLabels(void);
	~ConstellationLabels(void);

	void draw();

	//! load a comma separated file containing the database
	void	load( ci::DataSourceRef source );
};

