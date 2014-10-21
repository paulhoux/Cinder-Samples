#version 150

uniform sampler2D	uTexNormal;
uniform bool		uEnableFallOff;

uniform mat3 ciNormalMatrix;

in vec2 vTexCoord0;

// interpolated surface normal from vertex shader
in vec3 vNormal;

out vec4 oColor;

void main()
{
	// retrieve normal from texture
	vec3 Nmap = texture( uTexNormal, vTexCoord0.xy ).rgb;

	// modify it with the original surface normal
	const vec3 Ndirection = vec3(0.0, 1.0, 0.0);	// see: normal_map.frag (y-direction)
	vec3 Nfinal = ciNormalMatrix * normalize( vNormal + Nmap - Ndirection );

	// perform some falloff magic
	float falloff = sin( max( dot( Nfinal, vec3(0.25, 1.0, 0.25) ), 0.0) * 2.25);	
	float alpha = 0.01 + ( uEnableFallOff ? 0.3 * pow( falloff, 25.0 ) : 0.3 );

	// output color
	oColor = vec4( 1.0, 1.0, 1.0, alpha );
}