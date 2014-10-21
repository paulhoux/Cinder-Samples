// adapted from: http://www.tartiflop.com/disp2norm/srcview/index.html
// note: I have omitted the second and third order processing, because this
//       would require branching, which would stall the GPU needlessly.
//       You will notice incorrect normals at the edges of the normal map.

#version 150

uniform sampler2D uTex0;
uniform float uAmplitude;

in vec2 vTexCoord0;

out vec4 oColor;

float getDisplacement( float dx, float dy )
{
	return texture( uTex0, vTexCoord0 + vec2( dFdx( vTexCoord0.x ) * dx, dFdy( vTexCoord0.y ) * dy ) ).r;
}

void main(void)
{
	// calculate first order centered finite difference (y-direction)
	vec3 normal;
	normal.x = -0.5 * (getDisplacement(1,0) - getDisplacement(-1,0));
	normal.z = -0.5 * (getDisplacement(0,1) - getDisplacement(0,-1));
	normal.y = 1.0 / uAmplitude;
	normal = normalize(normal);

	oColor = vec4( normal, 1.0 );
}