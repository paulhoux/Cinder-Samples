#version 120

uniform sampler2D normal_map;

varying vec4 v;

void main()
{
	// retrieve normal from texture
	vec3 N = gl_NormalMatrix * texture2D( normal_map, gl_TexCoord[0].xy ).rgb;

	// perform some falloff magic
	float falloff = sin( max( dot( N, vec3(0.25, 1.0, 0.25) ), 0.0) * 2.25);	
	float alpha = 0.01 + 0.3 * pow( falloff, 25.0 );

	// output color
	gl_FragColor = vec4( 1.0, 1.0, 1.0, alpha );
}