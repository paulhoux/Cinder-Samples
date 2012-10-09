#version 120

uniform float		time;
uniform sampler2D	sinus;
uniform sampler2D	noise;

// fast sin() calculation using a lookup texture. \a period equals (radians / 2*pi)
float sinf( float period )
{
	return 2.0 * texture2D( sinus, vec2(period, 0.5) ).r - 1.0;
}

// fast noise() calculation using a lookup texture.
float noisef( float x, float y )
{
	return 2.0 * texture2D( noise, vec2(x, y) ).r - 1.0;
}

// calculate displacement based on uv coordinate
float displace( vec2 uv )
{
	float d = sinf( (uv.x * 0.4) + time * 0.04 );
	d += sinf( (uv.x * 0.7) + time * 0.04 );
	d += 0.25 * sinf( ((uv.x + uv.y) * 1.1) + time * 0.05 );
	d += 0.25 * sinf( (uv.y * 1.2) + time * 0.01 );
	d += 0.1 * sinf( ((uv.y + uv.x) * 2.8) + time * 0.09 );
	d += 0.15 * sinf( ((uv.y - uv.x) * 1.9) + time * 0.08 );

	// apply additional Perlin noise to the wave
	d += 2.0 * noisef( uv.x * 0.7 + time * 0.02, uv.y * 0.1 - time * 0.002 );

	// adjust amplitude of the displacement
	d *= 0.5f;

	return d;
}

void main()
{
	float d = 0.5 + 0.5 * displace( gl_TexCoord[0].xy );
	gl_FragColor = vec4( d, d, d, 1.0 );
}