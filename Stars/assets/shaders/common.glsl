
#define saturate(x) clamp( x, 0.0, 1.0 )

float log10( in float n ) {
	const float kLogBase10 = 1.0 / log2( 10.0 );
	return log2( n ) * kLogBase10;
}

// Returns the apparent magnitude based on absolute magnitude m and distance d (in parsecs)	
float apparentMagnitude( in float m, in float d )
{
	return m - 5.0 * ( 1.0 - log10( d ) );
}

// Returns an appropriate size based on apparent magnitude. Not scientifically correct.
float starSize( in float m, in float size, in float modifier )
{
	return size * pow( modifier, 1.0 - m ); 
}

// Returns an appropriate brightness based on apparent magnitude. Not scientifically correct.
float starBrightness( in float m, in float lower, in float upper )
{
	return pow( saturate( ( lower + ( 1.0 - m ) ) / ( upper + lower ) ), 1.5 );
}

vec3 toNDC( in vec4 v )
{
	float w = (v.w == 0.0) ? 0.0 : 1.0 / v.w;
	return v.xyz * w;
}

vec2 toScreenSpace( in vec4 v, in vec2 viewportSize )
{
	vec2 w = (v.w == 0.0) ? vec2( 0.0 ) : viewportSize / v.w;
	return v.xy * w;
}

