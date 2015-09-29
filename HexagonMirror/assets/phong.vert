#version 150

uniform mat4 ciModelView;
uniform mat4 ciProjectionMatrix;
uniform mat3 ciNormalMatrix;

uniform sampler2D	uTexture;
uniform vec2		uScale;

in vec4 ciPosition;
in vec3 ciNormal;
in vec4 ciColor;

in mat4 iModelMatrix;

out vec4 vertPosition;
out vec3 vertNormal;
out vec4 vertColor;

void main()
{
	// get the uTexture coordinate for this instance
	vec2 st = vec2( iModelMatrix[3].x * uScale.x, iModelMatrix[3].y * uScale.y );

	// retrieve a single color per hexagon from the uTexture
	vec4 clr = texture( uTexture, st );

	// retrieve the luminance and convert it to an angle
	float luminance = dot( vec3(0.3, 0.59, 0.11), clr.rgb );
	float angle = acos( luminance );

	// create a rotation matrix, based on the luminance
	float cosAngle = cos( angle );
	float sinAngle = sin( angle );

	mat4 m;
    m[0] = vec4(1.0,      0.0,       0.0, 0.0);
	m[1] = vec4(0.0, cosAngle, -sinAngle, 0.0);
	m[2] = vec4(0.0, sinAngle,  cosAngle, 0.0);
    m[3] = vec4(0.0,      0.0,       0.0, 1.0);

	// calculate final vertex position in eye space
	vertPosition = ciModelView * iModelMatrix * m * ciPosition;

	// do the same for the normal vector (note: this calculation is only correct if your model is uniformly scaled!)
	vertNormal = ciNormalMatrix * vec3( iModelMatrix * m * vec4( ciNormal, 0.0 ) );

	vertColor = ciColor;
	
	gl_Position = ciProjectionMatrix * vertPosition;
}
