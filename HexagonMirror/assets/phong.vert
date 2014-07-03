#version 120
#extension GL_EXT_draw_instanced : enable

uniform sampler2D	texture;
uniform vec2		scale;

attribute mat4		model_matrix;

varying vec4		V;
varying vec3		N;

void main()
{
	// get the texture coordinate for this instance
	vec2 st = vec2( 1.0 - model_matrix[3].x * scale.x, 1.0 - model_matrix[3].y * scale.y );

	// retrieve a single color per hexagon from the texture
	vec4 clr = texture2D( texture, st );

	// retrieve the luminance and convert it to an angle
	float luminance = dot( vec3(0.3, 0.59, 0.11), clr.rgb );
	float angle = acos( luminance );

	// create a rotation matrix, based on the luminance
	mat4 m;
	m[0][0] = 1.0;
	m[1][1] = cos(angle);	m[2][1] = -sin(angle);
	m[1][2] = sin(angle);	m[2][2] =  cos(angle);
	m[3][3] = 1.0;

	// calculate final vertex position in eye space
	V = gl_ModelViewMatrix * model_matrix * m * gl_Vertex;

	// do the same for the normal vector (note: this calculation is only correct if your model is uniformly scaled!)
	N = normalize( gl_NormalMatrix * vec3( model_matrix * m * vec4( gl_Normal, 0.0 ) ) );
	
	gl_Position = gl_ProjectionMatrix * V;
	gl_FrontColor = vec4(1, 1, 1, 1);
}
