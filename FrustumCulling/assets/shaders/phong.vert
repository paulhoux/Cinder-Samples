#version 150

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelView;
uniform mat3 ciNormalMatrix;

in vec4 ciPosition;
in vec3 ciNormal;

out vec4 vViewPosition;
out vec3 vNormal;

void main()
{
	vViewPosition = ciModelView * ciPosition;
	vNormal = ciNormalMatrix * ciNormal;

	gl_Position = ciModelViewProjection * ciPosition;
}
