#version 110

void main(void)
{
	// pass the (1st) texture coordinate of the vertex to the rasterizer
	gl_TexCoord[0] = gl_MultiTexCoord0;

	// transform the vertex from object space to '2D space' 
	// and pass it to the rasterizer
	gl_Position = ftransform();
}