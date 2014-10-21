// adapted from: http://stackoverflow.com/questions/9234724/how-to-change-hue-of-a-texture-with-glsl

#version 150

uniform sampler2D	uTex0;
uniform float		uHue;

in vec2 vTexCoord0;

out vec4 oColor;

const mat3 rgb2yiq = mat3( 0.299, 0.595716, 0.211456, 0.587, -0.274453, -0.522591, 0.114, -0.321263, 0.311135 );
const mat3 yiq2rgb = mat3( 1.0, 1.0, 1.0, 0.9563, -0.2721, -1.1070, 0.6210, -0.6474, 1.7046 );

void main() {
	// convert rgb to yiq 
	vec3 yiq = rgb2yiq * texture( uTex0, vTexCoord0 ).rgb;

	// calculate new hue
	float h = uHue + atan( yiq.b, yiq.g );

	// convert yiq to rgb
	float chroma = sqrt( yiq.b * yiq.b + yiq.g * yiq.g );
	vec3 rgb = yiq2rgb * vec3( yiq.r, chroma * cos(h), chroma * sin(h) );

	// output pixel color
	oColor = vec4( rgb, 1.0 );
}