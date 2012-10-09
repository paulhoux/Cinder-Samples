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

void main()
{
	// get the texture coordinate of this vertex
	vec2 t = gl_MultiTexCoord0.xy;

	// calculate an offset by adding multiple sin waves 
	// (most numbers were chosen arbitrarily, so feel free to play with them)
	float wave = sinf( (t.x * 0.4) + time * 0.04 );
	wave += sinf( (t.x * 0.7) + time * 0.04 );
	wave += 0.25 * sinf( ((t.x + t.y) * 1.1) + time * 0.05 );
	wave += 0.25 * sinf( (t.y * 1.2) + time * 0.01 );
	wave += 0.1 * sinf( ((t.y + t.x) * 2.8) + time * 0.09 );
	wave += 0.15 * sinf( ((t.y - t.x) * 1.9) + time * 0.08 );

	// apply additional Perlin nois to the wave
	wave += 2.0 * noisef( t.x * 0.7 + time * 0.02, t.y * 0.1 - time * 0.002 );

	// adjust amplitude of the displacement
	wave *= 10.0f;

	// now take the vertex and displace it along its normal
	vec4 v = gl_Vertex;
	v.x += gl_Normal.x * wave;
	v.y += gl_Normal.y * wave;
	v.z += gl_Normal.z * wave;

	// pass it on to the geometry shader
	gl_Position = gl_ModelViewProjectionMatrix * v;
}