#version 150

in vec4 vertPosition;
in vec3 vertNormal;
in vec4 vertColor;

out vec4 fragColor;

void main()
{
	// kLightPosition position in eye space (relative to camera)
	const vec3 kLightPosition = vec3(0.0, 0.0, 0.0);

	// calculate lighting vectors
    vec3 N = normalize( vertNormal );
	vec3 L = normalize( kLightPosition - vertPosition.xyz );
	vec3 E = normalize( -vertPosition.xyz ); 
	vec3 R = normalize( -reflect(L,N) );

	// diffuse term
	vec4 diffuse = vertColor;
	diffuse *= max( dot( N, L ), 0.0 );    

	// specular term
	const float kShininess = 25.0;

	vec4 specular = vec4(0.5, 0.5, 0.5, 1.0); 
	specular *= pow( max(dot(R,E),0.0), kShininess );

	// final color 
	fragColor = diffuse + specular;
}