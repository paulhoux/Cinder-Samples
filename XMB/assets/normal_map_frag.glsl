#version 120

uniform sampler2D texture;

void main(void)
{
	vec2 d = vec2(dFdx(gl_TexCoord[0].s), dFdy(gl_TexCoord[0].t));

	float tl = texture2D(texture, gl_TexCoord[0].st + d * vec2(-1.0, -1.0)).x;  // top left
	float l = texture2D(texture, gl_TexCoord[0].st + d * vec2(-1.0, 0.0)).x;  // left
	float bl = texture2D(texture, gl_TexCoord[0].st + d * vec2(-1.0, 1.0)).x;  // bottom left
	float t = texture2D(texture, gl_TexCoord[0].st + d * vec2( 0.0, -1.0)).x;  // top
	float b = texture2D(texture, gl_TexCoord[0].st + d * vec2( 0.0, 1.0)).x;  // bottom
	float tr = texture2D(texture, gl_TexCoord[0].st + d * vec2( 1.0, -1.0)).x;  // top right
	float r = texture2D(texture, gl_TexCoord[0].st + d * vec2( 1.0, 0.0)).x;  // right
	float br = texture2D(texture, gl_TexCoord[0].st + d * vec2( 1.0, 1.0)).x;  // bottom right

	// Compute dx using Sobel 
	float dX = tr + 2.0*r + br -tl - 2.0*l - bl;

	// Compute dy using Sobel
	float dY = bl + 2.0*b + br -tl - 2.0*t - tr;

	vec4 N = vec4( normalize( vec3(dX, 0.05, dY) ), 1.0 );

	N *= 0.5;
	N += 0.5;

	gl_FragColor = N;
}