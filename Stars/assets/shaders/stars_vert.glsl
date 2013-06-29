#version 120

// point sprite sizes differ between ATI/AMD and NVIDIA GPU's,
// ATI's are twice as large and have to be scaled down
uniform float	scale;

// define a few constants here, for faster rendering
const float LOG_BASE10 = 1.0 / log2(10.0);

const float SIZE = 90.0;			// the higher the value, the bigger the stars will be
const float SIZE_MODIFIER = 1.5;	// the lower the value, the more stars are visible

const float MAG_LOWER_BOUND = 16.0;	// if a star's apparent magnitude is higher than this, it will be rendered at 0% brightness (black) - a value of 11 is more or less realistic
const float MAG_UPPER_BOUND = 0.0;	// if a star's apparent magnitude is lower than this, it will be rendered at 100% brightness
const float MAG_RANGE = MAG_LOWER_BOUND + MAG_UPPER_BOUND;

float log10( float n ) {
	return log2(n) * LOG_BASE10;
}

void main() { 
	// calculate distance of star (in parsecs) to camera
	// see: http://www.opengl.org/discussion_boards/showthread.php/166796-GLSL-PointSprites-different-sizes?p=1178125&viewfull=1#post1178125
	vec3	vertex = vec3(gl_ModelViewMatrix * gl_Vertex);
	float	distance = length(vertex);

	// retrieve absolute magnitude from texture coordinates
	float magnitude = gl_MultiTexCoord0.x;

	// calculate apparent magnitude based on distance	
	float apparent = magnitude - 5.0 * (1.0 - log10(distance));

	// calculate point size based on apparent magnitude
    gl_PointSize = scale * SIZE * pow(SIZE_MODIFIER, 1.0 - apparent); 

	// determine color
	float brightness = clamp((MAG_LOWER_BOUND + (1.0 - apparent)) / MAG_RANGE, 0.0, 1.0);
	gl_FrontColor = gl_Color * pow(brightness, 1.5);
	
	// set position
    gl_Position = ftransform(); 
}