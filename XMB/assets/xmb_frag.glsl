#version 120

uniform sampler2D normal_map;

void main()
{
	// retrieve normal from texture
	vec3 normal = 2.0 * texture2D( normal_map, gl_TexCoord[0].xy ).rgb - vec3(1.0);
	normal = gl_NormalMatrix * normal;

	//
	float a = 0.5 + 0.5 * dot( normal, vec3(0, 1, 0) );

	// simply use the interpolated vertex colors
	gl_FragColor = vec4(1.0, 1.0, 1.0, 0.8 * pow(a, 10.0));
}