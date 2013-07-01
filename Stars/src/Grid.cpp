/*
 Copyright (c) 2010-2012, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "Grid.h"

using namespace ci;
using namespace std;

Grid::Grid(void)
	: mLineWidth(1.5f)
{
}

Grid::~Grid(void)
{
}

void Grid::setup()
{
	const int segments = 36;
	const int rings = 9;
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
	
	glLineWidth( mLineWidth );
	gl::color( Color(0.5f, 0.6f, 0.8f) * 0.25f );

	gl::enableAdditiveBlending();
	gl::draw( mVboMesh );
	gl::disableAlphaBlending();

	glPopAttrib();
}
