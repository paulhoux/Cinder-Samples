#version 120

uniform sampler2D	normal_map;
uniform bool		falloff_enabled;

// interpolated surface normal from vertex shader
varying vec3 Nsurface;

void main()
{
	// retrieve normal from texture
	vec3 Nmap = texture2D( normal_map, gl_TexCoord[0].xy ).rgb;

	// modify it with the original surface normal
	const vec3 Ndirection = vec3(0.0, 1.0, 0.0);	// see: normal_map_frag.glsl (y-direction)
	vec3 Nfinal = gl_NormalMatrix * normalize(Nsurface + Nmap - Ndirection);

	// perform some falloff magic
	float falloff = sin( max( dot( Nfinal, vec3(0.25, 1.0, 0.25) ), 0.0) * 2.25);	
	float alpha = 0.01 + (falloff_enabled ? 0.3 * pow( falloff, 25.0 ) : 0.3);

	// output color
	gl_FragColor = vec4( 1.0, 1.0, 1.0, alpha );
}