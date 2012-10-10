#version 120

uniform sampler2D normal_map;

void main()
{
	const vec3 falloff = normalize( vec3(0.25, 1.0, 0.25) );

	// retrieve normal from texture
	vec3 normal = 2.0 * texture2D( normal_map, gl_TexCoord[0].xy ).rgb - vec3(1.0);
	normal = gl_NormalMatrix * normal;

	//
	float a = max( 0.0, dot(normal, falloff) );
	a = pow(a, 15.0) + 0.5 * pow(a, 7.5);

	// simply use the interpolated vertex colors
	gl_FragColor = vec4(1.0, 1.0, 1.0, 0.05 + 0.70 * a);
}