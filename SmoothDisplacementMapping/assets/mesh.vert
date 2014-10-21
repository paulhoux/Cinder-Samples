#version 150

uniform sampler2D uTexDisplacement;

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec3 ciNormal;
in vec3 ciColor;
in vec2 ciTexCoord0;

out vec3 vNormal;
out vec3 vColor;
out vec2 vTexCoord0;

void main()
{
	// lookup displacement in map
	float displacement = texture( uTexDisplacement, ciTexCoord0.xy ).r;

	// now take the vertex and displace it along its normal
	vec4 displacedPosition = ciPosition;
	displacedPosition.xyz += ciNormal * displacement;

	// pass the surface normal and texture coordinate on to the fragment shader
	vNormal = ciNormal;
	vTexCoord0 = ciTexCoord0;

	// pass vertex on to the fragment shader
	gl_Position = ciModelViewProjection * displacedPosition;
}