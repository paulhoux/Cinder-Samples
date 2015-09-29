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
out vec2 vertPixcoord;
out vec4 vertOffset[3];

void main()
{
	// Somehow calling "SMAABlendingWeightCalculationVS(texCoord, vPixcoord, vOffset);" did not work :(
	vertPixcoord = ciTexCoord0 * SMAA_RT_METRICS.zw;

	// We will use these offsets for the searches later on (see @PSEUDO_GATHER4):
	vertOffset[0] = mad(SMAA_RT_METRICS.xyxy, vec4(-0.25, -0.125,  1.25, -0.125), ciTexCoord0.xyxy);
	vertOffset[1] = mad(SMAA_RT_METRICS.xyxy, vec4(-0.125, -0.25, -0.125,  1.25), ciTexCoord0.xyxy);

	// And these for the searches, they indicate the ends of the loops:
	vertOffset[2] = mad(SMAA_RT_METRICS.xxyy,
					vec4(-2.0, 2.0, -2.0, 2.0) * float(SMAA_MAX_SEARCH_STEPS),
					vec4(vertOffset[0].xz, vertOffset[1].yw));

	vertTexCoord0 = ciTexCoord0;
	gl_Position = ciModelViewProjection * ciPosition;
}