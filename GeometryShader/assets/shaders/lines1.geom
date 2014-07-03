// ------------------ Geometry Shader --------------------------------
uniform float	THICKNESS;		// the thickness of the line in pixels
uniform float	MITER_LIMIT;	// 1.0: always miter, -1.0: never miter, 0.75: default
uniform vec2	WIN_SCALE;		// the size of the viewport in pixels

varying out vec2 gsTexCoord;

vec2 screen_space(vec4 vertex)
{
	return vec2( vertex.xy / vertex.w ) * WIN_SCALE;
}

void main(void)
{
  // get the four vertices passed to the shader:
  vec2 p0 = screen_space( gl_PositionIn[0] );	// start of previous segment
  vec2 p1 = screen_space( gl_PositionIn[1] );	// end of previous segment, start of current segment
  vec2 p2 = screen_space( gl_PositionIn[2] );	// end of current segment, start of next segment
  vec2 p3 = screen_space( gl_PositionIn[3] );	// end of next segment

  // perform naive culling
  vec2 area = WIN_SCALE * 1.2;
  if( p1.x < -area.x || p1.x > area.x ) return;
  if( p1.y < -area.y || p1.y > area.y ) return;
  if( p2.x < -area.x || p2.x > area.x ) return;
  if( p2.y < -area.y || p2.y > area.y ) return;
  
  // determine the direction of each of the 3 segments (previous, current, next)
  vec2 v0 = normalize(p1-p0);
  vec2 v1 = normalize(p2-p1);
  vec2 v2 = normalize(p3-p2);

  // determine the normal of each of the 3 segments (previous, current, next)
  vec2 n0 = vec2(-v0.y, v0.x);
  vec2 n1 = vec2(-v1.y, v1.x);
  vec2 n2 = vec2(-v2.y, v2.x);

  // determine miter lines by averaging the normals of the 2 segments
  vec2 miter_a = normalize(n0 + n1);	// miter at start of current segment
  vec2 miter_b = normalize(n1 + n2);	// miter at end of current segment

  // determine the length of the miter by projecting it onto normal and then inverse it
  float length_a = THICKNESS / dot(miter_a, n1);
  float length_b = THICKNESS / dot(miter_b, n1);
  
  // prevent excessively long miters at sharp corners
  if( dot(v0,v1) < -MITER_LIMIT ) {
	miter_a = n1;
	length_a = THICKNESS;
	
	// close the gap
	if( dot(v0,n1) > 0 ) {
		gsTexCoord = vec2(0, 0);
		gl_FrontColor = gl_FrontColorIn[1];
		gl_Position = vec4( (p1 + THICKNESS * n0) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();
		gsTexCoord = vec2(0, 0);
		gl_FrontColor = gl_FrontColorIn[1];
		gl_Position = vec4( (p1 + THICKNESS * n1) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();
		gsTexCoord = vec2(0, 0.5);
		gl_FrontColor = gl_FrontColorIn[1];
		gl_Position = vec4( p1 / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();
		EndPrimitive();
	}
	else {
		gsTexCoord = vec2(0, 1);
		gl_FrontColor = gl_FrontColorIn[1];
		gl_Position = vec4( (p1 - THICKNESS * n1) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();		
		gsTexCoord = vec2(0, 1);
		gl_FrontColor = gl_FrontColorIn[1];
		gl_Position = vec4( (p1 - THICKNESS * n0) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();
		gsTexCoord = vec2(0, 0.5);
		gl_FrontColor = gl_FrontColorIn[1];
		gl_Position = vec4( p1 / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();
		EndPrimitive();
	}
  }

  if( dot(v1,v2) < -MITER_LIMIT ) {
	miter_b = n1;
	length_b = THICKNESS;
  }
  
  // generate the triangle strip
  gsTexCoord = vec2(0, 0);
  gl_FrontColor = gl_FrontColorIn[1];
  gl_Position = vec4( (p1 + length_a * miter_a) / WIN_SCALE, 0.0, 1.0 );
  EmitVertex();
  gsTexCoord = vec2(0, 1);
  gl_FrontColor = gl_FrontColorIn[1];
  gl_Position = vec4( (p1 - length_a * miter_a) / WIN_SCALE, 0.0, 1.0 );
  EmitVertex();
  gsTexCoord = vec2(0, 0);
  gl_FrontColor = gl_FrontColorIn[2];
  gl_Position = vec4( (p2 + length_b * miter_b) / WIN_SCALE, 0.0, 1.0 );
  EmitVertex();
  gsTexCoord = vec2(0, 1);
  gl_FrontColor = gl_FrontColorIn[2];
  gl_Position = vec4( (p2 - length_b * miter_b) / WIN_SCALE, 0.0, 1.0 );
  EmitVertex();

  EndPrimitive();
}