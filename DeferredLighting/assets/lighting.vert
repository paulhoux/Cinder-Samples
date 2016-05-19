#version 150

uniform mat4 ciModelView;
uniform mat4 ciModelViewProjection;

in vec4 ciPosition;

in vec4 iPositionAndRadius; // per instance;
in vec4 iColorAndIntensity; // per instance;

out vec4 lightPosition; // in view space (xyz = position, w = radius)
out vec4 lightColor; // (rgb = color, a = intensity)

void main( void )
{
	lightPosition = ciModelView * vec4( iPositionAndRadius.xyz, 1 );
	lightPosition.w = iPositionAndRadius.w;
	lightColor = iColorAndIntensity;

	vec4 position = ciPosition;
	position.xyz = position.xyz * iPositionAndRadius.w + iPositionAndRadius.xyz;

	gl_Position = ciModelViewProjection * position;
}