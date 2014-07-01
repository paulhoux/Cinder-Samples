#version 120

uniform sampler2D iSrc;
uniform sampler2D iDst;
uniform float     iFade;

void main()
{
	vec4 srcColor = texture2D( iSrc, gl_TexCoord[0].st );
	vec4 dstColor = texture2D( iDst, gl_TexCoord[0].st );
	gl_FragColor = mix( srcColor, dstColor, iFade );
}