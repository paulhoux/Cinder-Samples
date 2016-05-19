#version 150

in vec3 vertNormal; // in view space

out vec4 fragColor;

vec3 encodeNormal( in vec3 n )
{
	return n * 0.5 + 0.5;
}

void main( void )
{
	fragColor.rgb = encodeNormal( normalize( vertNormal ) );
	fragColor.a = 1.0;
}