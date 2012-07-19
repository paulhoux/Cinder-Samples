#version 110

uniform float size;

void main() { 
	// change size based on distance
	// see: http://www.opengl.org/discussion_boards/showthread.php/166796-GLSL-PointSprites-different-sizes?p=1178125&viewfull=1#post1178125
	vec3	vertex = vec3(gl_ModelViewMatrix * gl_Vertex);
	float	distance = length(vertex);
	float	attenuation = sqrt(1.0 / (0.95 + (0.02 + 0.0 * distance) * distance));
	float	scale = max( clamp(size * attenuation, 0.1, size), 1.0);

    gl_PointSize = scale;// * gl_MultiTexCoord0.x;
	gl_FrontColor = gl_Color;

    gl_Position = ftransform(); 
}