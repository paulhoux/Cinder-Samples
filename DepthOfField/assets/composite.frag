#version 150

#define saturate(s) clamp( s, 0.0, 1.0 )

uniform sampler2D packedBuffer;
uniform vec2      packedBufferInvSize;
uniform sampler2D blurBuffer;
uniform sampler2D nearBuffer;
uniform vec2      shift = vec2( 0.0 );

uniform float     farRadiusRescale = 1.0;

uniform int		  debugOption = 0;

out vec3 result;


const vec2 cocReadScaleBias = vec2(2.0, -1.0);

// Boost the coverage of the near field by this factor.  Should always be >= 1
//
// Make this larger if near-field objects seem too transparent
//
// Make this smaller if an obvious line is visible between the near-field blur and the mid-field sharp region
// when looking at a textured ground plane.
const float coverageBoost = 1.5;

void main() {
    ivec2 A         = ivec2(gl_FragCoord.xy - shift);

    vec4 pack    = texelFetch(packedBuffer, A, 0);
    vec3 sharp   = pack.rgb;
    vec3 blurred = texture(blurBuffer, gl_FragCoord.xy * packedBufferInvSize).rgb;
    vec4 near    = texture(nearBuffer, gl_FragCoord.xy * packedBufferInvSize);

    // Signed, normalized radius of the circle of confusion.
    // |normRadius| == 1.0 corresponds to camera->maxCircleOfConfusionRadiusPixels()
    float normRadius = pack.a * cocReadScaleBias.x + cocReadScaleBias.y;

    // Fix the far field scaling factor so that it remains independent of the 
    // near field settings
    normRadius *= (normRadius < 0.0) ? farRadiusRescale : 1.0;

    // Boost the blur factor
    //normRadius = clamp(normRadius * 2.0, -1.0, 1.0);

    if (coverageBoost != 1.0) {
        float a = saturate(coverageBoost * near.a);
        near.rgb = near.rgb * (a / max(near.a, 0.001f));
        near.a = a;
    }

    // Decrease sharp image's contribution rapidly in the near field
    // (which has positive normRadius)
    if (normRadius > 0.1) {
        normRadius = min(normRadius * 1.5, 1.0);
    }

    // Two mixs, the second of which has a premultiplied alpha
    result = mix(sharp, blurred, abs(normRadius)) * (1.0 - near.a) + near.rgb;
    
    /////////////////////////////////////////////////////////////////////////////////
    // Debugging options:
    const int SHOW_COC          = 1;
    const int SHOW_REGION       = 2;
    const int SHOW_NEAR         = 3;
    const int SHOW_BLURRY       = 4;
    const int SHOW_INPUT        = 5;
    const int SHOW_MID_AND_FAR  = 6;
    const int SHOW_SIGNED_COC   = 7;


    switch (debugOption) {
    case SHOW_COC:
        // Go back to the true radius, before it was enlarged by post-processing
        result.rgb = vec3(abs(pack.a * cocReadScaleBias.x + cocReadScaleBias.y));
        break;

    case SHOW_SIGNED_COC:
        {
            // Go back to the true radius, before it was enlarged by post-processing
            float r = pack.a * cocReadScaleBias.x + cocReadScaleBias.y;
            if (r < 0) {
                result.rgb = vec3(0.0, 0.14, 0.8) * abs(r);
            } else {
                result.rgb = vec3(1.0, 1.0, 0.15) * abs(r);
            }
        }
        break;

    case SHOW_REGION:
        if (pack.a < 0.49) {
            // Far field: Dark blue
            result.rgb = vec3(0.0, 0.07, 0.4) * (dot(sharp, vec3(1.0 / 3.0)) * 0.7 + 0.3);
        } else if (pack.a <= 0.51) {
            // Mifield: Gray
            result.rgb = vec3(0.4) * (dot(sharp, vec3(1.0 / 3.0)) * 0.7 + 0.3);
        } else {
            result.rgb = vec3(1.0, 1.0, 0.15) * (dot(sharp, vec3(1.0 / 3.0)) * 0.7 + 0.3);
        }
        break;

    case SHOW_BLURRY:
        result = blurred;
        break;

    case SHOW_NEAR:
        result = near.rgb;
        break;

    case SHOW_INPUT:
        result = sharp;
        break;

    case SHOW_MID_AND_FAR:
        // Just mix based on this pixel's blurriness. Works well in the background, less well in the foreground
        result = mix(sharp, blurred, abs(normRadius));
        break;
    }
}
