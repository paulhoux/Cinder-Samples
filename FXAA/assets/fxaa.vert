#version 150

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec2 ciTexCoord0;
in vec4 ciColor;

out vec2 vertTexCoord0;
out vec4 vertColor;

void main()
{
	vertTexCoord0 = ciTexCoord0;
	vertColor = ciColor;

	gl_Position = ciModelViewProjection * ciPosition;
}
