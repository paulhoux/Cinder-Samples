#version 150

uniform float		uTime;
uniform float		uAmplitude;

in vec2 vTexCoord0;

out vec4 oColor;

float wave( float period )
{
	return sin( period * 6.283185 );
}

// calculate displacement based on uv coordinate
float displace( vec2 uv )
{
	// large up and down movement
	float d = wave( (uv.x * 0.5) - uTime * 0.01 );
	// add a large wave from left to right
	d -= 1.2 * wave( (uv.x * 0.9) - uTime * 0.04 );
	// add diagonal waves from back to front
	d -= 0.25 * wave( ((uv.x + uv.y) * 2.2) - uTime * 0.05 );
	// add additional waves for increased complexity
	d += 0.25 * wave( (uv.y * 1.2) - uTime * 0.01 );
	d -= 0.15 * wave( ((uv.y + uv.x) * 2.8) - uTime * 0.09 );
	d += 0.15 * wave( ((uv.y - uv.x) * 1.9) - uTime * 0.08 );

	return d;
}

void main()
{
	float d = uAmplitude * displace( vTexCoord0.xy );
	oColor = vec4( d, d, d, 1.0 );
}