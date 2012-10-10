// adapted from: http://www.tartiflop.com/disp2norm/srcview/index.html

#version 120

uniform sampler2D texture;
uniform float amplitude;

float getDisplacement( float dx, float dy )
{
	vec2 uv = gl_TexCoord[0].xy;
	return texture2D( texture, uv + vec2( dFdx(uv.s) * dx, dFdy(uv.t) * dy ) ).r;
}

void main(void)
{
	// calculate first order centered finite difference
	vec3 normal;
	normal.x = -0.5 * (getDisplacement(1,0) - getDisplacement(-1,0));
	normal.z = -0.5 * (getDisplacement(0,1) - getDisplacement(0,-1));
	normal.y = 1.0 / amplitude;

	gl_FragColor = vec4( normal, 1.0 );
}