#version 150

uniform sampler2D tex0;
uniform sampler2D tex1;

uniform float time;
uniform float aspect;

in vec4 vColor;

out vec4 oColor;

const vec2 ORIGIN = vec2(0.5, 0.5);

void main() 
{
	float c, s;
	vec2 coord = gl_PointCoord.xy - ORIGIN;

	coord /= vec2(aspect, 1.0);

	// combine two rotated corona samples 
	// by modifying the texture look-up and mixing the sampled colors
	c = cos(time);	s = sin(time);	
	vec2 coord1 = vec2(coord.x * c - coord.y * s, coord.y * c + coord.x * s) + ORIGIN;
	c = cos(time * -0.7);	s = sin(time * -0.7);	
	vec2 coord2 = vec2(coord.x * c - coord.y * s, coord.y * c + coord.x * s) + ORIGIN;

	vec4 corona = mix(texture(tex1, coord1), texture(tex1, coord2), 0.5) * vColor;

	// mix with star sample
	c = cos(time * 0.1);	s = sin(time * 0.1);	
	vec2 coord3 = vec2(coord.x * c - coord.y * s, coord.y * c + coord.x * s) + ORIGIN;

	vec4 star = pow( texture(tex0, coord3), vec4( 2.0 ) ) * vColor;
	oColor = star + corona;
}