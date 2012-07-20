#version 110

varying float angle;

void main() { 
	// calculate distance of star (in parsecs) to camera
	// see: http://www.opengl.org/discussion_boards/showthread.php/166796-GLSL-PointSprites-different-sizes?p=1178125&viewfull=1#post1178125
	vec3	vertex = vec3(gl_ModelViewMatrix * gl_Vertex);
	float	distance = length(vertex);

	// determine an angle for the star streaks
	//vertex = normalize(vertex);
	//angle = vertex.x * -3.0 + vertex.y * 2.0;

	// retrieve absolute magnitude from texture coordinates
	float magnitude = gl_MultiTexCoord0.x;

	// calculate apparent magnitude based on distance
	const float toBase10 = 1.0 / log2(10.0);
	float apparent = magnitude - 5.0 * (1.0 - log2(distance) * toBase10);

	// calculate point size based on apparent magnitude
	const float size_modifier = 1.3; // the lower the value, the more stars are visible
    gl_PointSize = 60.0 * pow(size_modifier, 1.0 - apparent); 

	// determine color
	const float lower = 20.0;
	const float upper = 2.0;
	float brightness = clamp((lower + (1.0 - apparent)) / (lower + upper), 0.0, 1.0);
	gl_FrontColor = gl_Color * brightness;
	
	// set position
    gl_Position = ftransform(); 
}