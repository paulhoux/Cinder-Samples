#version 120

uniform float		time;
uniform sampler2D	sinus;

// fast sin() calculation using a lookup texture. \a radians = period * 2 * pi
float sinf( float period )
{
	return 2.0 * texture2D( sinus, vec2(period, 0.5) ).r - 1.0;
}

// calculate displacement based on uv coordinate
float displace( vec2 uv )
{
	float d = 1.5 * sinf( (uv.x * 0.5) - time * 0.03 );
	d *= sinf( (uv.x * 0.8) - time * 0.02 );
	d += 0.25 * sinf( ((uv.x + uv.y) * 1.1) - time * 0.05 );
	d += 0.25 * sinf( (uv.y * 1.2) - time * 0.01 );
	d += 0.1 * sinf( ((uv.y + uv.x) * 2.8) - time * 0.09 );
	d += 0.15 * sinf( ((uv.y - uv.x) * 1.9) - time * 0.08 );

	return 0.5 + 0.5 * d;
}

void main()
{
	float d = displace( gl_TexCoord[0].xy );
	gl_FragColor = vec4( d, d, d, 1.0 );
}