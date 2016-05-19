#version 150

uniform mat4 ciViewMatrix;
uniform mat4 ciProjectionMatrix;

in vec4 ciPosition;
in vec3 ciNormal;

in mat4 iModelMatrix; // per instance

out vec3 vertNormal; // in view space

void main( void )
{
	mat4 modelView = ciViewMatrix * iModelMatrix;
	
	mat3 normalMatrix = mat3( modelView ); // assumes homogeneous scaling.
	vertNormal = normalMatrix * ciNormal;

	gl_Position = ciProjectionMatrix * modelView * ciPosition;
}