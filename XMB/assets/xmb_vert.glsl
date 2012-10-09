#version 120

uniform float		time;
uniform sampler2D	sinus;
uniform sampler2D	noise;

float sinf( float period )
{
	return 2.0 * texture2D( sinus, vec2(period, 0.5) ).r - 1.0;
}

float noisef( float x, float y )
{
	return 2.0 * texture2D( noise, vec2(x, y) ).r - 1.0;
}

void main()
{
	vec2 t = gl_MultiTexCoord0.xy;

	float wave = sinf( (t.x * 0.5) + time * 0.05 );
	wave += 0.75 * sinf( ((t.y + t.x) * 0.6) + time * 0.03 );
	wave += 0.15 * sinf( (t.y * 2.2) + time * 0.01 );
	wave += 0.025 * sinf( ((t.y + t.x) * 5.8) + time * 0.09 );
	//wave += 0.015 * sinf( ((t.y - t.x) * 3.9) + time * 0.08 );

	wave += 2.9 * noisef( t.x * 0.3 + time * 0.01, t.y * 0.1 + time * 0.02 );

	vec4 v = gl_Vertex;
	v.y += 10.0 * wave;

	gl_Position = gl_ModelViewProjectionMatrix * v;
}