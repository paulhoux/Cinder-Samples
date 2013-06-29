#version 120

uniform sampler2D texture;

uniform float sides = 4.0;				// = number of 90-degree FOV render views
uniform float radians = 6.28318530;		// = 0.5 * sides * PI
uniform float reciprocal = 0.125;		// = 0.5 / sides

void main()
{
	/*	Official method to perform cylindrical projection
		based on code by Robert Hodgin.
	*/

	float s			= gl_TexCoord[0].s;
	float t			= gl_TexCoord[0].t;

	float offset	= floor( s * sides ) / sides;
	float azimuth	= s - ( offset + reciprocal );
	float tangent	= tan( radians * azimuth );
	float distance	= sqrt( tangent * tangent + 1.0 ); 
	         
	s				= reciprocal * ( tangent + 1.0 ) + offset;
	t				= distance * ( t - 0.5 ) + 0.5;

	gl_FragColor	= texture2D( texture, vec2( s, t ) );

	/*	The method below is a visually more pleasing way
		to do the cylindrical projection, but won't work
		in an actual setup - only on a flat screen.
		The value "0.3" is chosen arbitrarily and has
		no basis in correct mathematics.
	*/

	/*// get unwarped texture coordinates
	float s = gl_TexCoord[0].s;
	float t = gl_TexCoord[0].t;

	// determine face offset
	float f = floor( s * sides ) / sides;	
	
	// convert to face coordinates
	vec2 coords = vec2( (s - f) * sides, t );
 
	// horizontal correction
	coords.x = tan( (coords.x - 0.5) * 1.57079632 ) * 0.5 + 0.5; 
	coords.x = mix( (s - f) * sides, coords.x, 1.0 );

	// vertical correction
	coords.y = (0.5 - t) * 0.3 * sin(coords.x * 3.14159265); // 0.3
	
	// convert back to texture coordinates	
	coords.x = (coords.x / sides) + f;
	coords.y = t + coords.y;

	// perform texture look-up
	gl_FragColor = texture2D( texture, coords );
	//*/
}
