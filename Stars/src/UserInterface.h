#pragma once

#include "cinder/Font.h"
#include "cinder/gl/gl.h"

class UserInterface
{
public:
	UserInterface(void);
	~UserInterface(void);

	void	setup();
	void	draw();

	//! set distance of camera to Sun in parsecs, then convert to lightyears
	void	setCameraDistance( float distance ) { mDistance = distance * 3.261631f; }
private:
	float			mDistance;

	ci::Font		mFont;
	std::string		mText;
};

