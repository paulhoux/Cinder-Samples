#version 150

uniform sampler2D uSourceTex;
uniform float     uColorDepth = 31.0; // 2^bits - 1

in vec2 vertTexCoord0;

out vec4 fragColor;

const int kPatternSize = 4;
const int kPattern[16] = int[]( 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 );

float dither( in int x, in int y, in float c )
{
    float limit = kPattern[ x + kPatternSize * y + 1 ] / float( kPatternSize * kPatternSize );
    float dither = clamp( c, 0.0, 1.0 ) * uColorDepth;
    return mix( floor( dither ), ceil( dither ), step( limit, fract( dither ) ) ) / uColorDepth;
}

void main( void )
{
    fragColor.rgb = texture( uSourceTex, vertTexCoord0 ).rgb;

    vec2 xy = gl_FragCoord.xy;
    int x = int( mod( xy.x, kPatternSize ) );
    int y = int( mod( xy.y, kPatternSize ) );

    fragColor.r = dither( x, y, fragColor.r );
    fragColor.g = dither( x, y, fragColor.g );
    fragColor.b = dither( x, y, fragColor.b );
    fragColor.a = 1.0;
}