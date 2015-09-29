#version 150

uniform sampler2D	tex0;
uniform vec3 		pickingColor;

in vec4 vPosition;
in vec3 vNormal;
in vec2 vTexCoord0;
in vec3 vColor;

out vec4 [2] oColor;

void main()
{
	vec2 uv = vTexCoord0.st;

	vec3 N = normalize( vNormal );
	vec3 L = normalize( -vPosition.xyz );
	vec3 E = normalize( -vPosition.xyz );
	vec3 R = normalize( -reflect( L, N ) );

	// ambient term 
	vec3 ambient = vec3( 0 );

	// diffuse term
	vec3 diffuse = vColor; // or texture2D(tex0, uv) if you use textures
	diffuse *= max( dot( N, L ), 0.0 );
	diffuse = clamp( diffuse, 0.0, 1.0 );

	// specular term
	vec3 specular = vec3( 1 );
	specular *= pow( max( dot( R, E ), 0.0 ), 20.0 );
	specular = clamp( specular, 0.0, 1.0 );

	// write final color in first color target 
	oColor[0].rgb = ambient + diffuse + specular;
	oColor[0].a = 1.0;

	// write color code in second color target
	//  note:  use texture2D(tex0, uv).a for the alpha value if you use textures
	oColor[1] = vec4( pickingColor, 1.0 );
}