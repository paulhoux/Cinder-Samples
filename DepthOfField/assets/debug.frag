#version 150

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
    float dist = length( vertPosition.xyz );
    
    float coc = uAperture * ( uFocalLength * ( uFocalDistance - dist ) ) / ( dist * ( uFocalDistance - uFocalLength ) );
    coc *= uMaxCoCRadiusPixels;
    coc = clamp( coc * 0.5 + 0.5, 0.0, 1.0 );

	fragColor.rgb = vertColor.rgb;
	fragColor.a = coc;
}