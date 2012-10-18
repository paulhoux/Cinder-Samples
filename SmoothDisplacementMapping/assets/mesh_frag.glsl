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
	float lambert = dot( Nfinal, vec3(0.15, 1.0, 0.15) );
	float falloff = 0.03 + 0.3 * pow( sin( 0.5 + lambert * 2.0 ), 50.0 );
	float alpha = falloff_enabled ? falloff : 0.25;

	// output color
	gl_FragColor = vec4( 1.0, 1.0, 1.0, alpha );
}