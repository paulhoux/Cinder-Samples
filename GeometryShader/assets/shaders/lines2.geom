#version 150 

// This version of the line shader simply cuts off the corners and
// draws the line with no overdraw on neighboring segments at all

uniform float	THICKNESS;		// the thickness of the line in pixels
uniform vec2	WIN_SCALE;		// the size of the viewport in pixels

layout( lines_adjacency ) in;
layout( triangle_strip, max_vertices = 7 ) out;

in VertexData{
	vec3 mColor;
} VertexIn[4];

out VertexData{
	vec2 mTexCoord;
	vec3 mColor;
} VertexOut;

vec2 toScreenSpace( vec4 vertex )
{
	return vec2( vertex.xy / vertex.w ) * WIN_SCALE;
}

void main( void )
{
	// get the four vertices passed to the shader:
	vec2 p0 = toScreenSpace( gl_in[0].gl_Position );	// start of previous segment
	vec2 p1 = toScreenSpace( gl_in[1].gl_Position );	// end of previous segment, start of current segment
	vec2 p2 = toScreenSpace( gl_in[2].gl_Position );	// end of current segment, start of next segment
	vec2 p3 = toScreenSpace( gl_in[3].gl_Position );	// end of next segment

	// perform naive culling
	vec2 area = WIN_SCALE * 1.2;
	if( p1.x < -area.x || p1.x > area.x ) return;
	if( p1.y < -area.y || p1.y > area.y ) return;
	if( p2.x < -area.x || p2.x > area.x ) return;
	if( p2.y < -area.y || p2.y > area.y ) return;

	// determine the direction of each of the 3 segments (previous, current, next)
	vec2 v0 = normalize( p1 - p0 );
	vec2 v1 = normalize( p2 - p1 );
	vec2 v2 = normalize( p3 - p2 );

	// determine the normal of each of the 3 segments (previous, current, next)
	vec2 n0 = vec2( -v0.y, v0.x );
	vec2 n1 = vec2( -v1.y, v1.x );
	vec2 n2 = vec2( -v2.y, v2.x );

	// determine miter lines by averaging the normals of the 2 segments
	vec2 miter_a = normalize( n0 + n1 );	// miter at start of current segment
	vec2 miter_b = normalize( n1 + n2 );	// miter at end of current segment

	// determine the length of the miter by projecting it onto normal and then inverse it
	float length_a = THICKNESS / dot( miter_a, n1 );
	float length_b = THICKNESS / dot( miter_b, n1 );

	if( dot( v0, n1 ) > 0 ) {
		// start at negative miter
		VertexOut.mTexCoord = vec2( 0, 1 );
		VertexOut.mColor = VertexIn[1].mColor;
		gl_Position = vec4( ( p1 - length_a * miter_a ) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();

		// proceed to positive normal
		VertexOut.mTexCoord = vec2( 0, 0 );
		VertexOut.mColor = VertexIn[1].mColor;
		gl_Position = vec4( ( p1 + THICKNESS * n1 ) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();
	}
	else {
		// start at negative normal
		VertexOut.mTexCoord = vec2( 0, 1 );
		VertexOut.mColor = VertexIn[1].mColor;
		gl_Position = vec4( ( p1 - THICKNESS * n1 ) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();

		// proceed to positive miter
		VertexOut.mTexCoord = vec2( 0, 0 );
		VertexOut.mColor = VertexIn[1].mColor;
		gl_Position = vec4( ( p1 + length_a * miter_a ) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();
	}

	if( dot( v2, n1 ) < 0 ) {
		// proceed to negative miter
		VertexOut.mTexCoord = vec2( 0, 1 );
		VertexOut.mColor = VertexIn[2].mColor;
		gl_Position = vec4( ( p2 - length_b * miter_b ) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();

		// proceed to positive normal
		VertexOut.mTexCoord = vec2( 0, 0 );
		VertexOut.mColor = VertexIn[2].mColor;
		gl_Position = vec4( ( p2 + THICKNESS * n1 ) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();

		// end at positive normal
		VertexOut.mTexCoord = vec2( 0, 0 );
		VertexOut.mColor = VertexIn[2].mColor;
		gl_Position = vec4( ( p2 + THICKNESS * n2 ) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();
	}
	else {
		// proceed to negative normal
		VertexOut.mTexCoord = vec2( 0, 1 );
		VertexOut.mColor = VertexIn[2].mColor;
		gl_Position = vec4( ( p2 - THICKNESS * n1 ) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();

		// proceed to positive miter
		VertexOut.mTexCoord = vec2( 0, 0 );
		VertexOut.mColor = VertexIn[2].mColor;
		gl_Position = vec4( ( p2 + length_b * miter_b ) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();

		// end at negative normal
		VertexOut.mTexCoord = vec2( 0, 1 );
		VertexOut.mColor = VertexIn[2].mColor;
		gl_Position = vec4( ( p2 - THICKNESS * n2 ) / WIN_SCALE, 0.0, 1.0 );
		EmitVertex();
	}
	EndPrimitive();
}