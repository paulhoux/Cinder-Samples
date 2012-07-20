#pragma once

#include "cinder/Color.h"

class Conversions
{
public:
	//! converts a hexadecimal color (0xRRGGBB) to a Color
	static ci::Color toColor(uint32_t hex);
	//! converts a hexadecimal color (0xAARRGGBB) to a ColorA
	static ci::ColorA toColorA(uint32_t hex);
	//! converts a string to an integer
	static int toInt(const std::string &str);
	//! converts a string to a double
	static double toDouble(const std::string &str);
};

