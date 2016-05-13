#version 150

uniform sampler2D uTex;

uniform int uMaxCoCRadiusPixels = 5;
uniform float uAperture = 1.0;
uniform float uFocalDistance = 5.0;
uniform float uFocalLength = 1.0;

in vec4 vertPosition; // in view space
in vec3 vertNormal; // in view space

out vec4 fragColor;

void main()
{
    vec3 E = normalize( vertPosition.xyz );
    vec3 N = normalize( vertNormal );

    vec3  r = reflect( E, N );
    r.z += 1.0;

    float m = 2.0 * sqrt( dot( r, r ) );
    vec2 uv = r.xy / m + 0.5;

    float dist = length( vertPosition.xyz );
    
    float coc = uAperture * ( uFocalLength * ( uFocalDistance - dist ) ) / ( dist * ( uFocalDistance - uFocalLength ) );
    coc *= uMaxCoCRadiusPixels;
    coc = clamp( coc * 0.5 + 0.5, 0.0, 1.0 );

    //const float kScale = 0.1;
    //float coc = ( uFocalDistance - dist ) * kScale;
    //coc = clamp( coc * 0.5 + 0.5, 0.0, 1.0 );

    fragColor = vec4( texture( uTex, uv ).rgb, coc );
}