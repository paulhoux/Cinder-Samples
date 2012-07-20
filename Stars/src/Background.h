#pragma once

#include "cinder/Vector.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"

class Background
{
public:
	Background(void);
	~Background(void);

	void	setup();
	void	draw();

	void	create();

	void	setCameraDistance( float distance );
private:
	float				mAttenuation;
	ci::Vec3f			mRotation;

	ci::gl::Texture		mTexture;
	ci::gl::VboMesh		mVboMesh;
};

