#extension GL_EXT_gpu_shader4 : enable

// Pass in the render target metrics as a uniform
uniform vec4 SMAA_RT_METRICS; // (1/w, 1/h, w, h)

#define SMAA_PRESET_ULTRA
#define SMAA_INCLUDE_PS 0
#define SMAA_GLSL // Custom compatibility profile, not available in original
#include "SMAA.h"

// Shader outputs
varying float2 vPixcoord;
varying float4 vOffset[3];

void main()
{
	float2 texCoord = gl_MultiTexCoord0.st;

	// Somehow calling "SMAABlendingWeightCalculationVS(texCoord, vPixcoord, vOffset);" did not work :(
	vPixcoord = texCoord * SMAA_RT_METRICS.zw;

	// We will use these offsets for the searches later on (see @PSEUDO_GATHER4):
	vOffset[0] = mad(SMAA_RT_METRICS.xyxy, float4(-0.25, -0.125,  1.25, -0.125), texCoord.xyxy);
	vOffset[1] = mad(SMAA_RT_METRICS.xyxy, float4(-0.125, -0.25, -0.125,  1.25), texCoord.xyxy);

	// And these for the searches, they indicate the ends of the loops:
	vOffset[2] = mad(SMAA_RT_METRICS.xxyy,
					float4(-2.0, 2.0, -2.0, 2.0) * float(SMAA_MAX_SEARCH_STEPS),
					float4(vOffset[0].xz, vOffset[1].yw));

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}