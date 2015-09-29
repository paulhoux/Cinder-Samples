#version 150

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec4 ciColor;
in vec2 ciTexCoord0;

out vec4 vertColor;
out vec2 vertTexCoord0;

void main()
{
	vertColor = ciColor;
	vertTexCoord0 = ciTexCoord0;
	gl_Position = ciModelViewProjection * ciPosition;
}