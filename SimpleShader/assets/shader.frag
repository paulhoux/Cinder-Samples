#version 150

// We will use two 2D textures.
uniform sampler2D tex0;
uniform sampler2D tex1;

// We get the interpolated texture coordinate from the vertex shader.
in vec4 vTexCoord0;

// The output of our fragment shader is an RGBA color.
out vec4 oColor;

void main( void )
{
	// Using the interpolated texture coordinate, 
	// find the color of the bottom image.
	vec4 color0 = texture( tex0, vTexCoord0.st );

	// Do the same for the top image.
	vec4 color1 = texture( tex1, vTexCoord0.st );

	// In this example, we will invert the bottom color 
	// (black becomes white, red becomes cyan, etc.)...
	color0.rgb = vec3( 1.0 ) - color0.rgb;

	// ...and we will make the top image 50% transparent.
	color1.a = 0.5;

	// Now, we will have to do the blending ourselves...
	vec4 result = mix( color0, color1, color1.a );

	// ...and set the final pixel color.
	oColor.rgb = result.rgb;
	oColor.a = 1.0;
}