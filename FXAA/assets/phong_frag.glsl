#version 110

uniform bool bLuminanceInAlpha;	

varying vec3 v;
varying vec3 N;

void main()
{
	vec2 uv = gl_TexCoord[0].st;

	vec3 L = normalize(-v);
	vec3 E = normalize(-v);
	vec3 R = normalize(-reflect(L,N));

	// ambient term
	vec4 ambient = vec4(0.0, 0.0, 0.0, 1.0);

	// diffuse term with fake ambient occlusion
	float occlusion = 0.5 + 0.5*16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y);
	vec4 diffuse = gl_Color * occlusion;
	diffuse *= max(dot(N,L), 0.0);
	diffuse = clamp(diffuse, 0.0, 1.0);

	// specular term
	vec4 specular = diffuse;
	specular *= pow(max(dot(R,E),0.0), 50.0);
	specular = clamp(specular, 0.0, 1.0);

	// write gamma corrected final color
	vec3 final = sqrt(ambient + diffuse + specular).rgb;
	gl_FragColor.rgb = final;

	// write luminance in alpha channel (required for optimal FXAA)
	const vec3 luminance = vec3(0.299, 0.587, 0.114);
	gl_FragColor.a = bLuminanceInAlpha ? dot( final, luminance ) : 1.0;
}