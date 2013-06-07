#version 110

uniform sampler2D tex_diffuse;
uniform sampler2D tex_specular;

varying vec3 v;
varying vec3 N;

void main()
{	
	float shinyness = 25.0;
	
	vec3 L = normalize(gl_LightSource[0].position.xyz - v);   
	vec3 E = normalize(-v); 
	vec3 R = normalize(-reflect(L,N));  

	// ambient term 
	vec4 Iamb = gl_FrontLightProduct[0].ambient;    

	// diffuse term
	vec4 Idiff = texture2D( tex_diffuse, gl_TexCoord[0].st) * gl_FrontLightProduct[0].diffuse; 
	Idiff *= max(dot(N,L), 0.0);
	Idiff = clamp(Idiff, 0.0, 1.0);     

	// specular term
	vec4 Ispec = texture2D( tex_specular, gl_TexCoord[0].st);
	Ispec *= pow(max(dot(R,E),0.0), shinyness);
	Ispec = clamp(Ispec, 0.0, 1.0); 

	// final color 
	gl_FragColor = (Iamb + Idiff + Ispec);
}