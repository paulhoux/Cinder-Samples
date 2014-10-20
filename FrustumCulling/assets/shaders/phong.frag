#version 150

in vec4 vViewPosition;
in vec3 vNormal;

out vec4 oColor;

void main()
{
	oColor = vec4( 0.0, 0.0, 0.0, 1.0 );

	vec3 diffuse;
	vec3 specular;

	vec3 N = normalize( vNormal );
	vec3 L = normalize( -vViewPosition.xyz );
	vec3 E = normalize( -vViewPosition.xyz );
	vec3 R = normalize( reflect( -L, N ) );

	// diffuse term
	diffuse = vec3( 1.0, 0.0, 0.0 );
	diffuse *= max( dot( N, L ), 0.0 );
	oColor.rgb += diffuse;

	// specular term
	specular = vec3( 1.0, 1.0, 1.0 );
	specular *= pow( max( dot( R, E ), 0.0 ), 25.0 );
	oColor.rgb += specular;
}