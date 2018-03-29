#version 150

in vec2 vertTexCoord0;
in vec4 vertColor;

out vec4 fragColor;

void main(void)
{
	// calculate glowing line strips based on texture coordinate
	const float kResolution = 256.0;
	const float kCenter = 0.5;
	const float kWidth = 0.2;

	float f = fract( kResolution * vertTexCoord0.x );
	float d = abs(kCenter - f);
	float strips = clamp(kWidth / d, 0.0, 1.0);

	// calculate fade based on texture coordinate
	float fade = vertTexCoord0.y;

	// calculate output color
	fragColor.rgb = vertColor.rgb * strips * fade;
	fragColor.a = 1.0;
}