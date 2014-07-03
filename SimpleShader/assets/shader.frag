#version 110

// we will use two 2D textures
uniform sampler2D tex0;
uniform sampler2D tex1;

void main(void)
{
	// using the interpolated texture coordinate, 
	// find the color of the bottom image
	vec4 color0 = texture2D( tex0, gl_TexCoord[0].st );
	// do the same for the top image
	vec4 color1 = texture2D( tex1, gl_TexCoord[0].st );

	// in this example, we will invert the bottom color 
	// (black becomes white, red becomes cyan, etc.)
	color0.rgb = vec3(1.0) - color0.rgb;

	// and we will make the top image 50% transparent
	color1.a = 0.5;

	// now, we will have to do the blending ourselves
	vec4 result = mix(color0, color1, color1.a);

	// and set the final pixel color
	gl_FragColor.rgb = result.rgb;
	gl_FragColor.a = 1.0;
}