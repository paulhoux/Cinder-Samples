#version 120

uniform sampler2D displacement_map;

varying vec3 Nsurface;

void main()
{
	// lookup displacement in map
	float displacement = texture2D( displacement_map, gl_MultiTexCoord0.xy ).r;

	// now take the vertex and displace it along its normal
	vec4 V = gl_Vertex;
	V.x += gl_Normal.x * displacement;
	V.y += gl_Normal.y * displacement;
	V.z += gl_Normal.z * displacement;

	// pass the surface normal on to the fragment shader
	Nsurface = gl_Normal;

	// pass vertex and texture coordinate on to the fragment shader
	gl_Position = gl_ModelViewProjectionMatrix * V;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}