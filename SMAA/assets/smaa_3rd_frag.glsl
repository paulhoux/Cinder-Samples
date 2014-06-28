/**
 * Copyright (C) 2013 Jorge Jimenez (jorge@iryoku.com)
 * Copyright (C) 2013 Jose I. Echevarria (joseignacioechevarria@gmail.com)
 * Copyright (C) 2013 Belen Masia (bmasia@unizar.es)
 * Copyright (C) 2013 Fernando Navarro (fernandn@microsoft.com)
 * Copyright (C) 2013 Diego Gutierrez (diegog@unizar.es)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. As clarification, there
 * is no requirement that the copyright notice and permission be included in
 * binary distributions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define SMAA_PRESET_HIGH

#define lerp(a, b, t) mix(a, b, t)
#define saturate(a) clamp(a, 0.0, 1.0)
#define mad(a, b, c) (a * b + c)

uniform vec4 uRenderTargetMetrics; // (1/w, 1/h, w, h)

uniform sampler2D colorTex;
uniform sampler2D blendTex;

varying vec4 offset;

/**
 * Conditional move:
 */
void SMAAMovc(bvec2 cond, inout vec2 variable, vec2 value) {
	if (cond.x) variable.x = value.x;
	if (cond.y) variable.y = value.y;
}

void SMAAMovc(bvec4 cond, inout vec4 variable, vec4 value) {
	SMAAMovc(cond.xy, variable.xy, value.xy);
	SMAAMovc(cond.zw, variable.zw, value.zw);
}


//-----------------------------------------------------------------------------
// Neighborhood Blending Pixel Shader (Third Pass)

void main()
{
	vec4 color;
	vec2 texcoord = gl_TexCoord[0].st;

	// Fetch the blending weights for current pixel:
	vec4 a;
	a.x = texture2D(blendTex, offset.xy).a; // Right
	a.y = texture2D(blendTex, offset.zw).g; // Top
	a.wz = texture2D(blendTex, texcoord).xz; // Bottom / Left

	// Is there any blending weight with a value greater than 0.0?
	if (dot(a, vec4(1.0, 1.0, 1.0, 1.0)) < 1e-5) {
		color = texture2DLod(colorTex, texcoord, 0.0);
	} else {
		bool h = max(a.x, a.z) > max(a.y, a.w); // max(horizontal) > max(vertical)

		// Calculate the blending offsets:
		vec4 blendingOffset = vec4(0.0, a.y, 0.0, a.w);
		vec2 blendingWeight = a.yw;
		SMAAMovc(bvec4(h, h, h, h), blendingOffset, vec4(a.x, 0.0, a.z, 0.0));
		SMAAMovc(bvec2(h, h), blendingWeight, a.xz);
		blendingWeight /= dot(blendingWeight, vec2(1.0, 1.0));

		// Calculate the texture coordinates:
		vec4 blendingCoord = mad(blendingOffset, vec4(uRenderTargetMetrics.xy, -uRenderTargetMetrics.xy), texcoord.xyxy);

		// We exploit bilinear filtering to mix current pixel with the chosen
		// neighbor:
		color = blendingWeight.x * texture2DLod(colorTex, blendingCoord.xy, 0.0);
		color += blendingWeight.y * texture2DLod(colorTex, blendingCoord.zw, 0.0);
	}

	gl_FragColor = color;
}