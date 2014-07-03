#extension GL_EXT_gpu_shader4 : enable

// Pass in the render target metrics as a uniform
uniform vec4 SMAA_RT_METRICS; // (1/w, 1/h, w, h)

#define SMAA_PRESET_ULTRA
#define SMAA_INCLUDE_VS 0
#define SMAA_GLSL // Custom compatibility profile, not available in original
#include "SMAA.h"

// Additional shader inputs
uniform sampler2D uColorTex;
varying float4    vOffset[3];

void main()
{
    float2 texCoord = gl_TexCoord[0].st;
    gl_FragColor.rg = SMAALumaEdgeDetectionPS(texCoord, vOffset, uColorTex);
}