#extension GL_EXT_gpu_shader4 : enable

// Pass in the render target metrics as a uniform
uniform vec4 SMAA_RT_METRICS; // (1/w, 1/h, w, h)

#define SMAA_PRESET_ULTRA
#define SMAA_INCLUDE_PS 0
#define SMAA_GLSL_3
#include "SMAA.glsl"

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec2 ciTexCoord0;

out vec2 vertTexCoord0;
out vec4 vertOffset;

void main()
{
	// Somehow calling "SMAANeighborhoodBlendingVS(texCoord, vOffset);" did not work :(
	vertOffset = mad(SMAA_RT_METRICS.xyxy, vec4( 1.0, 0.0, 0.0,  1.0), ciTexCoord0.xyxy);

	vertTexCoord0 = ciTexCoord0;
	gl_Position = ciModelViewProjection * ciPosition;
}