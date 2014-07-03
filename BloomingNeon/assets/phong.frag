#version 110

uniform sampler2D tex_diffuse;
uniform sampler2D tex_specular;

varying vec3 v;
varying vec3 N;

void main()
{	
	const float shinyness = 5.0;
	const vec4  light_position = vec4(0, 5, 0, 1);
	const vec4  light_diffuse = vec4(1, 1, 1, 1);
	const vec4  light_specular = vec4(1, 1, 1, 1);
	
	vec3 L = normalize(light_position.xyz - v);
	vec3 E = normalize(-v); 
	vec3 R = normalize(-reflect(L,N));  

	// ambient term 
	vec4 Iamb = vec4(0, 0, 0, 1);

	// diffuse term
	vec4 Idiff = texture2D( tex_diffuse, gl_TexCoord[0].st) * light_diffuse; 
	Idiff *= max(dot(N,L), 0.0);
	Idiff = clamp(Idiff, 0.0, 1.0);

	// specular term
	vec4 Ispec = texture2D( tex_specular, gl_TexCoord[0].st) * light_specular;
	Ispec *= pow(max(dot(R,E),0.0), shinyness);
	Ispec = clamp(Ispec, 0.0, 1.0); 

	// final color 
	gl_FragColor = (Iamb + Idiff + Ispec);
}