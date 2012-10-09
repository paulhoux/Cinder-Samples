#version 120
#extension GL_EXT_geometry_shader4 : enable

void main(void)
{
	// find the face normal
    vec3 d1 = gl_PositionIn[1].xyz - gl_PositionIn[0].xyz;
    vec3 d2 = gl_PositionIn[2].xyz - gl_PositionIn[0].xyz;
    vec3 normal = normalize( cross(d1,d2) );

	// color should be more opaque if normal is facing upward
    float a = 0.5 + 0.5 * dot(normal, vec3(0, 1, 0));

	// pass on the vertices and vertex colors
    for(int i=0; i< gl_VerticesIn; i++){
		gl_FrontColor = vec4( 1.0, 1.0, 1.0, 0.6 * pow(a, 10.0) );
        gl_Position = gl_PositionIn[i];
        EmitVertex();
    }
    EndPrimitive();
}