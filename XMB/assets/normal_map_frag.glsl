// adapted from: http://www.gamedev.net/topic/594457-calculate-normals-from-a-displacement-map/

#version 120

uniform sampler2D texture;

// Scharr operator constants combined with luminance weights
const vec3 sobel1 = vec3(0.2990, 0.5870, 0.1140) * vec3(3.0);
const vec3 sobel2 = vec3(0.2990, 0.5870, 0.1140) * vec3(10.0);

// luminance weights, scaled by the average of 3x3 normalised kernel weights (including zeros)
const vec3 lum = vec3(0.2990, 0.5870, 0.1140) * vec3(0.355556);

void main(void)
{
   vec2 coord[3];
   vec4 texel[6];
   vec3 normal;

   // 3x3 kernel offsets
   vec2 d = vec2(dFdx(gl_TexCoord[0].s), dFdy(gl_TexCoord[0].t));
   coord[0] = gl_TexCoord[0].st - d;
   coord[1] = gl_TexCoord[0].st;
   coord[2] = gl_TexCoord[0].st + d;

   // Sobel operator, U direction
   texel[0] = texture2D(texture, vec2(coord[2].s, coord[0].t)) - texture2D(texture, vec2(coord[0].s, coord[0].t));
   texel[1] = texture2D(texture, vec2(coord[2].s, coord[1].t)) - texture2D(texture, vec2(coord[0].s, coord[1].t));
   texel[2] = texture2D(texture, vec2(coord[2].s, coord[2].t)) - texture2D(texture, vec2(coord[0].s, coord[2].t));

   // Sobel operator, V direction
   texel[3] = texture2D(texture, vec2(coord[0].s, coord[0].t)) - texture2D(texture, vec2(coord[0].s, coord[2].t));
   texel[4] = texture2D(texture, vec2(coord[1].s, coord[0].t)) - texture2D(texture, vec2(coord[1].s, coord[2].t));
   texel[5] = texture2D(texture, vec2(coord[2].s, coord[0].t)) - texture2D(texture, vec2(coord[2].s, coord[2].t));

   // compute luminance from each texel, apply kernel weights, and sum them all
   normal.x  = dot(texel[0].rgb, sobel1);
   normal.x += dot(texel[1].rgb, sobel2);
   normal.x += dot(texel[2].rgb, sobel1);

   normal.z  = dot(texel[3].rgb, sobel1);
   normal.z += dot(texel[4].rgb, sobel2);
   normal.z += dot(texel[5].rgb, sobel1);

   normal.y = dot(texture2D(texture, coord[1]).rgb, lum);

   gl_FragColor = vec4(vec3(0.5) + normalize(normal) * 0.5, 1.0);
}