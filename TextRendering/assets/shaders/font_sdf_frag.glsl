#version 110

uniform sampler2D	tex0;

uniform bool		isItalic;
uniform bool		isBold;

// if the font edges become too fuzzy,
// try a lower value for smoothness
const float smoothness = 64.0;

const float gamma = 2.2;

void main()
{	
	// retrieve signed distance
	vec4 clr = texture2D( tex0, gl_TexCoord[0].xy );
	float sdf = (isItalic) ? ((isBold) ? clr.a : clr.g) : ((isBold) ? clr.b : clr.r);

	// perform adaptive anti-aliasing of the edges 
	float w = clamp( smoothness * (abs(dFdx(gl_TexCoord[0].x)) + abs(dFdy(gl_TexCoord[0].y))), 0.0, 0.5);
	float a = smoothstep(0.5-w, 0.5+w, sdf);

	// gamma correction for linear attenuation
	a = pow(a, 1.0/gamma);

	// final color
	gl_FragColor.rgb = gl_Color.rgb;
	gl_FragColor.a = a;
}