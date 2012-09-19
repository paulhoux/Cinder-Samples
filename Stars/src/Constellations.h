#pragma once

#include "cinder/DataSource.h"
#include "cinder/DataTarget.h"
#include "cinder/Utilities.h"

#include "cinder/gl/Vbo.h"

class Constellations
{
public:
	Constellations(void);
	~Constellations(void);

	void setup() {};
	void update() {};
	void draw();

	void	clear();

	//! load a comma separated file containing the HYG star database
	void	load( ci::DataSourceRef source );

	//! reads a binary label data file
	void	read( ci::DataSourceRef source );
	//! writes a binary label data file
	void	write( ci::DataTargetRef target );
private:
	void						createMesh();

	ci::Vec3d					getStarCoordinate( double ra, double dec, double distance );
	std::vector< ci::Vec3d >	getStarCoordinates( ci::DataSourceRef source );
private:
	ci::gl::VboMesh				mMesh;

	std::vector< ci::Vec3f >	mVertices;
	std::vector< uint32_t >		mIndices;
};

