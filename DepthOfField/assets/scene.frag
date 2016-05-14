#version 150

uniform sampler2D uTex;

uniform int   uMaxCoCRadiusPixels = 5;
uniform float uAperture = 1.0;
uniform float uFocalDistance = 5.0;
uniform float uFocalLength = 1.0;

in vec4 vertPosition; // in view space
in vec3 vertNormal; // in view space
in vec4 vertColor;

out vec4 fragColor;

void main()
{
    // simple matcap shading
    vec3 E = normalize( vertPosition.xyz );
    vec3 N = normalize( vertNormal );

    vec3  r = reflect( E, N );
    r.z += 1.0;

    float m = 2.0 * sqrt( dot( r, r ) );
    vec2 uv = r.xy / m + 0.5;
    
    fragColor.rgb = texture( uTex, uv ).rgb * vertColor.rgb;

    // store the normalized circle of confusion radius in the alpha channel
    float dist = length( vertPosition.xyz );
    float coc = uMaxCoCRadiusPixels * uAperture * ( uFocalLength * ( uFocalDistance - dist ) ) / ( dist * ( uFocalDistance - uFocalLength ) );
    coc = clamp( coc * 0.5 + 0.5, 0.0, 1.0 );

    fragColor.a = coc;
}