#include "Conversions.h"

#include <sstream>

using namespace ci;
using namespace std;

Color Conversions::toColor(uint32_t hex)
{
	float r = ((hex & 0x00FF0000) >> 16) / 255.0f;
	float g = ((hex & 0x0000FF00) >> 8) / 255.0f;
	float b = ((hex & 0x000000FF)) / 255.0f;

	return Color(r, g, b);
}

ColorA Conversions::toColorA(uint32_t hex)
{
	float a = ((hex & 0xFF000000) >> 24) / 255.0f;
	float r = ((hex & 0x00FF0000) >> 16) / 255.0f;
	float g = ((hex & 0x0000FF00) >> 8) / 255.0f;
	float b = ((hex & 0x000000FF)) / 255.0f;

	return ColorA(r, g, b, a);
}

int Conversions::toInt(const std::string &str)
{
	int x;
	std::istringstream i(str);

	if (!(i >> x)) throw std::exception();

	return x;
}

double Conversions::toDouble(const std::string &str)
{
	double x;
	std::istringstream i(str);

	if (!(i >> x)) throw std::exception();

	return x;
}


