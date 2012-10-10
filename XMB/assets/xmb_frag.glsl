#version 120

uniform sampler2D normal_map;

varying vec4 v;

void main()
{
	// retrieve normal from texture
	vec3 N = gl_NormalMatrix * texture2D( normal_map, gl_TexCoord[0].xy ).rgb;

	float a = max(0.0, dot( vec3(0,1,1), normalize(N) ));
	a = 0.75 * pow(a, 75.0);

	gl_FragColor = vec4( a, a, a, 1 );
}