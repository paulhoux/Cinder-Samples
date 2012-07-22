
#version 120

uniform sampler2D tex0;

varying float angle;

void main() 
{
	/*// rotate the sprite by modifying the texture look-up:
	float c = cos(angle);
	float s = sin(angle);	
	const vec2 origin = vec2(0.5, 0.5);

	vec2 coord = gl_PointCoord.xy - origin;
	coord = vec2(coord.x * c - coord.y * s, coord.y * c + coord.x * s) + origin;
	//*/

	//
	gl_FragColor = texture2D(tex0, gl_PointCoord.xy) * gl_Color;
}