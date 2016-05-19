#version 150

uniform sampler2D uTexNormals;
uniform sampler2D uTexDepth;
uniform vec2      uNearFarClip;     // x = near clip, y = far clip
uniform vec2      uInvViewportSize; // x = 1/width, y = 1/height

uniform mat4 ciProjectionMatrixInverse;

in vec4 lightPosition; // in view space (xyz = position, w = radius)
in vec4 lightColor; // (rgb = color, a = intensity)

out vec4 fragColor;

vec3 decodeNormal( in vec3 n )
{
	return normalize( n * 2.0 - 1.0 );
}

float linearizeDepth( in float z )
{
	float n = uNearFarClip.x;
	float f = uNearFarClip.y;
	return 2.0 * n * f / ( f + n - z * ( f - n ) );
}

void main( void )
{
	ivec2 uv = ivec2( gl_FragCoord.xy );

	// Reconstruct view space position from depth.
	vec4 vertPosition;
	vertPosition.xy = gl_FragCoord.xy * uInvViewportSize.xy * 2.0 - 1.0;
	vertPosition.z = texelFetch( uTexDepth, uv, 0 ).r * 2.0 - 1.0;
	vertPosition.w = 1.0;

	vertPosition = ciProjectionMatrixInverse * vertPosition;
	vertPosition.xyz *= ( vertPosition.w == 0 ) ? 0.0 : 1.0 / vertPosition.w;

	// Sample normal from texture.
	vec3 N = decodeNormal( texelFetch( uTexNormals, uv, 0 ).rgb );

	// Simple lambert.
	const float kRadius = 1.0;
	vec3 L = lightPosition.xyz - vertPosition.xyz;
	float distance = length( L );
    float d = max( distance - kRadius, 0.0 );
    L /= distance;

    // calculate basic attenuation
    float denom = d / kRadius + 1.0;
    float attenuation = 1.0 / ( denom * denom );
     
    // scale and bias attenuation such that:
    //   attenuation == 0 at extent of max influence
    //   attenuation == 1 when d == 0
    const float kCutoff = 4.0 / 255.0;
    float intensity = lightColor.a;
    attenuation = ( attenuation - kCutoff ) / ( 1.0 - kCutoff );
    attenuation = max( attenuation, 0.0 );

	float lambert = max( dot( N, L ), 0.0 );

	fragColor.rgb = attenuation * lambert * intensity * lightColor.rgb;
	fragColor.a = 1.0;
}