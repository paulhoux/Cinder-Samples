#version 110

void main(void)
{
	// calculate glowing line strips based on texture coordinate
	const float resolution = 256.0;
	const float center = 0.5;
	const float width = 0.02;

	float f = fract( resolution * gl_TexCoord[0].s );
	float d = abs(center - f);
	float strips = clamp(width / d, 0.0, 1.0);

	// calculate fade based on texture coordinate
	float fade = gl_TexCoord[0].y;

	// calculate output color
	gl_FragColor.rgb = gl_Color.rgb * strips * fade;
	gl_FragColor.a = 1.0;
}