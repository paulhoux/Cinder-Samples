#version 150

uniform float		uTexOffset;
uniform sampler2D	uLeftTex;
uniform sampler2D	uRightTex;

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec4 ciColor;
in vec2 ciTexCoord0;

out vec4 vertColor;
out vec2 vertTexCoord0;

void main(void)
{	
	// retrieve texture coordinate and offset it to scroll the texture
	vec2 coord = ciTexCoord0 + vec2(0.0, uTexOffset);

	// retrieve the FFT from left and right texture and average it
	float fft = max(0.0001, mix( texture( uLeftTex, coord ).r, texture( uRightTex, coord ).r, 0.5));

	// convert to decibels
	const float kLogBase10 = 1.0 / log(10.0);
	float decibels = 10.0 * log( fft ) * kLogBase10;

	// offset the vertex based on the decibels
	vec4 vertex = ciPosition;
	vertex.y += 2.0 * decibels;

	// pass (unchanged) texture coordinates, bumped vertex and vertex color
	vertTexCoord0 = ciTexCoord0;
	vertColor = ciColor;
	gl_Position = ciModelViewProjection * vertex;
}