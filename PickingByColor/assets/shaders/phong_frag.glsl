#version 110

uniform sampler2D	tex0;
uniform vec3 		pickingColor;

varying vec3 v;
varying vec3 N;

void main()
{		
	vec2 uv = gl_TexCoord[0].st;
	
	vec3 L = normalize(gl_LightSource[0].position.xyz - v);   
	vec3 E = normalize(-v); 
	vec3 R = normalize(-reflect(L,N));  

	// ambient term 
	vec4 ambient = gl_FrontLightProduct[0].ambient;    

	// diffuse term
	vec4 diffuse = gl_FrontLightProduct[0].diffuse; // or texture2D(tex0, uv) if you use textures
	diffuse *= max(dot(N,L), 0.0);
	diffuse = clamp(diffuse, 0.0, 1.0);     

	// specular term
	vec4 specular = gl_FrontLightProduct[0].specular; 
	specular *= pow(max(dot(R,E),0.0), gl_FrontMaterial.shininess);
	specular = clamp(specular, 0.0, 1.0); 

	// write final color in first color target 
	gl_FragData[0] = ambient + diffuse + specular;	

	// write color code in second color target
	//  note:  use texture2D(tex0, uv).a for the alpha value if you use textures
	gl_FragData[1] = vec4(pickingColor, 1.0);
}