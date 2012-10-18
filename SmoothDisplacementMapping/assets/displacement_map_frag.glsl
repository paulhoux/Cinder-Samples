#version 120

uniform float		time;
uniform float		amplitude;
uniform sampler2D	sinus;

// fast sin() calculation using a lookup texture. \a radians = period * 2 * pi
float sinf( float period )
{
	return 2.0 * texture2D( sinus, vec2(period, 0.5) ).r - 1.0;
}

// calculate displacement based on uv coordinate
float displace( vec2 uv )
{
	// large up and down movement
	float d = sinf( (uv.x * 0.5) - time * 0.01 );
	// add a large wave from left to right
	d -= 1.2 * sinf( (uv.x * 1.2) - time * 0.03 );
	// add additional waves for increased complexity
	d -= 0.15 * sinf( (uv.x * 1.3 + uv.y * 2.2) - time * 0.02 );
	d += 0.20 * sinf( uv.y * 1.2 - time * 0.01 );
	d += 0.25 * sinf( (uv.x - uv.y * 2.9) - time * 0.08 );
	d -= 0.10 * sinf( (uv.y + uv.x * 2.8) - time * 0.04 );

	return d;
}

void main()
{
	float d = amplitude * displace( gl_TexCoord[0].xy );
	gl_FragColor = vec4( d, d, d, 1.0 );
}