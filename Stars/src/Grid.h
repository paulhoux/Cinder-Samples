#pragma once

#include "cinder/gl/Vbo.h"

class Grid
{
public:
	Grid(void);
	~Grid(void);

	void setup();
	void draw();
private:
	ci::gl::VboMesh		mVboMesh;
};

