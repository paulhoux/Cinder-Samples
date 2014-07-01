#version 120

uniform sampler2D iSrc;
uniform sampler2D iDst;
uniform float     iFade;

void main()
{
	vec4 srcColor = texture2D( iSrc, gl_TexCoord[0].st );
	vec4 dstColor = texture2D( iDst, gl_TexCoord[0].st );
	
	float wipe = pow( clamp(2.0 * gl_TexCoord[0].s + (3.0 * iFade - 2.0), 0.0, 1.0), 20.0);
	gl_FragColor = mix( srcColor, dstColor, wipe );
}