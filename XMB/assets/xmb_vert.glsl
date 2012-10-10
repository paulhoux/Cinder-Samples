#version 120

uniform sampler2D displacement_map;

varying vec4 v;

void main()
{
	// lookup displacement in map
	float displacement = 15.0 * texture2D( displacement_map, gl_MultiTexCoord0.xy ).r;

	// now take the vertex and displace it along its normal
	v = gl_Vertex;
	v.x += gl_Normal.x * displacement;
	v.y += gl_Normal.y * displacement;
	v.z += gl_Normal.z * displacement;

	// pass it on to the fragment shader
	gl_Position = gl_ModelViewProjectionMatrix * v;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}