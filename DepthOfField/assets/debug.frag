#version 150

in vec4 vertPosition; // in view space
in vec3 vertNormal; // in view space
in vec4 vertColor;

out vec4 fragColor;

void main()
{
	fragColor = vertColor;
}