#version 150

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelView;
uniform mat3 ciNormalMatrix;

in vec4 ciPosition;
in vec3 ciNormal;
in vec2 ciTexCoord0;
in vec3 ciColor;

out vec4 vPosition;
out vec3 vNormal;
out vec2 vTexCoord0;
out vec3 vColor;

void main()
{
	vPosition = ciModelView * ciPosition;
	vNormal = ciNormalMatrix * ciNormal;
	vTexCoord0 = ciTexCoord0;
	vColor = ciColor;

	gl_Position = ciModelViewProjection * ciPosition;
}
