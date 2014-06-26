#version 110

uniform sampler2D	tex0;
uniform vec3 		pickingColor;

varying vec3 v;
varying vec3 N;

void main()
{
	vec2 uv = gl_TexCoord[0].st;

	vec3 L = normalize( vec3(0, -1, 1) );
	vec3 E = normalize(-v);
	vec3 R = normalize(-reflect(L,N));

	// ambient term
	vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0);

	// diffuse term with fake ambient occlusion
	float occlusion = 0.5 + 0.5*16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y);
	vec4 diffuse = gl_Color * occlusion;
	diffuse *= max(dot(N,L), 0.0);
	diffuse = clamp(diffuse, 0.0, 1.0);

	// specular term
	vec4 specular = diffuse;
	specular *= pow(max(dot(R,E),0.0), 50.0);
	specular = clamp(specular, 0.0, 1.0);

	// write final color in first color target
	gl_FragData[0] = ambient + diffuse + specular;

	// write color code in second color target
	//  note:  use texture2D(tex0, uv).a for the alpha value if you use textures
	gl_FragData[1] = vec4(pickingColor, 1.0);
}