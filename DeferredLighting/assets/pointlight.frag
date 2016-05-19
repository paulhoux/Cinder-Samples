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

	// Calculate light vectors.
	vec3 E = normalize( -vertPosition.xyz );
	vec3 L = lightPosition.xyz - vertPosition.xyz;
	float distance = length( L );
    L /= distance;

    // Calculate basic attenuation.
	const float kRadius = 1.0;
    float d = max( distance - kRadius, 0.0 );
    float denom = d / kRadius + 1.0;
    float attenuation = 1.0 / ( denom * denom );
     
    // Scale and bias attenuation such that:
    //   attenuation == 0 at extent of max influence
    //   attenuation == 1 when d == 0
    const float kCutoff = 4.0 / 255.0;
    float intensity = lightColor.a;
    attenuation = ( attenuation - kCutoff ) / ( 1.0 - kCutoff );
    attenuation = max( attenuation, 0.0 );

    // Diffuse (Lambertian).
	float lambert = max( dot( N, L ), 0.0 );
	vec3  diffuse = lambert * attenuation * intensity * lightColor.rgb;

	// Specular (Blinn).
	const float kRoughness = 0.3;
	const float kRoughness2 = kRoughness * kRoughness;
    const float kRoughness4 = kRoughness2 * kRoughness2;
    const float kSpecularPower = 2.0 / kRoughness4 - 2.0;

	vec3 H = normalize( L + E );
	float NoH = max( dot( N, H ), 0.0 );
    float blinn = 0.2 * ( kSpecularPower + 2.0 ) / 2.0 * pow( NoH, kSpecularPower );

    vec3 specular = blinn * attenuation * intensity * lightColor.rgb;

    // Output final color.
	fragColor.rgb = diffuse + specular;
	fragColor.a = 1.0;
}