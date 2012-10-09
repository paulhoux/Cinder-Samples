#version 120

uniform sampler2D displacement_map;

void main()
{
	// lookup displacement in map
	float displacement = 2.0 * texture2D( displacement_map, gl_MultiTexCoord0.xy ).r - 1.0;

	// now take the vertex and displace it along its normal
	vec4 v = gl_Vertex;
	v.x += gl_Normal.x * displacement * 10.0;
	v.y += gl_Normal.y * displacement * 10.0;
	v.z += gl_Normal.z * displacement * 10.0;

	// pass it on to the fragment shader
	gl_Position = gl_ModelViewProjectionMatrix * v;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}