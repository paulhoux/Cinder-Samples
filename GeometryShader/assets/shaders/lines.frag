#version 150

uniform sampler2D tex0;

in VertexData{
	vec2 mTexCoord;
	vec3 mColor;
} VertexIn;

out vec4 oColor;

void main(void)
{
	vec4 clr = texture( tex0, VertexIn.mTexCoord.xy );
	oColor.rgb = VertexIn.mColor * clr.rgb;
	oColor.a = clr.a;
}