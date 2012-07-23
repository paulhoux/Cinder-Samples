#include "Grid.h"

using namespace ci;
using namespace std;

Grid::Grid(void)
{
}

Grid::~Grid(void)
{
}

void Grid::setup()
{
	const int segments = 30;
	const int rings = 15;
	const int subdiv = 10;

	const float radius = 2000.0f;

	const float theta_step = toRadians(90.0f) / (rings * subdiv);
	const float phi_step = toRadians(360.0f) / (segments * subdiv);
	
	std::vector< Vec3f > vertices;

	// start with the rings
	float x, y, z;
	for(int theta=1-rings;theta<rings;++theta) {
		float tr = theta * theta_step * subdiv;

		y = sinf(tr);

		for(int phi=0;phi<segments;++phi) {
			for(int div=0;div<subdiv;++div) {
				float pr = (phi * subdiv + div) * phi_step;

				x = cosf(tr) * sinf(pr);
				z = cosf(tr) * cosf(pr);
				vertices.push_back( radius * Vec3f(x, y, z) );

				pr += phi_step;

				x = cosf(tr) * sinf(pr);
				z = cosf(tr) * cosf(pr);
				vertices.push_back( radius * Vec3f(x, y, z) );
			}
		}
	}

	// then the segments
	for(int phi=0;phi<segments;++phi) {
		float pr = phi * phi_step * subdiv;

		for(int theta=1-rings;theta<rings-1;++theta) {
			for(int div=0;div<subdiv;++div) {
				float tr = (theta * subdiv + div) * theta_step;

				x = cosf(tr) * sinf(pr);
				y = sinf(tr);
				z = cosf(tr) * cosf(pr);
				vertices.push_back( radius * Vec3f(x, y, z) );

				tr += theta_step;

				x = cosf(tr) * sinf(pr);
				y = sinf(tr);
				z = cosf(tr) * cosf(pr);
				vertices.push_back( radius * Vec3f(x, y, z) );
			}
		}
	}
	
	//
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();

	mVboMesh = gl::VboMesh(vertices.size(), 0, layout, GL_LINES);
	mVboMesh.bufferPositions( &(vertices.front()), vertices.size() );	
}

void Grid::draw()
{
	if(!mVboMesh) return;

	glPushAttrib( GL_CURRENT_BIT | GL_LINE_BIT | GL_ENABLE_BIT );
	
	glLineWidth( 2.0f );
	gl::color( Color(0.1f, 0.1f, 0.1f) );

	gl::enableAdditiveBlending();
	gl::draw( mVboMesh );
	gl::disableAlphaBlending();

	glPopAttrib();
}
