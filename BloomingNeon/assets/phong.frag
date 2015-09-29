#version 150

uniform sampler2D tex_diffuse;
uniform sampler2D tex_specular;

in vec4 vViewPosition;
in vec3 vNormal;
in vec2 vTexCoord0;

out vec4 oColor;

void main()
{
	const float shininess = 5.0;
	const vec4  lightViewPosition = vec4( 0, 5, 0, 1 );
	const vec4  lightDiffuse = vec4( 1, 1, 1, 1 );
	const vec4  lightSpecular = vec4( 1, 1, 1, 1 );

	vec3 N = normalize( vNormal );
	vec3 L = normalize( lightViewPosition.xyz - vViewPosition.xyz );
	vec3 E = normalize( -vViewPosition.xyz );
	vec3 R = normalize( -reflect( L, N ) );

	// ambient term 
	vec3 ambient = vec3( 0, 0, 0 );

	// diffuse term
	vec3 diffuse = pow( texture2D( tex_diffuse, vTexCoord0 ).rgb * lightDiffuse.rgb, vec3( 2.0 ) );
	diffuse *= max( dot( N, L ), 0.0 );
	diffuse = clamp( diffuse, 0.0, 1.0 );

	// specular term
	vec3 specular = pow( texture2D( tex_specular, vTexCoord0 ).rgb * lightSpecular.rgb, vec3( 2.0 ) );
	specular *= pow( max( dot( R, E ), 0.0 ), shininess );
	specular = clamp( specular, 0.0, 1.0 );

	// final color 
	oColor.rgb = sqrt( ambient + diffuse + specular );
	oColor.a = 1.0;
}