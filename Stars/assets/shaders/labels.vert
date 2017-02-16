#version 150

#include "common.glsl"

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelView;

in vec4 ciPosition;
in vec4 ciColor;
in vec2 ciTexCoord0;
in vec4 ciTexCoord1;

out vec4 vColor;
out vec2 vTexCoord0;

// viewport parameters (x, y, width, height)
uniform vec4 uViewport;
uniform vec2 uScale;

void main()
{
	// convert label position to normalized device coordinates to find the 2D offset
	vec4 position = vec4( ciTexCoord1.xyz , 1 );
	vec3 offset = toNDC( ciModelViewProjection * position );

	// add extra offset based on star's diameter
	vec3  v = vec3( ciModelView * position );
	float dist = length( v );
	float magnitude = ciTexCoord1.w;	
	float apparent = apparentMagnitude( magnitude, dist );

	float size = starSize( apparent );
	offset.x += 0.25 * size / uViewport.z;

	// pass font texture coordinate to fragment shader
	vTexCoord0 = ciTexCoord0;

	// set the color
	vColor.rgb = ciColor.rgb;

	// convert vertex from screen space to normalized device coordinates
	vec3 vertex = vec3( ciPosition.xy * vec2( uScale.x, -uScale.y ) / uViewport.zw * 2.0, 0.0 );
	vertex.xy *= min( 1.0, max( size * 0.05, 2.0 * saturate( exp2( 1.0 - dist * 0.1 ) ) ) );

	// calculate final vertex position by offsetting it
	gl_Position = vec4( vertex + offset, 1.0 );
}