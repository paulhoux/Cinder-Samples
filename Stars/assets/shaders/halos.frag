#version 150

uniform sampler2D tex0;

uniform float time;
uniform float aspect;

in vec4 vColor;

out vec4 oColor;

const vec2 ORIGIN = vec2(0.5, 0.5);

void main() 
{
	vec2 coord = gl_PointCoord.xy * 2.0 - 1.0;
	coord.x /= aspect;

	const float kFallOff = 6.0;
	float opacity = pow( clamp( 1.0 - length( coord ), 0.0, 1.0 ), kFallOff );

	oColor = opacity * texture(tex0, gl_PointCoord) * vColor;
}