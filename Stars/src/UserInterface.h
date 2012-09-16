#pragma once

#include "cinder/gl/gl.h"
#include "text/TextBox.h"

#include <boost/format.hpp>

class UserInterface
{
public:
	UserInterface(void);
	~UserInterface(void);

	void	setup();
	void	draw();

	//! set distance of camera to Sun in parsecs, then convert to lightyears
	void	setCameraDistance( float distance ) { mDistance = distance * 3.261631f; mBox.setText( (boost::format(mText) % mDistance).str() ); }
private:
	float				mDistance;

	ph::text::TextBox	mBox;
	std::string			mText;
};

