#version 150

// Cinder will automatically send the default matrices and attributes to our shader.
uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec4 ciTexCoord0;

// We will pass the texture coordinate to the fragment shader as well.
out vec4 vTexCoord0;

void main(void)
{
	// Pass the (1st) texture coordinate of the vertex to the rasterizer.
	vTexCoord0 = ciTexCoord0;

	// Transform the vertex from object space to '2D space' 
	// and pass it to the rasterizer.
	gl_Position = ciModelViewProjection * ciPosition;
}