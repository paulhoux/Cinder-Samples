// adapted from: http://www.tartiflop.com/disp2norm/srcview/index.html
// note: I have omitted the second and third order processing, because this
//       would require branching, which would stall the GPU needlessly.
//       You will notice incorrect normals at the edges of the normal map.

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
	// calculate first order centered finite difference (y-direction)
	vec3 normal;
	normal.x = -0.5 * (getDisplacement(1,0) - getDisplacement(-1,0));
	normal.z = -0.5 * (getDisplacement(0,1) - getDisplacement(0,-1));
	normal.y = 1.0 / amplitude;
	normal = normalize(normal);

	gl_FragColor = vec4( normal, 1.0 );
}