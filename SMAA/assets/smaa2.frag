#extension GL_EXT_gpu_shader4 : enable

// Pass in the render target metrics as a uniform
uniform vec4 SMAA_RT_METRICS; // (1/w, 1/h, w, h)

#define SMAA_PRESET_ULTRA
#define SMAA_INCLUDE_VS 0
#define SMAA_GLSL_3
#include "SMAA.glsl"

// Additional shader inputs
uniform sampler2D uEdgesTex;
uniform sampler2D uAreaTex;
uniform sampler2D uSearchTex;

in vec2 vertTexCoord0;
in vec2 vertPixcoord;
in vec4 vertOffset[3];

out vec4 fragColor;

void main()
{
	vec4 subsampleIndices = vec4(0.0, 0.0, 0.0, 0.0);

	fragColor = SMAABlendingWeightCalculationPS(vertTexCoord0, vertPixcoord, vertOffset, 
	   uEdgesTex, uAreaTex, uSearchTex, subsampleIndices);
}