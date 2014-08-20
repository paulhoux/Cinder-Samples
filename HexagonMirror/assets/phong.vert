#version 120
#extension GL_EXT_draw_instanced : enable

uniform sampler2D	texture;
uniform vec2		scale;

attribute mat4		model_matrix;

varying vec4		vVertex;
varying vec3		vNormal;

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
    m[0] = vec4(1.0,        0.0,         0.0, 0.0);
	m[1] = vec4(0.0, cos(angle), -sin(angle), 0.0);
	m[2] = vec4(0.0, sin(angle),  cos(angle), 0.0);
    m[3] = vec4(0.0,        0.0,         0.0, 1.0);

	// calculate final vertex position in eye space
	vVertex = gl_ModelViewMatrix * model_matrix * m * gl_Vertex;

	// do the same for the normal vector (note: this calculation is only correct if your model is uniformly scaled!)
	vNormal = gl_NormalMatrix * vec3( model_matrix * m * vec4( gl_Normal, 0.0 ) );
	
	gl_Position = gl_ProjectionMatrix * vVertex;
	gl_FrontColor = vec4(1, 1, 1, 1);
}
