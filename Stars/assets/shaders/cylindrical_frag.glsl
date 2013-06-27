#version 110

uniform sampler2D panoTex;

uniform float radians;
uniform float numCameras;
uniform float invNumCamsHalf;

void main()
{
	float sCoord	= gl_TexCoord[0].s;
	float sOff		= floor( sCoord * numCameras ) / numCameras;
	float azim		= sCoord - ( sOff + invNumCamsHalf );
	float tanVar	= tan( radians * azim );
	float dist		= sqrt( tanVar * tanVar + 1.0 );          
	float s			= ( tanVar + 1.0 ) * invNumCamsHalf + sOff;
	float t			= ( gl_TexCoord[0].t - 0.5 ) * dist + 0.5;
	gl_FragColor	= texture2D( panoTex, vec2( s, t ) );
}
