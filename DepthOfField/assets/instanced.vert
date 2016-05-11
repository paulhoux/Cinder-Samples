#version 150

uniform mat4 ciViewMatrix;
uniform mat4 ciProjectionMatrix;

in vec4 ciPosition;
in vec3 ciNormal;

in mat4 vInstanceMatrix; // per instance

out vec4 vertPosition; // in view space
out vec3 vertNormal; // in view space

void main()
{
    vertPosition = ciViewMatrix * vInstanceMatrix * ciPosition;

    mat3 normalMatrix = mat3( ciViewMatrix * vInstanceMatrix ); // assumes homogenous scaling.
    vertNormal = normalize( normalMatrix * ciNormal );

    gl_Position = ciProjectionMatrix * vertPosition;
}