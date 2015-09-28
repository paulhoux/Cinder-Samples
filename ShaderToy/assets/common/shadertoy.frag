#version 150

uniform sampler2D iSrc;
uniform sampler2D iDst;
uniform float     iFade;

in vec4 vertColor;
in vec2 vertTexCoord0;

out vec4 fragColor;

void main()
{
	vec4 srcColor = texture( iSrc, vertTexCoord0 );
	vec4 dstColor = texture( iDst, vertTexCoord0 );
	
	float wipe = pow( clamp(2.0 * vertTexCoord0.x + (3.0 * iFade - 2.0), 0.0, 1.0), 20.0);
	fragColor = mix( srcColor, dstColor, wipe );
}