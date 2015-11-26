#version 150

#include "common.glsl"

uniform mat4 ciModelView;
uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec2 ciTexCoord0;
in vec4 ciColor;

out vec4 vColor;

// point sprite sizes differ between ATI/AMD and NVIDIA GPU's,
// ATI's are twice as large and have to be scaled down
uniform float	scale;


void main() { 
	// calculate distance of star (in parsecs) to camera
	// see: http://www.opengl.org/discussion_boards/showthread.php/166796-GLSL-PointSprites-different-sizes?p=1178125&viewfull=1#post1178125
	vec3	vertex = vec3( ciModelView * ciPosition );
	float	dist = length( vertex );

	// retrieve absolute magnitude from texture coordinates
	float magnitude = ciTexCoord0.x;

	// calculate apparent magnitude based on distance	
	float apparent = apparentMagnitude( magnitude, dist );

	// determine color
	vColor = ciColor * starBrightness( apparent );

	// calculate point size based on apparent magnitude
    gl_PointSize = scale * starSize( apparent );
	
	// set position
    gl_Position = ciModelViewProjection * ciPosition; 
}