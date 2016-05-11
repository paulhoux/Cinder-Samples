#version 150

uniform mat4 ciModelView;
uniform mat4 ciProjectionMatrix;
uniform mat3 ciNormalMatrix;

in vec4 ciPosition;
in vec3 ciNormal;

out vec4 vertPosition; // in view space
out vec3 vertNormal; // in view space

void main()
{
    vertPosition = ciModelView * ciPosition;
    vertNormal = normalize( ciNormalMatrix * ciNormal );

    gl_Position = ciProjectionMatrix * vertPosition;
}