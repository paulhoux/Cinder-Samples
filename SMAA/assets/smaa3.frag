#extension GL_EXT_gpu_shader4 : enable

// Pass in the render target metrics as a uniform
uniform vec4 SMAA_RT_METRICS; // (1/w, 1/h, w, h)

#define SMAA_PRESET_ULTRA
#define SMAA_INCLUDE_VS 0
#define SMAA_GLSL_3
#include "SMAA.glsl"

// Additional shader inputs
uniform sampler2D uColorTex;
uniform sampler2D uBlendTex;

in vec2 vertTexCoord0;
in vec4 vertOffset;

out vec4 fragColor;

void main()
{
    fragColor = SMAANeighborhoodBlendingPS(vertTexCoord0, vertOffset, uColorTex, uBlendTex);
}