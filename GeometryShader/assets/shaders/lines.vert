#version 150

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec3 ciColor;

out VertexData{
	vec3 mColor;
} VertexOut;

void main(void)
{
	VertexOut.mColor = ciColor;
	gl_Position = ciModelViewProjection * ciPosition;
}